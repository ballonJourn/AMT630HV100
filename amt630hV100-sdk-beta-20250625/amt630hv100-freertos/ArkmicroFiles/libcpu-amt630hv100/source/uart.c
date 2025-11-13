#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "serial.h"
#include "sysinfo.h"
#include "sfud.h"
#include "source/crc32.h"
#include "storage_param1/hcn_usr_param.h"

#ifdef DELTA_UPDATE_SUPPORT
#include "delta_update.h"
#endif

/* UART Register Offsets. */
#define UART_DR		0x00	/* Data read or written from the interface. */
#define UART_RSR	0x04	/* Receive status register (Read). */
#define UART_FR		0x18	/* Flag register (Read only). */
#define UART_ILPR	0x20	/* IrDA low power counter register. */
#define UART_IBRD	0x24	/* Integer baud rate divisor register. */
#define UART_FBRD	0x28	/* Fractional baud rate divisor register. */
#define UART_LCRH	0x2c	/* Line control register. */
#define UART_CR		0x30	/* Control register. */
#define UART_IFLS	0x34	/* Interrupt fifo level select. */
#define UART_IMSC	0x38	/* Interrupt mask. */
#define UART_RIS	0x3c	/* Raw interrupt status. */
#define UART_MIS	0x40	/* Masked interrupt status. */
#define UART_ICR	0x44	/* Interrupt clear register. */
#define UART_DMACR	0x48	/* DMA control register. */


#define UART_DR_OE		(1 << 11)
#define UART_DR_BE		(1 << 10)
#define UART_DR_PE		(1 << 9)
#define UART_DR_FE		(1 << 8)

#define UART_RSR_OE 		0x08
#define UART_RSR_BE 		0x04
#define UART_RSR_PE 		0x02
#define UART_RSR_FE 		0x01

#define UART_FR_RI			0x100
#define UART_FR_TXFE		0x080
#define UART_FR_RXFF		0x040
#define UART_FR_TXFF		0x020
#define UART_FR_RXFE		0x010
#define UART_FR_BUSY		0x008
#define UART_FR_DCD 		0x004
#define UART_FR_DSR 		0x002
#define UART_FR_CTS 		0x001
#define UART_FR_TMSK		(UART_FR_TXFF + UART_FR_BUSY)

#define UART_CR_CTSEN		0x8000	/* CTS hardware flow control */
#define UART_CR_RTSEN		0x4000	/* RTS hardware flow control */
#define UART_CR_OUT2		0x2000	/* OUT2 */
#define UART_CR_OUT1		0x1000	/* OUT1 */
#define UART_CR_RTS			0x0800	/* RTS */
#define UART_CR_DTR			0x0400	/* DTR */
#define UART_CR_RXE			0x0200	/* receive enable */
#define UART_CR_TXE			0x0100	/* transmit enable */
#define UART_CR_LBE			0x0080	/* loopback enable */
#define UART_CR_RTIE		0x0040
#define UART_CR_TIE 		0x0020
#define UART_CR_RIE 		0x0010
#define UART_CR_MSIE		0x0008
#define UART_CR_IIRLP		0x0004	/* SIR low power mode */
#define UART_CR_SIREN		0x0002	/* SIR enable */
#define UART_CR_UARTEN		0x0001	/* UART enable */
 
#define UART_LCRH_SPS		0x80
#define UART_LCRH_WLEN_8	0x60
#define UART_LCRH_WLEN_7	0x40
#define UART_LCRH_WLEN_6	0x20
#define UART_LCRH_WLEN_5	0x00
#define UART_LCRH_FEN		0x10
#define UART_LCRH_STP2		0x08
#define UART_LCRH_EPS		0x04
#define UART_LCRH_PEN		0x02
#define UART_LCRH_BRK		0x01


#define UART_IFLS_RX1_8	(0 << 3)
#define UART_IFLS_RX2_8	(1 << 3)
#define UART_IFLS_RX4_8	(2 << 3)
#define UART_IFLS_RX6_8	(3 << 3)
#define UART_IFLS_RX7_8	(4 << 3)
#define UART_IFLS_TX1_8	(0 << 0)
#define UART_IFLS_TX2_8	(1 << 0)
#define UART_IFLS_TX4_8	(2 << 0)
#define UART_IFLS_TX6_8	(3 << 0)
#define UART_IFLS_TX7_8	(4 << 0)

#define UART_OEIM		(1 << 10)	/* overrun error interrupt mask */
#define UART_BEIM		(1 << 9)	/* break error interrupt mask */
#define UART_PEIM		(1 << 8)	/* parity error interrupt mask */
#define UART_FEIM		(1 << 7)	/* framing error interrupt mask */
#define UART_RTIM		(1 << 6)	/* receive timeout interrupt mask */
#define UART_TXIM		(1 << 5)	/* transmit interrupt mask */
#define UART_RXIM		(1 << 4)	/* receive interrupt mask */
#define UART_DSRMIM		(1 << 3)	/* DSR interrupt mask */
#define UART_DCDMIM		(1 << 2)	/* DCD interrupt mask */
#define UART_CTSMIM		(1 << 1)	/* CTS interrupt mask */
#define UART_RIMIM		(1 << 0)	/* RI interrupt mask */

#define UART_OEIS		(1 << 10)	/* overrun error interrupt status */
#define UART_BEIS		(1 << 9)	/* break error interrupt status */
#define UART_PEIS		(1 << 8)	/* parity error interrupt status */
#define UART_FEIS		(1 << 7)	/* framing error interrupt status */
#define UART_RTIS		(1 << 6)	/* receive timeout interrupt status */
#define UART_TXIS		(1 << 5)	/* transmit interrupt status */
#define UART_RXIS		(1 << 4)	/* receive interrupt status */
#define UART_DSRMIS		(1 << 3)	/* DSR interrupt status */
#define UART_DCDMIS		(1 << 2)	/* DCD interrupt status */
#define UART_CTSMIS		(1 << 1)	/* CTS interrupt status */
#define UART_RIMIS		(1 << 0)	/* RI interrupt status */


#define UART485_RBR_THR_DLL              0x000
#define UART485_DLH_IER                  0x004
#define UART485_IIR_FCR                  0x008
#define UART485_RBR                      0x000
#define UART485_THR                      0x000
#define UART485_DLL                      0x000
#define UART485_DLH                      0x004
#define UART485_IER                      0x004
#define UART485_IIR                      0x008
#define UART485_FCR                      0x008
#define UART485_LCR                      0x00C
#define UART485_MCR                      0x010
#define UART485_LSR                      0x014
#define UART485_MSR                      0x018
#define UART485_SCR                      0x01C
#define UART485_LPDLL                    0x020
#define UART485_LPDLH                    0x024
#define UART485_RESERVED                 0x028
#define UART485_SRBR                     0x030
#define UART485_STHR                     0x06C
#define UART485_FAR                      0x070
#define UART485_TFR                      0x074
#define UART485_RFW                      0x078
#define UART485_USR                      0x07C
#define UART485_TFL                      0x080
#define UART485_RFL                      0x084
#define UART485_SRR                      0x088
#define UART485_SRTS                     0x08C
#define UART485_SBCR                     0x090
#define UART485_SDMAM                    0x094
#define UART485_SFE                      0x098
#define UART485_SRT                      0x09C
#define UART485_STET                     0x0A0
#define UART485_HTX                      0x0A4
#define UART485_DMASA                    0x0A8
#define UART485_TCR                      0x0AC
#define UART485_DE_EN                    0x0B0
#define UART485_RE_EN                    0x0B4
#define UART485_DET                      0x0B8
#define UART485_TAT                      0x0BC
#define UART485_DLF                      0x0C0
#define UART485_RAR                      0x0C4
#define UART485_TAR                      0x0C8
#define UART485_LCR_EXT                  0x0CC
#define UART485_CPR                      0x0F4
#define UART485_UCV                      0x0F8
#define UART485_CTR                      0x0FC

#define UART485_LSR_RFE					(1 << 7)
#define UART485_LSR_TEMT				(1 << 6)
#define UART485_LSR_THRE				(1 << 5)
#define UART485_LSR_BI					(1 << 4)
#define UART485_LSR_FE					(1 << 3)
#define UART485_LSR_PE					(1 << 2)
#define UART485_LSR_OE					(1 << 1)
#define UART485_LSR_DR					(1 << 0)

#define UART485_IIR_IID_MASK			(0xf << 0)
#define UART485_IIR_IID_MODEM_STATUS		0x0
#define UART485_IIR_IID_NO_INT_PENDING		0x1
#define UART485_IIR_IID_THR_EMPTY			0x2
#define UART485_IIR_IID_REV_DATA_AVAIL		0x4
#define UART485_IIR_IID_REV_LINE_STATUS		0x6
#define UART485_IIR_IID_BUSY_DETECT			0x7
#define UART485_IIR_IID_CHAR_TIMEOUT		0xc


#define CSIZE	0x0003
#define   CS8	0x0000
#define   CS6	0x0001
#define   CS7	0x0002
#define   CS5	0x0003
#define CSTOPB	0x0004
#define PARENB	0x0010
#define PARODD	0x0020
#define CMSPAR	0x0100	/* mark or space (stick) parity */
#define CRTSCTS	0x0200	/* flow control */

#define UART_CLK			24000000
#define UART_DEBUG_PORT		UART_ID0

#define UART_BUF_SIZE	4096
#define UART_ISR_PASS_LIMIT	16

#define UART_BLOCK_TIMEOUT		pdMS_TO_TICKS(100)
#define UART_NO_BLOCK			( ( TickType_t ) 0 )

static uint32_t ulUartBase[UART_NUM] = {REGS_UART0_BASE, REGS_UART1_BASE, REGS_UART2_BASE, REGS_UART3_BASE};
static UartPort_t *pxUartPort[UART_NUM] = {NULL};

#define uart_circ_empty(circ)		((circ)->head == (circ)->tail)
#define uart_circ_clear(circ)		((circ)->head = (circ)->tail = 0)

#define uart_circ_chars_pending(circ)	\
	(CIRC_CNT((circ)->head, (circ)->tail, UART_XMIT_SIZE))

#define uart_circ_chars_free(circ)	\
	(CIRC_SPACE((circ)->head, (circ)->tail, UART_XMIT_SIZE))


/* static void vUartSlectPad(uint32_t id)
{
	switch (id) {
	case UART_ID0:
		vSysctlConfigure(SYS_PAD_CTRL02, 12, 0xf, 5);
		break;
	case UART_ID1:
		vSysctlConfigure(SYS_PAD_CTRL02, 16, 0xf, 5);
		break;
	case UART_ID2:
		vSysctlConfigure(SYS_PAD_CTRL02, 20, 0xf, 5);
		break;
	case UART_ID3:
		vSysctlConfigure(SYS_PAD_CTRL02, 24, 0xf, 5);
		break;
	}
} */

static int iUartRxChars(UartPort_t *uap)
{
	uint32_t status;
	unsigned int ch, max_count = 256;
	int fifotaken = 0;

	while (max_count--) {
		status = readl(uap->regbase + UART_FR);
		if (status & UART_FR_RXFE)
			break;

		/* Take chars from the FIFO and update status */
		ch = readl(uap->regbase + UART_DR);
		fifotaken++;

		/* discard when rx buf is full */
		if (CIRC_SPACE_TO_END(uap->rxbuf.head, uap->rxbuf.tail, UART_BUF_SIZE) > 0) {
			*(uap->rxbuf.buf + uap->rxbuf.head) = ch;
			uap->rxbuf.head = (uap->rxbuf.head + 1) & (UART_BUF_SIZE - 1);
		} else {
			;//printf("uart%d rx buf is full, discard a data.\n", uap->id);
		}
	}

	if (fifotaken > 0)
		xSemaphoreGiveFromISR(uap->xRev, NULL);

	return fifotaken;

}


static void vUartStopTx(UartPort_t *uap)
{
	writel(readl(uap->regbase + UART_IMSC) & ~UART_TXIM, uap->regbase + UART_IMSC);
}

static void vUartStartTx(UartPort_t *uap)
{
	writel(readl(uap->regbase + UART_IMSC) | UART_TXIM, uap->regbase + UART_IMSC);
}

 static BaseType_t xUartTxChar(UartPort_t *uap, unsigned char c, int from_irq)
{
	if (!from_irq && (readl(uap->regbase + UART_FR) & UART_FR_TXFF))
		return pdFALSE; /* unable to transmit character */

	writel(c, uap->regbase + UART_DR);

	return pdTRUE;
}

/* Returns true if tx interrupts have to be (kept) enabled  */
static BaseType_t xUartTxChars(UartPort_t *uap, int from_irq)
{
	struct circ_buf *xmit = &uap->txbuf;
	int count = uap->fifosize >> 1;

	if (uart_circ_empty(xmit)) {
		vUartStopTx(uap);
		return pdFALSE;
	}

	do {
		if (from_irq && count-- == 0)
			break;

		if (!xUartTxChar(uap, xmit->buf[xmit->tail], from_irq))
			break;

		xmit->tail = (xmit->tail + 1) & (UART_BUF_SIZE - 1);
	} while (!uart_circ_empty(xmit));

	if (from_irq)
		xSemaphoreGiveFromISR(uap->xSend, NULL);

	if (uart_circ_empty(xmit)) {
		vUartStopTx(uap);
		return pdFALSE;
	}

	return pdTRUE;
}

static void vUartIntHandler(void *param)
{
	UartPort_t *uap = param;
	uint32_t status;
	uint32_t imsc;
	uint32_t pass_counter = UART_ISR_PASS_LIMIT;
		
	imsc = readl(uap->regbase + UART_IMSC);
	status = readl(uap->regbase + UART_RIS) & imsc;
	if (status) {
		do {
			writel(status & ~(UART_TXIS | UART_RTIS | UART_RXIS),
				    uap->regbase + UART_ICR); 

			if (status & (UART_RTIS | UART_RXIS)) {
				iUartRxChars(uap);
			}
			if (status & UART_TXIS)
				xUartTxChars(uap, pdTRUE);

			if (pass_counter-- == 0)
				break;

			status = readl(uap->regbase + UART_RIS) & imsc;
		} while (status != 0);
	}
}

static void vUart485StopTx(UartPort_t *uap)
{
	writel(readl(uap->regbase + UART485_IER) & ~(1 << 1), uap->regbase + UART485_IER);
}

static void vUart485StartTx(UartPort_t *uap)
{
	writel(readl(uap->regbase + UART485_IER) | (1 << 1), uap->regbase + UART485_IER);
}

static int iUart485RxChars(UartPort_t *uap)
{
	uint32_t lsr;
	unsigned int ch, max_count = 256;
	int fifotaken = 0;

	while (max_count--) {
		lsr = readl(uap->regbase + UART485_LSR);
		if (!(lsr & UART485_LSR_DR))
			break;

		/* Take chars from the FIFO and update status */
		ch = readl(uap->regbase + UART485_RBR);
		fifotaken++;

		/* discard when rx buf is full */
		if (CIRC_SPACE_TO_END(uap->rxbuf.head, uap->rxbuf.tail, UART_BUF_SIZE) > 0) {
			*(uap->rxbuf.buf + uap->rxbuf.head) = ch;
			uap->rxbuf.head = (uap->rxbuf.head + 1) & (UART_BUF_SIZE - 1);
		} else {
			//printf("uart%d rx buf is full, discard a data.\n", uap->id);
		}
	}

	if (fifotaken > 0)
		xSemaphoreGiveFromISR(uap->xRev, NULL);

	return fifotaken;
}

/* Returns true if tx interrupts have to be (kept) enabled  */
static BaseType_t xUart485TxChars(UartPort_t *uap, int from_irq)
{
	struct circ_buf *xmit = &uap->txbuf;
	int count = uap->fifosize >> 1;

	if (uart_circ_empty(xmit)) {
		vUart485StopTx(uap);
		return pdFALSE;
	}

	do {
		if (from_irq && count-- == 0)
			break;
		writel(xmit->buf[xmit->tail], uap->regbase + UART485_THR);
		xmit->tail = (xmit->tail + 1) & (UART_BUF_SIZE - 1);
	} while (!uart_circ_empty(xmit));

	if (from_irq)
		xSemaphoreGiveFromISR(uap->xSend, NULL);

	return pdTRUE;
}

static void vUart485IntHandler(void *param)
{
	UartPort_t *uap = param;
	uint32_t iir;
		
	iir = readl(uap->regbase + UART485_IIR);
	switch (iir & UART485_IIR_IID_MASK) {
	case UART485_IIR_IID_THR_EMPTY:
		xUart485TxChars(uap, 1);
		break;
	case UART485_IIR_IID_REV_DATA_AVAIL:
	case UART485_IIR_IID_REV_LINE_STATUS:
	case UART485_IIR_IID_CHAR_TIMEOUT:
		iUart485RxChars(uap);
		break;
	case UART485_IIR_IID_BUSY_DETECT:
		iir = readl(uap->regbase + UART485_USR); //clear busy interrupt
		break;
	}
}

UartPort_t *xUartOpen(uint32_t id)
{
	int irq;
	
	if (id >= UART_NUM) {
		TRACE_INFO("Wrong input uart id.\n");
		return NULL;
	}

	if (pxUartPort[id] != NULL) {
		if (id != UART_DEBUG_PORT)
			TRACE_ERROR("Uart %d is already in use.\n", id);
		return NULL;
	}
	
	UartPort_t *uap = (UartPort_t *)pvPortMalloc(sizeof(UartPort_t));
	if (uap == NULL) {
		if (id != UART_DEBUG_PORT)
			TRACE_ERROR("Out of memory for uart%d.\n", id);
		return NULL;
	}
	memset(uap, 0, sizeof(*uap));

	uap->rxbuf.buf = (char*)pvPortMalloc(UART_BUF_SIZE);
	if (uap->rxbuf.buf == NULL) {
		if (id != UART_DEBUG_PORT)
			TRACE_ERROR("Out of memory for uart%d.\n", id);
		goto err;
	}

	uap->txbuf.buf = (char*)pvPortMalloc(UART_BUF_SIZE);
	if (uap->txbuf.buf == NULL) {
		if (id != UART_DEBUG_PORT)
			TRACE_ERROR("Out of memory for uart%d.\n", id);
		goto err;
	}

	uap->xMutex = xSemaphoreCreateRecursiveMutex();
	configASSERT(uap->xMutex);

	uap->xRev = xSemaphoreCreateBinary();
	configASSERT(uap->xRev);

	uap->xSend = xSemaphoreCreateBinary();
	configASSERT(uap->xSend);

	//vUartSlectPad(id);
	uap->id = id;
	uap->regbase = ulUartBase[id];
	if ((uap->id == UART_ID0) || (uap->id == UART_ID2))
		uap->fifosize = 16;
	else if (uap->id == UART_ID1)
		uap->fifosize = 32;
	else
		uap->fifosize = 128;
	
	irq = UART0_IRQn + id;
	if ((uap->id == UART_ID0) || (uap->id == UART_ID2))
		request_irq(irq, 0, vUartIntHandler, uap);
	else
		request_irq(irq, 0, vUart485IntHandler, uap);

	return pxUartPort[id] = uap;

err:
	if (uap->txbuf.buf)
		vPortFree(uap->txbuf.buf);
	if (uap->rxbuf.buf)
		vPortFree(uap->rxbuf.buf);
	vPortFree(uap);
	return NULL;
};

void vUartInit(UartPort_t *uap, uint32_t baud, uint32_t flags)
{
	unsigned int lcr_h, quot, cr;
	int count = 0;

	/* soft reset */
	sys_soft_reset(softreset_uart0 + uap->id - UART_ID0);

	if ((uap->id == UART_ID0) || (uap->id == UART_ID2)) {
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
		writel(cr, uap->regbase + UART_CR);
		writel(0, uap->regbase + UART_FBRD);
		writel(1, uap->regbase + UART_IBRD);
		writel(0, uap->regbase + UART_LCRH);
		writel(0, uap->regbase + UART_DR);
		while (readl(uap->regbase + UART_FR) & UART_FR_BUSY);
		if (count++ < 10 && !(readl(uap->regbase + UART_RIS) & UART_TXIS)) {
			printf("ERROR! Uart status wrong.\n");
			goto reconfig;
		}

		/* first, disable everything */
		writel(0, uap->regbase + UART_CR);
		
		cr = UART_CR_RXE | UART_CR_TXE;
		if (flags & CRTSCTS)
			cr |= UART_CR_CTSEN | UART_CR_RTSEN;

		/* Set baud rate */
		writel(quot & 0x3f, uap->regbase + UART_FBRD);
		writel(quot >> 6, uap->regbase + UART_IBRD);

		writel(UART_RTIM | UART_RXIM, uap->regbase + UART_IMSC);
		writel(UART_IFLS_RX4_8 | UART_IFLS_TX4_8, uap->regbase + UART_IFLS);
		writel(lcr_h, uap->regbase + UART_LCRH);
		writel(cr | UART_CR_UARTEN, uap->regbase + UART_CR);
	} else {
		//set modem
		writel(0, uap->regbase + UART485_MCR);

		// baud = clk/(16*U_DLL)
		//set baud rate
		writel(readl(uap->regbase + UART485_LCR) | (1 << 7), uap->regbase + UART485_LCR);
		writel(UART_CLK / (16 * baud), uap->regbase + UART485_DLL);
		writel(0, uap->regbase + UART485_DLH);
		if ((uap->id == UART_ID1) || (uap->id == UART_ID3)) {		//Set fractional baud rate
			//Fractional baud divisor = ((uart_clk%(16*baud))/(16*baud)) * 16 = (uart_clk%(16*baud)) * 16 / (16*baud);
			u32 divisor = (UART_CLK % (16*baud)) * 16 / (16*baud);
			printf("fractional divisor:%d\n", divisor);
			writel(divisor, uap->regbase + UART485_DLF);
		}
		writel(readl(uap->regbase + UART485_LCR) & ~(1 << 7), uap->regbase + UART485_LCR);

		cr = readl(uap->regbase + UART485_LCR);
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
		writel(cr, uap->regbase + UART485_LCR);

		//set fifo
		writel((2 << 6) | (3 << 4) | 7, uap->regbase + UART485_FCR);
		
		//enable rx and err interrupt
		writel((1 << 4) | 1, uap->regbase + UART485_IER);

		//设置通信模式全双工还是半双工
		//485mode disable
		writel(0, uap->regbase + UART485_TCR);
	}

	/* 高速串口先初始化再配置pad脚，否则在初始化过程中有收到数据会导致串口出错 */
	if (uap->id == UART_ID1)
		pinctrl_set_group(PGRP_UART1);
	else if (uap->id == UART_ID3)
		pinctrl_set_group(PGRP_UART3);
}

void vUartClose(UartPort_t *uap)
{
	if (uap == NULL) return;

	if ((uap->id == UART_ID0) || (uap->id == UART_ID2)) {
		writel(0, uap->regbase + UART_IMSC);
		writel(0, uap->regbase + UART_CR);
	} else {

	}

	vSemaphoreDelete(uap->xSend);
	vSemaphoreDelete(uap->xRev);
	vSemaphoreDelete(uap->xMutex);
	vPortFree(uap->rxbuf.buf);
	uap->rxbuf.tail = uap->rxbuf.head =  0;
	vPortFree(uap->txbuf.buf);
	uap->rxbuf.tail = uap->rxbuf.head =  0;
	vPortFree(uap);
	pxUartPort[uap->id] = NULL;
}

int iUartWrite(UartPort_t *uap, uint8_t *buf, size_t len, TickType_t xBlockTime)
{
	int c, ret = 0;
	TickType_t timeout = xBlockTime;
	TickType_t starttime = xTaskGetTickCount();
	TickType_t sptime;

	while (len) {
		xSemaphoreTakeRecursive(uap->xMutex, portMAX_DELAY);
		
		c = CIRC_SPACE_TO_END(uap->txbuf.head, uap->txbuf.tail, UART_BUF_SIZE);
		if (len < c)
			c = len;
		if (c <= 0) {
			xSemaphoreGiveRecursive(uap->xMutex);
			sptime = xTaskGetTickCount() - starttime;
			if (timeout > sptime) {
				xSemaphoreTake(uap->xSend, timeout - sptime);
				continue;
			} else {
				break;
			}
		}
		memcpy(uap->txbuf.buf + uap->txbuf.head, buf, c);
		uap->txbuf.head = (uap->txbuf.head + c) & (UART_BUF_SIZE - 1);
		buf += c;
		len -= c;
		ret += c;
		
		xSemaphoreGiveRecursive(uap->xMutex);
	}

	if ((uap->id == UART_ID0) || (uap->id == UART_ID2))
		vUartStartTx(uap);
	else
		vUart485StartTx(uap);

	return ret;
}

int iUartReadNoBlock(UartPort_t *uap, uint8_t *buf, size_t len)
{
    int c;

	xSemaphoreTakeRecursive(uap->xMutex, portMAX_DELAY);

	c = CIRC_CNT_TO_END(uap->rxbuf.head, uap->rxbuf.tail, UART_BUF_SIZE);
	if (len < c)
		c = len;
	if (c <= 0) {
		xSemaphoreGiveRecursive(uap->xMutex);
	}
	memcpy(buf, uap->rxbuf.buf + uap->rxbuf.tail, c);
	uap->rxbuf.tail = (uap->rxbuf.tail + c) & (UART_BUF_SIZE - 1);
	xSemaphoreGiveRecursive(uap->xMutex);

	return c;
}

int iUartRead(UartPort_t *uap, uint8_t *buf, size_t len, TickType_t xBlockTime)
{
	int c, ret = 0;
	TickType_t timeout = xBlockTime;
	TickType_t starttime = xTaskGetTickCount();
	TickType_t sptime;

	while (len) {
		xSemaphoreTakeRecursive(uap->xMutex, portMAX_DELAY);

		c = CIRC_CNT_TO_END(uap->rxbuf.head, uap->rxbuf.tail, UART_BUF_SIZE);
		if (len < c)
			c = len;
		if (c <= 0) {
			xSemaphoreGiveRecursive(uap->xMutex);
			sptime = xTaskGetTickCount() - starttime;
			if (timeout > sptime) {
				xSemaphoreTake(uap->xRev, timeout - sptime);
				continue;
			} else {
				break;
			}
		}
		memcpy(buf, uap->rxbuf.buf + uap->rxbuf.tail, c);
		uap->rxbuf.tail = (uap->rxbuf.tail + c) & (UART_BUF_SIZE - 1);
		buf += c;
		len -= c;
		ret += c;
		
		xSemaphoreGiveRecursive(uap->xMutex);
	}

	return ret;
}

void vDebugConsoleInitialise(void)
{
	UartPort_t *uap = xUartOpen(UART_DEBUG_PORT);

	if (uap != NULL) {

		vUartInit(uap, 115200, 0);
	}
}

/* retarget printf function */
int32_t putchar(int32_t ch)
{
	if (get_system_log() >= 1) {
		while (readl(REGS_UART0_BASE + UART_FR) & UART_FR_TXFF);
			writel(ch, REGS_UART0_BASE + UART_DR);
		
		return ch;
	} else {
		return 0;
	}
}

xComPortHandle xSerialPortInitMinimal( unsigned long ulWantedBaud, unsigned portBASE_TYPE uxQueueLength )
{
	return pxUartPort[UART_DEBUG_PORT];
}

signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed char cOutChar, TickType_t xBlockTime )
{
	if (iUartWrite(pxPort, (uint8_t *)&cOutChar, 1, xBlockTime) == 1)
		return pdPASS;

	return pdFAIL;
}

void vSerialPutString( xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength )
{
	signed char *pxNext;
	
	/* NOTE: This implementation does not handle the queue being full as no
	block time is used! */

	/* The port handle is not required as this driver only supports UART0. */
	( void ) pxPort;
	( void ) usStringLength;

	/* Send each character in the string, one at a time. */
	pxNext = ( signed char * ) pcString;
	while( *pxNext )
	{
		xSerialPutChar( pxPort, *pxNext, UART_NO_BLOCK );
		pxNext++;
	}
}

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed char *pcRxedChar, TickType_t xBlockTime )
{
	if (iUartRead(pxPort, (uint8_t *)pcRxedChar, 1, xBlockTime) == 1)
		return pdPASS;

	return pdFAIL;
}


#define UUP_PACKET_SIZE		128
#define UUP_MAX_FRAME_LEN	(UUP_PACKET_SIZE + 16)
#define UUP_RX_FRAME_NUM	16
static unsigned char uup_rx_buf[UUP_RX_FRAME_NUM][UUP_MAX_FRAME_LEN];
static unsigned char *uup_rx_ptr;
static int uup_rx_rev_len = 0;
static int uup_rx_head = 0;
static int uup_rx_tail = 0;
static int uup_rx_state = 0;
static int uup_rx_data_len = 0;
uint32_t checksum, calc_checksum = 0xffffffff;
SysInfo uup_bak_sysinfo;

#ifdef OTA_UPDATE_SUPPORT
#include "ota_update.h"
#include "updatefile.h"
#include "romfile.h"
#include "animation.h"

typedef enum {
	UART_FRAME_START,
	UART_FRAME_FILEINFO,
	UART_FRAME_FILEXFER,
	UART_FRAME_FINISH,
} eUartFrameType;

typedef enum {
	UUP_STATE_IDLE,
	UUP_STATE_START,
	UUP_STATE_GET_FILEINFO,
	UUP_STATE_FILE_TFR,
	UUP_STATE_END,
} eUartUpdateState;

#define UUP_ACK_OK				1
#define UUP_ACK_FAIL			0
#define UUP_MAX_FILE_SIZE		0x1000000
#define UUP_RX_FRAME_NUM		16
#define UUP_MAX_LOADER_SIZE		STEPLDR_MAX_SIZE//0x10000
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
#define UART_UPATE_RAW_SIZE		0x20000
#else
#define UART_UPATE_RAW_SIZE		0x10000
#endif

static int uup_status = UUP_STATE_IDLE;
static int uup_file_type = 0;
static int uup_file_size = 0;
static int uup_packet_num = 0;
static int uup_rev_packet = 0;
static int uup_rev_len = 0;
static int uup_file_patch = 0;
//static char uup_filename[32];
//static FF_FILE *uup_file = NULL;
static int upfile_offset=0;
static unsigned char uup_buf[UART_UPATE_RAW_SIZE];
static unsigned int uup_buf_len = 0;

static void uup_send_ack(UartPort_t *uap, int type, int ret)
{
	unsigned char buf[7] = {0x55, 0x80, 0xc5, 0x02, 0x00, 0x00, 0x00};
	int i;

	buf[4] = type;
	buf[5] = ret;
	for (i = 1; i < 6; i++)
		buf[6] ^= buf[i];

	iUartWrite(uap, buf, 7, pdMS_TO_TICKS(100));
}

static void uup_ota_update(UartPort_t *uap, uint8_t *framebuf, size_t len)
{
	int frametype = framebuf[0];
	uint8_t *buf = framebuf + 1;
	unsigned int framelen;
	unsigned int packetnum;
	unsigned int filesize=0;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_flash *sflash = sfud_get_device(0);
#endif
	SysInfo *sysinfo = GetSysInfo();

	switch (frametype) {
		case UART_FRAME_START: {
			memcpy(&uup_bak_sysinfo, sysinfo, sizeof(SysInfo));
			uup_send_ack(uap, frametype, UUP_ACK_OK);
			uup_status = UUP_STATE_START;
			break;
		}
		case UART_FRAME_FILEINFO: {
			if (uup_status != UUP_STATE_START && uup_status != UUP_STATE_GET_FILEINFO) {
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			uup_rev_len = 0;
			uup_rev_packet = 0;
			uup_file_type = buf[0];

			if ((uup_file_type&0xf0)==0x40)
			{
				uup_file_patch = 1;
				uup_file_type &= 0x0f;
#ifndef DELTA_UPDATE_SUPPORT
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
#endif
			}
			else
			{
	        	uup_file_patch = 0;	
			}

			if (uup_file_type > UPFILE_TYPE_LNCHEMMC) {
				printf("Rev wrong file type %d.\n", uup_file_type);
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			uup_packet_num = (buf[1] << 16) | (buf[2] << 8) | buf[3];
			uup_file_size = UUP_PACKET_SIZE * uup_packet_num;
			if (uup_file_size > UUP_MAX_FILE_SIZE) {
				printf("Rev wrong file size.\n");
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			//不是完成的升级包，先从原来那份读出来写到另外一份
			if ((uup_file_patch == 0)&&(uup_file_type >= UPFILE_TYPE_RESOURCE )&&(uup_file_type <= UPFILE_TYPE_APP)) {
				if (uup_file_size > get_subfile_maxsize(uup_file_type)) {
					printf("Not have enough space to update subfile %x.\n", uup_file_type);
					uup_send_ack(uap, frametype, UUP_ACK_FAIL);
					break;
				}
				if (backup_whole_image()) {
					printf("backup_whole_image fail.\n");
					uup_send_ack(uap, frametype, UUP_ACK_FAIL);
					break;
				}
			}
#ifdef DELTA_UPDATE_SUPPORT
			if (uup_file_patch) {
				upfile_offset = OTA_MEDIA_OFFSET;
			} else
#endif
				{
				upfile_offset = get_upfile_offset(uup_file_type, 1);
				printf("upfile_offset is %x.\n", upfile_offset);
				if (upfile_offset == 0xffffffff) {
					printf("get_upfile_offset fail.\n");
					uup_send_ack(uap, frametype, UUP_ACK_FAIL);
					break;
				}
			}
			uup_send_ack(uap, frametype, UUP_ACK_OK);
			uup_status = UUP_STATE_GET_FILEINFO;
			break;
		}
		case UART_FRAME_FILEXFER: {
			if (uup_status != UUP_STATE_GET_FILEINFO && uup_status != UUP_STATE_FILE_TFR) {
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
#if 1
			if ((uup_rev_packet == 0)&&(uup_file_patch == 0)) {
				if (uup_file_type == UPFILE_TYPE_WHOLE) {
					UpFileHeader *header = (UpFileHeader *)&buf[1];
					if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
						printf("Wrong whole file magic.\n");
						uup_send_ack(uap, frametype, UUP_ACK_FAIL);
						break;
					}
					checksum = header->checksum;
					sysinfo->app_size = header->files[0].size;
				} else if (uup_file_type == UPFILE_TYPE_RESOURCE) {
					RomHeader *header = (RomHeader *)&buf[1];
					if (header->magic != MKTAG('R', 'O', 'M', 'A')) {
						printf("Wrong resource file magic.\n");
						uup_send_ack(uap, frametype, UUP_ACK_FAIL);
						break;
					}
					checksum = header->checksum;
				} else if (uup_file_type == UPFILE_TYPE_ANIMATION) {
					BANIHEADER *header = (BANIHEADER *)&buf[1];
					if (header->magic != MKTAG('B', 'A', 'N', 'I')) {
						printf("Wrong animation file magic.\n");
						uup_send_ack(uap, frametype, UUP_ACK_FAIL);
						break;
					}
					checksum = header->checksum;
				} else if (uup_file_type == UPFILE_TYPE_APP) {
					unsigned int magic = buf[1] | (buf[2] << 8) | (buf[3] << 16) | (buf[4] << 24);
					if (magic != UPFILE_APP_MAGIC) {
						printf("Wrong app file magic.\n");
						uup_send_ack(uap, frametype, UUP_ACK_FAIL);
						break;
					}
					unsigned char *tmp = buf + 1 + APPLDR_CHECKSUM_OFFSET;
					checksum = tmp[0] | (tmp[1] <<8) | (tmp[2] << 16) | (tmp[3] << 24);
				} else if (uup_file_type == UPFILE_TYPE_STEPLDR ){
					unsigned char *tmp = buf + 1 + APPLDR_CHECKSUM_OFFSET;
					checksum = tmp[0] | (tmp[1] <<8) | (tmp[2] << 16) | (tmp[3] << 24);
				} else if (uup_file_type == UPFILE_TYPE_FIRSTLDR){
					unsigned int *tmp = (unsigned int *)(buf + 1 + APPLDR_CHECKSUM_OFFSET);
					checksum =* tmp;
					*tmp = 0;
				}
			}
#endif
			packetnum = buf[0];
			printf("uup_rev_packet %d.\n", uup_rev_packet);
			if ((uup_rev_packet & 0xff) != packetnum) {
				printf("Wrong packet number.\n");
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			framelen = len - 2;
			/* only last frame size is less than UUP_PACKET_SIZE */
			if (framelen > UUP_PACKET_SIZE ||
				(framelen < UUP_PACKET_SIZE && uup_rev_packet != uup_packet_num - 1)) {
				printf("Wrong packet len.\n");
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			uup_rev_len += framelen;
			if (uup_file_type >= UPFILE_TYPE_FIRSTLDR && uup_rev_len > UUP_MAX_LOADER_SIZE) {
				printf("loader file is too large.\n");
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			uup_rev_packet++;

			memcpy(uup_buf + uup_buf_len, buf + 1, framelen);
			uup_buf_len += framelen;
			if ((uup_buf_len == UART_UPATE_RAW_SIZE) && (uup_file_type != UPFILE_TYPE_FIRSTLDR)) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				sfud_erase_write(sflash, upfile_offset, uup_buf_len, uup_buf);
#else
				emmc_write(upfile_offset, uup_buf_len, uup_buf);
#endif
				upfile_offset += uup_buf_len;
				uup_buf_len = 0;
			}
			uup_send_ack(uap, frametype, UUP_ACK_OK);
			uup_status = UUP_STATE_FILE_TFR;
			break;
		}
		case UART_FRAME_FINISH: {
			if (uup_status != UUP_STATE_FILE_TFR && uup_status != UART_FRAME_FINISH) {
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				break;
			}
			if (!buf[0]) {
				printf("update end with error!\n");
				uup_send_ack(uap, frametype, UUP_ACK_FAIL);
				uup_status = UUP_STATE_END;
				break;
			}

			filesize = uup_rev_len;

			if (uup_buf_len) {
				if (uup_file_type != UPFILE_TYPE_FIRSTLDR) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
					sfud_erase_write(sflash, upfile_offset, uup_buf_len, uup_buf);
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
					sfud_flush(sflash);
#endif
#else
					emmc_write(upfile_offset, uup_buf_len, uup_buf);
#endif
				} else {
					calc_checksum = xcrc32(uup_buf, filesize, calc_checksum);
					upfile_offset = 0;

					if (calc_checksum == checksum) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
						sfud_erase_write(sflash, upfile_offset, uup_buf_len, uup_buf);
#else
						emmc_write(upfile_offset, uup_buf_len, uup_buf);
#endif
						printf("UPFILE_TYPE_FIRSTLDR update ok\n\r");
						uup_send_ack(uap, frametype, UUP_ACK_OK);
						uup_status = UUP_STATE_END;
						sysinfo->loader_size = filesize;
						SaveSysInfo();
						wdt_cpu_reboot();
					} else {
						uup_send_ack(uap, frametype, UUP_ACK_FAIL);
						uup_status = UUP_STATE_END;
						break;
					}
				}
			}
#ifdef DELTA_UPDATE_SUPPORT
			if (uup_file_patch) {
				uup_send_ack(uap, frametype, UUP_ACK_OK);
				uup_status = UUP_STATE_END;
				delta_update(uup_file_type, filesize);
			} 
			else
#endif
			{
				printf("checksum after burn...\n");
				uup_send_ack(uap, frametype, UUP_ACK_OK);
				uup_status = UUP_STATE_END;
				printf("filesize is %x\n", filesize);
				printf("checksum is %x\n", checksum);

				if (checksum == get_upfile_checksum(uup_file_type, filesize, 1, 1)) {
					if (uup_file_type >= UPFILE_TYPE_RESOURCE && uup_file_type <= UPFILE_TYPE_APP) {
						update_part_upfile_info(sysinfo, uup_file_type, filesize);
						uup_bak_sysinfo.app_checksum = get_upfile_checksum(UPFILE_TYPE_WHOLE, 0, 0, 1);
						if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
							uup_bak_sysinfo.image_offset = UPDATEFILE_MEDIA_B_OFFSET;
						else
							uup_bak_sysinfo.image_offset = UPDATEFILE_MEDIA_OFFSET;

						if (uup_file_type == UPFILE_TYPE_APP) {
							uup_bak_sysinfo.app_size = filesize;	
						}
					} else if (uup_file_type == UPFILE_TYPE_WHOLE) {	
						UpFileHeader *header;
						uint32_t headersize;

						headersize = sizeof(UpFileHeader) + sizeof(UpFileInfo);
						header = pvPortMalloc(headersize);
						if (!header) {
							printf("header malloc fail.\n");
							break;
						}
						if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
							uup_bak_sysinfo.image_offset = UPDATEFILE_MEDIA_B_OFFSET;
						else
							uup_bak_sysinfo.image_offset = UPDATEFILE_MEDIA_OFFSET;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
						sfud_read(sflash, uup_bak_sysinfo.image_offset, headersize, (void*)header);
#else
						emmc_read(uup_bak_sysinfo.image_offset, headersize, (void*)header);
#endif
						if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
							printf("Error! Wrong update file.\n");
							vPortFree(header);
							break;
						}
						uup_bak_sysinfo.app_checksum = header->checksum;
						uup_bak_sysinfo.app_size = header->files[0].size;
						vPortFree(header);
					} else {
						set_upfile_offset(&uup_bak_sysinfo, uup_file_type, upfile_offset);
						if (uup_file_type == UPFILE_TYPE_STEPLDR) {
							uup_bak_sysinfo.stepldr_size = filesize;
						}
					}
					printf("burn ok.\n");
					memcpy(sysinfo, &uup_bak_sysinfo, sizeof(SysInfo));
					SaveSysInfo();
					wdt_cpu_reboot();
				} else {
					printf("checksum after burn fail.\n");
				}
			}
			break;
		}
	}
}
#else
static void uart_tx_demo_thread(void *param)
{
	UartPort_t *uap = param;
	uint8_t uarttx[32] = "hello, i am amt630h\n";

	//printf("### uart_tx_demo_thread\n");
	for (;;) {
		iUartWrite(uap, uarttx, strlen((char*)uarttx), pdMS_TO_TICKS(100));
		vTaskDelay(1000);
	}
}
#endif

static void uart_rx_demo_thread(void *param)
{
	UartPort_t *uap = xUartOpen(UART_MCU_PORT);
	uint8_t uartrx[16];
	int len;
	int i;

	if (!uap) {
		printf("open uart %d fail.\n", UART_MCU_PORT);
		vTaskDelete(NULL);
		return;
	}

	vUartInit(uap, 115200, 0);

#ifndef OTA_UPDATE_SUPPORT
	if (xTaskCreate(uart_tx_demo_thread, "uartsend", configMINIMAL_STACK_SIZE, uap,
		configMAX_PRIORITIES / 3, NULL) != pdPASS) {
		printf("create uart tx demo task fail.\n");
		vTaskDelete(NULL);
		return;
	}
#endif

	for (;;) {
		len = iUartRead(uap, uartrx, 16, pdMS_TO_TICKS(20));
		for (i = 0; i < len; i++) {
			switch (uup_rx_state) {
			case 0:
				if (uartrx[i] == 0x55) {
					uup_rx_state++;
					uup_rx_rev_len = 0;
					uup_rx_ptr = &uup_rx_buf[uup_rx_head][0];
				}
				break;
			case 1:
				if (uartrx[i] == 0x81)
					uup_rx_state++;
				else
					uup_rx_state = 0;
				*uup_rx_ptr++ = uartrx[i];
				break;
			case 2:
				if (uartrx[i] == 0xc6)
					uup_rx_state++;
				else
					uup_rx_state = 0;
				*uup_rx_ptr++ = uartrx[i];
				break;
			case 3:
				uup_rx_data_len = uartrx[i];
				if ((uup_rx_data_len <= 0) || (uup_rx_data_len > UUP_PACKET_SIZE + 2)) {
					printf("Invalid uup_rx_data_len %d\n", uup_rx_data_len);
					uup_rx_state = 0;
				} else {
					uup_rx_state++;
					*uup_rx_ptr++ = uartrx[i];
				}
				break;
			case 4:
				*uup_rx_ptr++ = uartrx[i];
				if (++uup_rx_rev_len == uup_rx_data_len)
					uup_rx_state++;
				break;
			case 5:
				*uup_rx_ptr++ = uartrx[i];
				uup_rx_head = (uup_rx_head + 1) % UUP_RX_FRAME_NUM;
				uup_rx_state = 0;
				break;
			}
		}

		if (uup_rx_tail != uup_rx_head) {
			unsigned char *buf;
			unsigned char checksum = 0;

			buf = &uup_rx_buf[uup_rx_tail][0];
			len = buf[2];
			for (i = 0; i < len + 3; i++)
				checksum ^= buf[i];
			if (checksum == buf[len + 3]) {
#ifdef OTA_UPDATE_SUPPORT
				uup_ota_update(uap, buf + 3, len);
#else
				if (buf[3] == 0) {
					SysInfo *sysinfo = GetSysInfo();
					printf("receive uart update cmd, updating...\n");
					sysinfo->update_media_type = UPDATE_MEDIA_UART;
					sysinfo->update_status = UPDATE_STATUS_START;
					SaveSysInfo();
					wdt_cpu_reboot();
				}
#endif
			} else {
				printf("rev frame checksum err.\n");
			}
			uup_rx_tail = (uup_rx_tail + 1) % UUP_RX_FRAME_NUM;
		}
	}
}

int uart_rx_demo(void)
{
	/* Create a task to process uart rx data */
	if (xTaskCreate(uart_rx_demo_thread, "uartdemo", configMINIMAL_STACK_SIZE*16, NULL,
		configMAX_PRIORITIES / 3, NULL) != pdPASS) {
		printf("create uart rx demo task fail.\n");
		return -1;
	}

	return 0;
}
