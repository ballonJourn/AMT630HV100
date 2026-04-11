#include <string.h>

#include "amt630h.h"
#include "typedef.h"
#include "uart.h"
#include "aic.h"
#include "update.h"
#include "spi.h"
#include "crc32.h"
#include "sysinfo.h"
#include "board.h"
#include "sysctl.h"
#include "sdmmc.h"

#define UART_CLK	CLK_24MHZ
#define UART_ISR_PASS_LIMIT	1

static unsigned int ulUartBase[UART_NUM] = {REGS_UART0_BASE, REGS_UART1_BASE, REGS_UART2_BASE, REGS_UART3_BASE};

void InitUart(unsigned int baud)
{
	unsigned int Baud_Rate_Divisor;
	unsigned int val;

    //select pad
	val = rSYS_PAD_CTRL02;
	val &= ~(0xF<<12);
	val |=(0x1<<14)|(0x1<<12);
	rSYS_PAD_CTRL02 = val;

	Baud_Rate_Divisor = ((CLK_24MHZ<<3) + baud)/(baud<<1);
	rUART_IBRD = Baud_Rate_Divisor >> 6;
	rUART_FBRD = Baud_Rate_Divisor & 0x3f;

	rUART_LCR_H = 0x70;//data len:8 bit,parity checking disable
	rUART_IFLS = 0x19;
	rUART_CR = 0x301;
}

static char HexToChar(unsigned char value)
{
	value &= 0x0f;

	if ( value < 10 )
		return(0x30 + value);
	else
		return(0x60 + value - 9);
}


static void ShortToStr(unsigned short value, char *str)
{
	str[0] = HexToChar(value >> 12);
	str[1] = HexToChar((value >> 8) & 0x0f);
	str[2] = HexToChar((value >> 4) & 0x0f);
	str[3] = HexToChar(value & 0x0f);
	str[4] = 0;
}

void IntToStr(unsigned int value, char *str)
{
	ShortToStr(value >> 16, str);
	ShortToStr(value & 0xffff, str + 4);
	str[8] = 0;
}

void  SendUartString(char * buf)
{
	int i = 0;
	while ( buf[i] != 0)
	{
		while ( !(rUART_FR & 0x20) )
		{
			rUART_DR = buf[i++];
			if ( buf[i] == 0 )
				return;
		}
		while ( (rUART_FR & 0x20));
	}
}

void SendUartChar(char ch)
{
	while ( (rUART_FR & 0x20) );
	rUART_DR = ch;
}


void PrintVariableValueHex(char * variable, unsigned int value)
{
	char buf[10];

	SendUartString(variable);
	SendUartString(": 0x");
	IntToStr(value, buf);
	SendUartString(buf);
	SendUartString("\r\n");
}

void SendUartWord(unsigned int data)
{
    char buf[10];

    SendUartString("0x");
	IntToStr(data, buf);
	SendUartString(buf);
	SendUartString("\r\n");
}
void uart_puts(const UINT8*buf)
{
	INT32 i = 0;
	while ( buf[i] != 0)
	{
		while( !(rUART_FR & 0x20) )
		{
			if ( buf[i] == '\n' )
			{
				rUART_DR = '\r';
				while( (rUART_FR & 0x20) );
			}

			rUART_DR = buf[i++];
			if ( buf[i] == 0 )
				return;
		}
		while( (rUART_FR & 0x20) );
	}
}

/* retarget printf function */
int putchar(int ch)
{
	while (readl(REGS_UART0_BASE + UART_FR) & UART_FR_TXFF);
	writel(ch, REGS_UART0_BASE + UART_DR);

	return ch;
}

typedef enum {
	UART_FRAME_START,
	UART_FRAME_FILEINFO,
	UART_FRAME_FILEXFER,
	UART_FRAME_FINISH,
} eUartFrameType;

#define UUP_STATE_IDLE		0
#define UUP_STATE_START		1
#define UUP_STATE_END		2

#define UUP_ACK_OK			1
#define UUP_ACK_FAIL		0

#define UUP_MAX_FILE_SIZE	0x1000000
#define UUP_BUF_SIZE	(BYTESPERPAGE * PAGESPERSECTORS)
#define UUP_PACKET_SIZE		128
#define UUP_MAX_FRAME_LEN	(UUP_PACKET_SIZE + 16)
#define UUP_RX_FRAME_NUM	16
#define UUP_MAX_LOADER_SIZE	STEPLDR_MAX_SIZE//0x10000


typedef struct {
	int type;
	unsigned char *buf;
	unsigned int len;
	int ack;
} UartFrame;

#pragma data_alignment=16
static unsigned char uup_rx_buf[UUP_RX_FRAME_NUM][UUP_MAX_FRAME_LEN];
static unsigned char *uup_rx_ptr;
static int uup_rx_rev_len = 0;
static int uup_rx_head = 0;
static int uup_rx_tail = 0;
static int uup_rx_state = 0;
static int uup_rx_data_len = 0;
static int uup_status = UUP_STATE_IDLE;
static int uup_file_type = 0;
static unsigned int uup_file_offset;
static int uup_file_size = 0;
static int uup_packet_num = 0;
static int uup_rev_packet = 0;
static int uup_rev_len = 0;

#pragma data_alignment=16
static unsigned char uup_buf[UUP_MAX_LOADER_SIZE];
static unsigned int uup_buf_len = 0;

static void uart_select_pad(int id)
{
	switch (id) {
	case UART_ID0:
		vSysctlConfigure(SYS_PAD_CTRL02, 12, 0xf, 0x5);
		break;
	case UART_ID1:
		vSysctlConfigure(SYS_PAD_CTRL02, 16, 0xf, 0x5);
		break;
	case UART_ID2:
		vSysctlConfigure(SYS_PAD_CTRL02, 20, 0xf, 0x5);
		break;
	case UART_ID3:
		vSysctlConfigure(SYS_PAD_CTRL02, 24, 0xf, 0x5);
		break;
	}
}

void uart_init(int id, unsigned int baud, unsigned int flags)
{
	unsigned int lcr_h, quot, cr;
	unsigned int regbase;
	int count = 0;

	//uart_select_pad(id);

	regbase = ulUartBase[id];

	if (id == UART_ID0 || id == UART_ID2) {
		if (baud > UART_CLK / 16)
			quot = (UART_CLK * 8 + baud / 2) / baud;
		else
			quot = (UART_CLK * 4 + baud / 2) / baud;

		switch (flags & CSIZE) {
		case CS5:
			lcr_h = UART_LCRH_WLEN_5;
			break;
		case CS6:
			lcr_h = UART_LCRH_WLEN_6;
			break;
		case CS7:
			lcr_h = UART_LCRH_WLEN_7;
			break;
		default: // CS8
			lcr_h = UART_LCRH_WLEN_8;
			break;
		}
		if (flags & CSTOPB)
			lcr_h |= UART_LCRH_STP2;
		if (flags & PARENB) {
			lcr_h |= UART_LCRH_PEN;
			if (!(flags & PARODD))
				lcr_h |= UART_LCRH_EPS;
			if (flags & CMSPAR)
				lcr_h |= UART_LCRH_SPS;
		}
		lcr_h |= UART_LCRH_FEN;

		/* Provoke TX FIFO interrupt into asserting */
		cr = UART_CR_UARTEN | UART_CR_TXE | UART_CR_LBE;
	reconfig:
		writel(cr, regbase + UART_CR);
		writel(0, regbase + UART_FBRD);
		writel(1, regbase + UART_IBRD);
		writel(0, regbase + UART_LCRH);
		writel(0, regbase + UART_DR);
		while (readl(regbase + UART_FR) & UART_FR_BUSY);
		if (count++ < 10 && !(readl(regbase + UART_RIS) & UART_TXIS)) {
			SendUartString("ERROR! Uart status wrong.\n");
			goto reconfig;
		}

		/* first, disable everything */
		writel(0, regbase + UART_CR);

		cr = UART_CR_RXE | UART_CR_TXE;
		if (flags & CRTSCTS)
			cr |= UART_CR_CTSEN | UART_CR_RTSEN;

		/* Set baud rate */
		writel(quot & 0x3f, regbase + UART_FBRD);
		writel(quot >> 6, regbase + UART_IBRD);

		writel(UART_RTIM | UART_RXIM, regbase + UART_IMSC);
		writel(UART_IFLS_RX4_8 | UART_IFLS_TX4_8, regbase + UART_IFLS);
		writel(lcr_h, regbase + UART_LCRH);
		writel(cr | UART_CR_UARTEN, regbase + UART_CR);
	} else {
		//set modem
		writel(0, regbase + UART485_MCR);

		// baud = clk/(16*U_DLL)
		//set baud rate
		writel(readl(regbase + UART485_LCR) | (1 << 7), regbase + UART485_LCR);
		writel(UART_CLK / (16 * baud), regbase + UART485_DLL);
		writel(0, regbase + UART485_DLH);
		//Set fractional baud rate
		//Fractional baud divisor = ((uart_clk%(16*baud))/(16*baud)) * 16 = (uart_clk%(16*baud)) * 16 / (16*baud);
		writel((UART_CLK % (16*baud)) * 16 / (16*baud), regbase + UART485_DLF);

		writel(readl(regbase + UART485_LCR) & ~(1 << 7), regbase + UART485_LCR);

		cr = readl(regbase + UART485_LCR);
		//校验
		cr &= ~(7 << 3);
		if (flags & PARENB) {
			cr  |= (1 << 3);
			if (!(flags & PARODD))
				cr |= (1 << 4);
			if (flags & CMSPAR)
				cr |= (1 << 5);
		}
		//停止位
		if (flags & CSTOPB)
			cr |= (1 << 2);
		else
			cr &= ~(1 << 2);
		//数据位
		cr &= ~(3 << 0);
		switch (flags & CSIZE) {
		case CS5:
			break;
		case CS6:
			cr |= 1;
			break;
		case CS7:
			cr |= 2;
			break;
		default: // CS8
			cr |= 3;
			break;
		}
		writel(cr, regbase + UART485_LCR);

		//set fifo
		writel((2 << 6) | (3 << 4) | 7, regbase + UART485_FCR);

		//enable rx and err interrupt
		writel((1 << 4) | 1, regbase + UART485_IER);

		//设置通信模式全双工还是半双工
		//485mode disable
		writel(0, regbase + UART485_TCR);
	}

	uart_select_pad(id);
}


static int uart_rx_chars(int id)
{
	unsigned int regbase = ulUartBase[id];
	unsigned int status;
	unsigned int ch, max_count = 256;
	int fifotaken = 0;

	while (max_count--) {
		if (id == UART_ID0 || id == UART_ID2) {
			status = readl(regbase + UART_FR);
			if (status & UART_FR_RXFE)
				break;

			/* Take chars from the FIFO and update status */
			ch = readl(regbase + UART_DR);
		} else {
			status = readl(regbase + UART485_LSR);
			if (!(status & UART485_LSR_DR))
				break;

			/* Take chars from the FIFO and update status */
			ch = readl(regbase + UART485_RBR);
		}

		fifotaken++;
		switch (uup_rx_state) {
		case 0:
			if (ch == 0x55) {
				uup_rx_state++;
				uup_rx_rev_len = 0;
				uup_rx_ptr = &uup_rx_buf[uup_rx_head][0];
			}
			break;
		case 1:
			if (ch == 0x81)
				uup_rx_state++;
			else
				uup_rx_state = 0;
			*uup_rx_ptr++ = ch;
			break;
		case 2:
			if (ch == 0xc6)
				uup_rx_state++;
			else
				uup_rx_state = 0;
			*uup_rx_ptr++ = ch;
			break;
		case 3:
			uup_rx_data_len = ch;
			if (uup_rx_data_len <= 0 || uup_rx_data_len > UUP_PACKET_SIZE + 2) {
				SendUartString("Invalid uart frame len.\n");
				uup_rx_state = 0;
			} else {
				uup_rx_state++;
				*uup_rx_ptr++ = ch;
			}
			break;
		case 4:
			*uup_rx_ptr++ = ch;
			if (++uup_rx_rev_len == uup_rx_data_len)
				uup_rx_state++;
			break;
		case 5:
			*uup_rx_ptr++ = ch;
			uup_rx_head = (uup_rx_head + 1) % UUP_RX_FRAME_NUM;
			uup_rx_state = 0;
			break;
		}

		/* discard when rx buf is full */

	}

	return fifotaken;
}

void uart_tx_chars(int id, unsigned char *buf, int len)
{
	unsigned int regbase = ulUartBase[id];
	int i;

	for (i = 0; i < len; i++) {
		if (id == UART_ID0 || id == UART_ID2) {
			while (readl(regbase + UART_FR) & UART_FR_TXFF);
			writel(buf[i], regbase + UART_DR);
		} else {
			while (!(readl(regbase + UART485_LSR) & UART485_LSR_TEMT));
			writel(buf[i], regbase + UART485_THR);
		}
	}
}

void uart_update_int_handler(void *param)
{
	int id = (int)param;
	unsigned int regbase = ulUartBase[id];
	unsigned int status;
	unsigned int imsc;
	uint32_t pass_counter = UART_ISR_PASS_LIMIT;

	imsc = readl(regbase + UART_IMSC);
	status = readl(regbase + UART_RIS) & imsc;
	if (status) {
		do {
			writel(status & ~(/*UART_TXIS | */UART_RTIS | UART_RXIS),
				    regbase + UART_ICR);

			if (status & (UART_RTIS | UART_RXIS)) {
				uart_rx_chars(id);
			}
			/* if (status & UART_TXIS)
				xUartTxChars(uap, pdTRUE); */

			if (pass_counter-- == 0)
				break;

			status = readl(regbase + UART_RIS) & imsc;
		} while (status != 0);
	}
}

void uart485_update_int_handler(void *param)
{
	int id = (int)param;
	unsigned int regbase = ulUartBase[id];
	uint32_t iir;

	iir = readl(regbase + UART485_IIR);
	switch (iir & UART485_IIR_IID_MASK) {
	case UART485_IIR_IID_REV_DATA_AVAIL:
	case UART485_IIR_IID_REV_LINE_STATUS:
	case UART485_IIR_IID_CHAR_TIMEOUT:
		uart_rx_chars(id);
	}
}

static void uart_send_ack(int type, int ret)
{
	unsigned char buf[7] = {0x55, 0x80, 0xc5, 0x02, 0x00, 0x00, 0x00};
	int i;

	buf[4] = type;
	buf[5] = ret;
	for (i = 1; i < 6; i++)
		buf[6] ^= buf[i];

	uart_tx_chars(UART_MCU_PORT, buf, 7);
}

static int uart_rev_frame(UartFrame *frame)
{
	unsigned char *buf;
	int len;
	unsigned char checksum = 0;
	int i;

	if (uup_rx_tail == uup_rx_head) {
		return 0;
	}
	buf = &uup_rx_buf[uup_rx_tail][0];
	uup_rx_tail = (uup_rx_tail + 1) % UUP_RX_FRAME_NUM;
	len = buf[2];
	for (i = 0; i < len + 3; i++)
		checksum ^= buf[i];
	if (checksum == buf[len + 3]) {
		frame->buf = &buf[4];
		frame->len = len - 1;
		frame->type = buf[3];
	} else {
		SendUartString("rev frame checksum err.\n");
		/* for (int i = 0; i < len + 4; i++) {
			if (i && i % 16 == 0)
				printf("\n");
			printf("%.2x, ", buf[i]);
		}
		printf("\n0x%x, 0x%x.\n", checksum, buf[len + 3]); */
		uart_send_ack(buf[3], UUP_ACK_FAIL);
		return 0;
	}

	return 1;
}


void updateFromUart(int id)
{
	UartFrame rx_frame;
	unsigned char *buf;
	unsigned int  checksum = 0, calc_checksum = 0xffffffff;
	unsigned int framelen;
	unsigned int packetnum;
	SysInfo *sysinfo = GetSysInfo();

	uart_init(id, 115200, 0);
	if (id == UART_ID0 || id == UART_ID2)
		request_irq(UART0_IRQn + id, 0, uart_update_int_handler, (void*)id);
	else
		request_irq(UART0_IRQn + id, 0, uart485_update_int_handler, (void*)id);

	do {
		if (!uart_rev_frame(&rx_frame))
			continue;

		switch (rx_frame.type) {
		case UART_FRAME_START:
			uart_send_ack(rx_frame.type, UUP_ACK_OK);
			uup_status = UUP_STATE_START;
			update_logo_init();
			break;
		case UART_FRAME_FILEINFO:
			if (uup_status != UUP_STATE_START)
				break;
			buf = rx_frame.buf;
			uup_file_type = buf[0];
			if (uup_file_type > UPFILE_TYPE_STEPLDR) {
				PrintVariableValueHex("Rev wrong file type", uup_file_type);
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				break;
			}
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			uup_file_offset = SpiGetUpfileOffset(uup_file_type);
#else
			uup_file_offset = EmmcGetUpfileOffset(uup_file_type);
#endif
			if (uup_file_offset == 0xffffffff) {
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				break;
			}
			uup_packet_num = (buf[1] << 16) | (buf[2] << 8) | buf[3];
			uup_file_size = UUP_PACKET_SIZE * uup_packet_num;
			if (uup_file_size > UUP_MAX_FILE_SIZE - uup_file_offset) {
				SendUartString("Rev wrong file size.\n");
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				break;
			}
			uart_send_ack(rx_frame.type, UUP_ACK_OK);
			break;
		case UART_FRAME_FILEXFER:
			if (uup_status != UUP_STATE_START)
				break;
			buf = rx_frame.buf;
			packetnum = buf[0];
			PrintVariableValueHex("uup_rev_packet", uup_rev_packet);
			if ((uup_rev_packet & 0xff) != packetnum) {
				SendUartString("Wrong packet number.\n");
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				break;
			}
			if (uup_rev_packet == 0) {
				if (uup_file_type == UPFILE_TYPE_WHOLE) {
					UpFileHeader *header = (UpFileHeader *)&buf[1];
					if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
						SendUartString("Wrong whole file magic.\n");
						uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
						break;
					}
					checksum = header->checksum;
					sysinfo->app_size = header->files[0].size;
					PrintVariableValueHex("sysinfo->appsize ", sysinfo->app_size);
				} else if (uup_file_type == UPFILE_TYPE_RESOURCE) {
					RomHeader *header = (RomHeader *)&buf[1];
					if (header->magic != MKTAG('R', 'O', 'M', 'A')) {
						SendUartString("Wrong resource file magic.\n");
						uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
						break;
					}
					checksum = header->checksum;
				} else if (uup_file_type == UPFILE_TYPE_ANIMATION) {
					BANIHEADER *header = (BANIHEADER *)&buf[1];
					if (header->magic != MKTAG('B', 'A', 'N', 'I')) {
						SendUartString("Wrong animation file magic.\n");
						uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
						break;
					}
					checksum = header->checksum;
				} else if (uup_file_type == UPFILE_TYPE_APP) {
					unsigned int magic = buf[1] | (buf[2] << 8) | (buf[3] << 16) | (buf[4] << 24);
					if (magic != UPFILE_APP_MAGIC) {
						SendUartString("Wrong app file magic.\n");
						uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
						break;
					}
					unsigned char *tmp = buf + 1 + APPLDR_CHECKSUM_OFFSET;
					checksum = tmp[0] | (tmp[1] <<8) | (tmp[2] << 16) | (tmp[3] << 24);
				}
			}
			framelen = rx_frame.len - 1;
			/* only last frame size is less than UUP_PACKET_SIZE */
			if (framelen > UUP_PACKET_SIZE ||
				(framelen < UUP_PACKET_SIZE && uup_rev_packet != uup_packet_num - 1)) {
				SendUartString("Wrong packet len.\n");
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				break;
			}
			memcpy(uup_buf + uup_buf_len, buf + 1, framelen);
			uup_buf_len += framelen;
			if (uup_buf_len > UUP_MAX_LOADER_SIZE) {
				SendUartString("loader file is too large.\n");
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				break;
			}
			uup_rev_packet++;
			//loader程序如果升级错误会导致系统起不来并且不能再继续升级，因此需要
			//等数据校验成功后再烧录(loader程序通常并不需要升级)
			if (uup_file_type < UPFILE_TYPE_FIRSTLDR && uup_buf_len == UUP_BUF_SIZE) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				if(!FlashBurn(uup_buf, uup_file_offset, UUP_BUF_SIZE, 0))
#else
				if(!EmmcBurn(uup_buf, uup_file_offset, UUP_BUF_SIZE, 0))
#endif
				{
					uart_send_ack(rx_frame.type, UUP_ACK_OK);
				} else {
					SendUartString("Burn failed.\n");
					uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
					break;
				}
				//文件起始部分需要先将头信息里的checksum清0来计算checksum
				if (uup_rev_packet == UUP_BUF_SIZE / UUP_PACKET_SIZE) {
					if (uup_file_type == UPFILE_TYPE_WHOLE) {
						UpFileHeader *pheader = (UpFileHeader *)uup_buf;
						pheader->checksum = 0;
					} else if (uup_file_type == UPFILE_TYPE_RESOURCE) {
						RomHeader *pheader = (RomHeader *)uup_buf;
						pheader->checksum = 0;
					} else if (uup_file_type == UPFILE_TYPE_ANIMATION) {
						BANIHEADER *pheader = (BANIHEADER *)uup_buf;
						pheader->checksum = 0;
					} else if (uup_file_type == UPFILE_TYPE_APP) {
						unsigned int *tmp = (unsigned int *)(uup_buf + APPLDR_CHECKSUM_OFFSET);
						*tmp = 0;
					}
				}
				calc_checksum = xcrc32(uup_buf, UUP_BUF_SIZE, calc_checksum);
				uup_buf_len = 0;
				uup_file_offset += UUP_BUF_SIZE;
				uup_rev_len += UUP_BUF_SIZE;
				update_progress_set(uup_rev_len * 100 / uup_file_size);
			} else {
				uart_send_ack(rx_frame.type, UUP_ACK_OK);
			}
			break;
		case UART_FRAME_FINISH:
			buf = rx_frame.buf;
			if (!buf[0]) {
				SendUartString("update end with error!\n");
				uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				uup_status = UUP_STATE_END;
				break;
			}
			if (uup_file_type < UPFILE_TYPE_FIRSTLDR) {
				if (uup_buf_len) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
					if(FlashBurn(uup_buf, uup_file_offset, uup_buf_len, 0))
#else
					if(EmmcBurn(uup_buf, uup_file_offset, uup_buf_len, 0))
#endif
					{
						SendUartString("update fail with burn failed.!\n");
						calc_checksum = 0xFFFFFFFF;
					} else {
						//如果升级文件小于UUP_BUF_SIZE，此时收到的是文件起始部分
						if (uup_rev_packet < UUP_BUF_SIZE / UUP_PACKET_SIZE) {
							if (uup_file_type == UPFILE_TYPE_WHOLE) {
								UpFileHeader *pheader = (UpFileHeader *)uup_buf;
								pheader->checksum = 0;
							} else if (uup_file_type == UPFILE_TYPE_RESOURCE) {
								RomHeader *pheader = (RomHeader *)uup_buf;
								pheader->checksum = 0;
							} else if (uup_file_type == UPFILE_TYPE_ANIMATION) {
								BANIHEADER *pheader = (BANIHEADER *)uup_buf;
								pheader->checksum = 0;
							} else if (uup_file_type == UPFILE_TYPE_APP) {
								unsigned int *checksum = (unsigned int *)(uup_buf + APPLDR_CHECKSUM_OFFSET);
								*checksum = 0;
							}
						}
						calc_checksum = xcrc32(uup_buf, uup_buf_len, calc_checksum);
					}
				}
				if (calc_checksum == checksum) {
					SendUartString("update finish!\n");
					update_progress_set(100);
					if (uup_file_type == UPFILE_TYPE_APP)
						sysinfo->app_size = uup_file_size;
					sysinfo->update_status = UPDATE_STATUS_END;
					SaveSysInfo(sysinfo);
					uart_send_ack(rx_frame.type, UUP_ACK_OK);
				} else {
					SendUartString("update checksum fail!\n");
					uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				}
			} else {
				unsigned int *tmp = (unsigned int *)(uup_buf + APPLDR_CHECKSUM_OFFSET);
				checksum = *tmp;
				*tmp = 0;
				calc_checksum = xcrc32(uup_buf, uup_buf_len, calc_checksum);
				if (calc_checksum == checksum) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
					if (!FlashBurn(uup_buf, uup_file_offset, uup_buf_len, 1))
#else
					if (!EmmcBurn(uup_buf, uup_file_offset, uup_buf_len, 1))
#endif
					{
						if (uup_file_type == UPFILE_TYPE_STEPLDR)
							sysinfo->stepldr_size = uup_buf_len;
						sysinfo->update_status = UPDATE_STATUS_END;
						SaveSysInfo(sysinfo);
						uart_send_ack(rx_frame.type, UUP_ACK_OK);
					} else {
						SendUartString("burn loader fail!\n");
						uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
					}
				} else {
					SendUartString("update checksum fail!\n");
					uart_send_ack(rx_frame.type, UUP_ACK_FAIL);
				}
			}
			uup_status = UUP_STATE_END;
			break;
		}
	} while (uup_status != UUP_STATE_END);
}

