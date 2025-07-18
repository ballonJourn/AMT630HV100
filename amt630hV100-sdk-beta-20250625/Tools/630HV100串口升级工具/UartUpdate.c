#include "stdafx.h"
#include "binport_.h"
#include "UartUpdate.h"

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _CRT_SECURE_NO_WARNINGS
static int transferOver = 0;
HANDLE hComm;

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

#pragma pack(16)
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

static int nStop=0;

static DWORD WINAPI uartRevThread(LPVOID lpParam)
{
	unsigned char buf[UUP_MAX_FRAME_LEN];
	DWORD len;
	int i;
	
	while (!transferOver) {
		if (nStop==1)break;
		ReadFile(hComm, buf, UUP_MAX_FRAME_LEN, &len, NULL);
		
		for (i = 0; i < (int)len; i++) {
			if (nStop==1)break;
			switch (uup_rx_state) {
			case 0:
				if (buf[i] == 0x55) {
					uup_rx_state++;
					uup_rx_rev_len = 0;
					uup_rx_ptr = &uup_rx_buf[uup_rx_head][0];
				}
				break;
			case 1:
				if (buf[i] == 0x80)
					uup_rx_state++;
				else
					uup_rx_state = 0;
				*uup_rx_ptr++ = buf[i];
				break;
			case 2:
				if (buf[i] == 0xc5)
					uup_rx_state++;
				else
					uup_rx_state = 0;
				*uup_rx_ptr++ = buf[i];
				break;
			case 3:
				uup_rx_data_len = buf[i];
				uup_rx_state++;
				*uup_rx_ptr++ = buf[i];
				break;
			case 4:
				*uup_rx_ptr++ = buf[i];
				if (++uup_rx_rev_len == uup_rx_data_len)
					uup_rx_state++;
				break;
			case 5:
				*uup_rx_ptr++ = buf[i];
				uup_rx_head = (uup_rx_head + 1) % UUP_RX_FRAME_NUM;
				uup_rx_state = 0;
				break;
			}
		}
	}
	
	return 0;
}

static int uart_wait_ack(int ack_type, unsigned int timeout_ms)
{
	unsigned char *buf;
	int len;
	unsigned char checksum = 0;
	int i;
	DWORD tm = GetTickCount();

	while (GetTickCount() - tm < timeout_ms) {
		if (uup_rx_tail == uup_rx_head) {
			Sleep(1);
			continue;
		}
		buf = &uup_rx_buf[uup_rx_tail][0];
		uup_rx_tail = (uup_rx_tail + 1) % UUP_RX_FRAME_NUM;

		len = buf[2];
		if (len != 2) {
			printf("rev wrong ack len.\n");
			continue;
		}
		for (i = 0; i < len + 3; i++)
			checksum ^= buf[i];
		if (checksum == buf[len + 3]) {
			if (buf[3] != ack_type) {
				printf("rev wrong ack type.\n");
				continue;
			}
			return buf[4];
		}
		else {
			printf("rev ack checksum err.\n");
			continue;
		}
	}
	
	return 0;
}


static void uart_send_frame(unsigned char *data, int len)
{
	unsigned char buf[UUP_MAX_FRAME_LEN];
	DWORD wLen;
	
	buf[0] = 0x55;
	buf[1] = 0x81;
	buf[2] = 0xc6;
	buf[3] = len;
	memcpy(buf + 4, data, len);
	buf[len + 4] = 0;
	for (int i = 1; i < len + 4; i++)
		buf[len + 4] ^= buf[i];
	WriteFile(hComm, buf, len + 5, &wLen, NULL);
}

static void uart_send_start(void)
{
	unsigned char buf = UART_FRAME_START;
	
	uart_send_frame(&buf, 1);
}



static void uart_send_fileinfo(int filetype, int filesize)
{
	unsigned char buf[5];
	int packetnum = (filesize + UUP_PACKET_SIZE - 1) / UUP_PACKET_SIZE;
	
	buf[0] = UART_FRAME_FILEINFO;
	buf[1] = filetype;
	buf[2] = packetnum >> 16;
	buf[3] = packetnum >> 8;
	buf[4] = packetnum & 0xff;
	uart_send_frame(buf, 5);
}

static void uart_send_filedata(int packet_index, unsigned char *data, int len)
{
	unsigned char buf[UUP_MAX_FRAME_LEN];
	
	buf[0] = UART_FRAME_FILEXFER;
	buf[1] = packet_index & 0xff;
	memcpy(buf + 2, data, len);
	uart_send_frame(buf, len + 2);
}

static void uart_send_end(int ret)
{
	unsigned char buf[2];
	
	buf[0] = UART_FRAME_FINISH;
	buf[1] = ret;
	uart_send_frame(buf, 2);
}

static void VaribleInit(void)
{
	transferOver = 0;
	uup_rx_rev_len = 0;
	uup_rx_head = 0;
	uup_rx_tail = 0;
	uup_rx_state = 0;
	uup_rx_data_len = 0;
	uup_status = UUP_STATE_IDLE;
	uup_file_type = 0;
	uup_file_size = 0;
	uup_packet_num = 0;
	uup_rev_packet = 0;
	uup_rev_len = 0;
}

int pMain(int argc, char *argv0,char *argv1,char *argv2,char *argv3,char *argv4)
{
char *endptr;
	printf("%d !!\n", strtoul(argv3, &endptr, 10));

	CHAR comName[16];
	int comPort;
	int i;

	printf("start......\n");
	if (argc != 5) {
		printf("Input Error! Usage: %s uartPort baud fileType fileName.\n", argv0);
		//system("pause");
		return -1;
	}

	VaribleInit();
	
	char delims[] = "COM";
	char *result = NULL;
	result = strtok( argv1, delims );
	comPort = atoi(result);
	if (comPort < 10)
		sprintf(comName,  "%s", argv1);
	else
		sprintf(comName,  "\\\\.\\%s", argv1);


	hComm = CreateFile(comName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hComm == INVALID_HANDLE_VALUE) {
		wprintf(L"open %s fail.\n", comName);
		printfSys("open com fail");
		return -1;
	}
	else {
		wprintf(L"open %s ok.\n", comName);
		printfSys("open com ok");
	}

	//需要设置超时，否则，WriteFile() 或ReadFile()会一直阻塞任务。
	COMMTIMEOUTS comtimeout;
	comtimeout.ReadIntervalTimeout = 0xffffffff;
	comtimeout.ReadTotalTimeoutMultiplier = 0x0;
	comtimeout.ReadTotalTimeoutConstant = 0x0;

	comtimeout.WriteTotalTimeoutMultiplier = 4;//0x64; //100  // 表示平均写一个字节的时间上限
	comtimeout.WriteTotalTimeoutConstant = 32;//0x3e8; //992 //单位毫秒 //表示写数据总超时常量
	if (!SetCommTimeouts(hComm, &comtimeout)) {
		printf("SetCommTimeouts fail\n");
		printfSys("SetCommTimeouts fail");
	}

	//设置输入输出缓冲区大小
	if (!SetupComm(hComm, 4096, 4096)) {
		printf("SetupComm fail\n");
		printfSys("SetupComm fail");
	}

	DCB dcb;
	GetCommState(hComm, &dcb);
	dcb.BaudRate = strtoul(argv2, &endptr, 10);
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;
	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.EofChar = (char)0xa0;
	//dcb.fBinary = true;  // binary mode, no EOF check　指定是否允许二进制模式WIN95中须为TRUE
	SetCommState(hComm, &dcb);

	HANDLE hRevThread = CreateThread(NULL, 0, uartRevThread, 0, 0,NULL);

	int fileType;
	FILE *fp;
	int len;
	unsigned char buf[UUP_PACKET_SIZE];
	int filesize;
	int packetnum = 0;
	int ret = -1;
	int retry = 10000 / 100 * 2;
	fileType = strtoul(argv3, &endptr, 10);
	printf("fileType = %d !!\n", fileType);

	fp = fopen(argv4, "rb");
	if (!fp) {
		printf("open %s fail.\n", argv4);
		printfSys("open file fail");
		goto _end_;
	}
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	
	for (i = 0; i < retry; i++) {
		uart_send_start();
		if (uart_wait_ack(UART_FRAME_START, 100)) {
			printf("wait start ack ok.\n");
			printfSys("wait start ack ok.");
			break;
		} else {
			printf("wait start ack fail.\n");
			printfSys("wait start ack fail.");
		}
	}

	if (i == retry)
		goto _end_;

	retry = 3;
	for (i = 0; i < retry; i++) {
		uart_send_fileinfo(fileType, filesize);
		if (uart_wait_ack(UART_FRAME_FILEINFO, 300 * 1000)) {
			printf("wait fileinfo ack ok.\n");
			printfSys("wait fileinfo ack ok.");
			break;
		} else {
			printf("wait fileinfo ack fail.\n");
			printfSys("wait fileinfo ack fail.");
		}
	}
	if (i == retry)
		goto _end_;

	while (1) {
		len = fread(buf, 1, UUP_PACKET_SIZE, fp);
		if (len <= 0)
			break;
		for (i = 0; i < retry; i++) {
			uart_send_filedata(packetnum, buf, len);
			if (uart_wait_ack(UART_FRAME_FILEXFER, 1000)) {
				printf("wait filedata ack ok.\n");
				printfSys("wait filedata ack ok.");
				break;
			} else {
				printf("wait filedata ack fail.\n");
				printfSys("wait filedata ack fail.");
			}
		}
		if (i == retry)
			goto _end_;
		packetnum++;
	}

	ret = 0;

_end_:
	for (i = 0; i < retry; i++) {
		uart_send_end(ret == 0);
		if (uart_wait_ack(UART_FRAME_FINISH, 10000)) {
			printf("wait finish ack ok.\n");
			printfSys("wait finish ack ok.");
			break;
		}
		else {
			printf("wait finish ack fail.\n");
			printfSys("wait finish ack fail.");
		}
	}

	transferOver = 1;
	WaitForSingleObject(hRevThread, INFINITE);

	PurgeComm(hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	CloseHandle(hComm);

	return 0;
}

void startF()
{
	nStop=0;
}
void stopF()
{
	nStop=1;
}

void printfSys(CString str)
{
 	CBinport_App *p = (CBinport_App *)AfxGetApp();
 	p->pp->m_MsgMain.SetWindowText(str);
}