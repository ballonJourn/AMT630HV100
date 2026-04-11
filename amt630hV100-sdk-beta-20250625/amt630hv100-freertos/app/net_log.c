#include <stdlib.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "board.h"
#include "chip.h"
#include "animation.h"
#include "sfud.h"
#include "romfile.h"
#include "updatefile.h"
#include "sysinfo.h"
#include "mmcsd_core.h"
#include "ff_stdio.h"
#include "ota_update.h"
#include "md5.h"
#include "unzip.h"
#include "zip.h"
#include "ulog.h"
#include "easyflash.h"
#include "source/crc32.h"
#include "ff_sfdisk.h"
#include "ark_dwc2.h"

#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_DHCP.h"
#include "iperf_task.h"
#include "iot_wifi.h"
#include "FreeRTOS_DHCP_Server.h"

#ifdef NCM_UPDATE_SUPPORT

#define LOG_FRAME_HEADER_MAGIC	0xFF5AFFA5
#define LOG_FRAME_TAIL_MAGIC	0xFFA5FF5A
#define FILENAME_LEN	128
#define MD5_LEN			32

#define LOG_BUF_SIZE	0x100000

#pragma pack(push, 1)
typedef struct {
	uint32_t header;
	uint32_t type;
	uint32_t sub_type;
	uint32_t sub_type_1;
	uint32_t len;
	uint8_t checksum;
} LogFrame;

typedef struct {
	uint8_t request_type;
	uint32_t log_size;
	uint8_t log_md5[MD5_LEN];
	uint8_t log_filename[FILENAME_LEN];
} LogTfrReq;
#pragma pack(pop)

static Socket_t log_req_socket;
static Socket_t log_tfr_socket;

#define FRAME_MAX_LEN		0x100000
#define LOG_TFR_MAX_SIZE	128000
#define OTA_LOG_FILENAME	"log.zip"
static int log_rev_state = 0;
static int log_state_len = 0;
static int log_rev_len = 0;
static uint32_t log_frame_type = 0;
static uint32_t log_frame_subtype = 0;
static uint32_t log_frame_subtype_1 = 0;
static uint32_t log_frame_len = 0;
static uint8_t log_frame_checksum = 0;
static uint8_t *log_frame_buf;
static uint32_t log_file_size;
static uint32_t log_file_offset;
static uint32_t log_file_framenum;
static uint8_t net_log_timeout_flag = 0;

#define LOG_CHECKSUM(x) ((((x) >> 24) & 0xff) + (((x) >> 16) & 0xff) + (((x) >> 8) & 0xff) + ((x) & 0xff))
static uint8_t calc_log_checksum(uint32_t type, uint32_t subtype, uint32_t subtype_1, uint32_t len)
{
	uint32_t checksum = 0;

	checksum = LOG_CHECKSUM(type) + LOG_CHECKSUM(subtype) + LOG_CHECKSUM(subtype_1) + LOG_CHECKSUM(len);

	return 0x100 - (uint8_t)checksum;
}

static void logSendFrame(Socket_t socket, uint32_t type, uint32_t subtype,
								uint32_t subtype_1, void *data, uint32_t len)
{
	LogFrame *frame = NULL;
	uint8_t *tmp;
	int32_t leftsize;
	BaseType_t ret;
	u32 errcnt = 0;

	printf("send frame type=0x%x, subtype=0x%x, len=%d.\n", type, subtype, len);
	frame = (LogFrame*)pvPortMalloc(sizeof(LogFrame) + len + 4);
	if (frame) {
		frame->header = LOG_FRAME_HEADER_MAGIC;
		frame->type = type;
		frame->sub_type = subtype;
		frame->sub_type_1 = subtype_1;
		frame->len = len;
		frame->checksum = calc_log_checksum(type, subtype, subtype_1, len);
		tmp = (uint8_t*)frame + sizeof(LogFrame);
		if (len) {
			memcpy(tmp, data, len);
			tmp += len;
		}
		*(uint32_t *)tmp = LOG_FRAME_TAIL_MAGIC;
	}

	leftsize = sizeof(LogFrame) + len + 4;
	tmp = (uint8_t *)frame;
	while (leftsize > 0) {
		ret = FreeRTOS_send(socket, tmp, leftsize, 0);
		if (ret > 0) {
			leftsize -= ret;
			tmp += ret;
			errcnt =  0;
		} else {
			//printf("FreeRTOS_send err %d.\n", ret);
			TRACE_INFO("FreeRTOS_send err %d.\n", ret);
			if (errcnt++ > 100) {
				//printf("FreeRTOS_send fail.\n");
				TRACE_INFO("FreeRTOS_send fail.\n");
				break;
			}
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}
	net_log_timeout_flag = 1;
	vPortFree(frame);
}

static int logTransferData(void)
{
	char filename[FILENAME_LEN + 16] = OTA_MOUNT_PATH "/" OTA_LOG_FILENAME;
	FF_FILE *fp;
	uint32_t size;
	uint8_t *buf;

	fp = ff_fopen(filename, "rb");
	if (!fp) {
		printf("open log file fail.\n");
		return -1;
	}
	if (log_file_offset == 0) {
		log_file_framenum = 1;
		log_file_size = ff_filelength(fp);
	}
	size = log_file_size - log_file_offset;
	size = size > LOG_TFR_MAX_SIZE ? LOG_TFR_MAX_SIZE : size;
	ff_fseek(fp, log_file_offset, FF_SEEK_SET);
	log_file_offset += size;
	buf = pvPortMalloc(size + 6);
	if (!buf) {
		printf("tfrlog buf malloc fail.\n");
		ff_fclose(fp);
		return -1;
	}
	if (size > 0) {
		*(uint32_t*)buf = size;
		if (log_file_offset == log_file_size)
			*(uint16_t*)(buf + 4) = 0;
		else
			*(uint16_t*)(buf + 4) = log_file_framenum;
		log_file_framenum++;
		if (ff_fread(buf + 6, 1, size, fp) != size) {
			printf("read tfrlog fail.\n");
			vPortFree(buf);
			ff_fclose(fp);
			return -1;
		}
	} else {
		memset(buf, 0, 6);
	}
	logSendFrame(log_tfr_socket, 0x1f, 0x01, 0, buf, size + 6);
	vPortFree(buf);
	ff_fclose(fp);

	return 0;
}

static void RevLogFrameHandler(uint32_t type, uint32_t subtype, uint32_t subtype_1,
										uint8_t *data, uint32_t len)
{
	printf("rev frame type=0x%x, subtype=0x%x, len=%d.\n", type, subtype, len);
	switch (type) {
	case 0x0a:
		if (subtype == 0x10) {
			if (data[0] == 0x01) {
				LogTfrReq req = {0};
				uint8_t *logbuf;
				size_t logsize;
				SystemTime_t tm;
				char filename[FILENAME_LEN + 16] = OTA_MOUNT_PATH "/" OTA_LOG_FILENAME;
				zipFile zf = {0};
				zip_fileinfo zi = {0};
				FF_FILE *fp;
				size_t filesize;
				size_t leftsize;
				size_t size;
				size_t logindex;
				unsigned char md5out[16];
				char md5string[MD5_LEN + 1];
				md5_context md5ctx;
				uint32_t logcrc;
				int i;

				logbuf = pvPortMalloc(LOG_BUF_SIZE);
				if (!logbuf) {
					printf("logbuf malloc fail.\n");
					logSendFrame(log_req_socket, 0x1b, 0x06, 0, &req, sizeof(req));
					break;
				}

				iGetLocalTime(&tm);
				snprintf((char*)req.log_filename, FILENAME_LEN, "IPLog%.4d%.2d%.2d%.2d%.2d%.2d.zip", tm.tm_year,
					tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				zf = zipOpen(filename, 0);
				if (!zf) {
					printf("create log file fail.\n");
					vPortFree(logbuf);
					logSendFrame(log_req_socket, 0x1b, 0x06, 0, &req, sizeof(req));
					break;
				}
				if (zipOpenNewFileInZip(zf, "log", &zi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, 9)) {
					printf("zipOpenNewFileInZip fail.\n");
					zipClose(zf, NULL);
					vPortFree(logbuf);
					logSendFrame(log_req_socket, 0x1b, 0x06, 0, &req, sizeof(req));
					break;
				}

				leftsize = logsize = ef_log_get_used_size();
				logcrc = 0xffffffff;
				logindex = 0;
				while (leftsize > 0) {
					size = leftsize > LOG_BUF_SIZE ? LOG_BUF_SIZE : leftsize;
					read_flash_log(logbuf, logindex, size);
					if (zipWriteInFileInZip(zf, logbuf, size)) {
						printf("zipWriteInFileInZip fail.\n");
						break;
					}
					logcrc = xcrc32(logbuf, size, logcrc);
					leftsize -= size;
					logindex += size;
				}
				if (leftsize > 0) {
					zipClose(zf, NULL);
					vPortFree(logbuf);
					logSendFrame(log_req_socket, 0x1b, 0x06, 0, &req, sizeof(req));
					break;
				}
				sprintf((char*)logbuf, "\nlog size is 0x%x, crc is 0x%x.\n", logsize, logcrc);
				zipWriteInFileInZip(zf, logbuf, strlen((char*)logbuf));
				zipCloseFileInZip(zf);
				zipClose(zf, NULL);
				fp = ff_fopen(filename, "rb");
				if (!fp) {
					printf("open log zip file fail.\n");
					vPortFree(logbuf);
					logSendFrame(log_req_socket, 0x1b, 0x06, 0, &req, sizeof(req));
					break;
				}
				req.log_size = filesize = ff_filelength(fp);
				leftsize = filesize;
				memset(&md5ctx, 0, sizeof(md5_context));
				md5_starts(&md5ctx);
				while (leftsize > 0) {
					size = leftsize > LOG_BUF_SIZE ? LOG_BUF_SIZE : leftsize;
					size = ff_fread(logbuf, 1, size, fp);
					md5_update(&md5ctx, logbuf, size);
					leftsize -= size;
				}
				md5_finish(&md5ctx, md5out);
				ff_fclose(fp);
				vPortFree(logbuf);
   				for (i = 0; i < 16; i++)
					sprintf(md5string + 2 * i, "%.2x", md5out[i]);
				memcpy(req.log_md5, md5string, MD5_LEN);
				req.request_type = 1;
				logSendFrame(log_req_socket, 0x1b, 0x06, 0, &req, sizeof(req));
			}
		}
		break;
	case 0x0b:
		if (subtype == 0x06) {
			if (data[0] == 0x01 && data[1] == 0x00) {
				uint8_t errdata[6] = {0};

				log_file_offset = 0;
				if (logTransferData())
					logSendFrame(log_tfr_socket, 0x1f, 0x01, 0, errdata, sizeof(errdata));
			}
		}
		break;
	case 0x0f:
		if (subtype == 0x02) {
			if (data[0] == 0x00) {
				uint8_t errdata[6] = {0};

				if (logTransferData())
					logSendFrame(log_tfr_socket, 0x1f, 0x01, 0, errdata, sizeof(errdata));
			}
		} else if (subtype == 0x03) {
			if (data[128] == 0x00) {
				printf("log transfer ok.\n");
			} else if (data[128] == 0x01 && data[129] == 0x01) {
				uint8_t errdata[6] = {0};

				printf("log transfer fail, resend.\n");
				log_file_offset = 0;
				if (logTransferData())
					logSendFrame(log_tfr_socket, 0x1f, 0x01, 0, errdata, sizeof(errdata));
			}
		}
	}
}

static void RevLogDataHandler(uint8_t *buf, int32_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		switch (log_rev_state) {
		case 0:		//head receive
			if (log_state_len == 0 && buf[i] == 0xa5) {
				log_state_len++;
			} else if (log_state_len == 1 && buf[i] == 0xff) {
				log_state_len++;
			} else if (log_state_len == 2 && buf[i] == 0x5a) {
				log_state_len++;
			} else if (log_state_len == 3 && buf[i] == 0xff) {
				log_rev_state++;
				log_state_len = 0;
				log_frame_type = 0;
				log_frame_subtype = 0;
				log_frame_subtype_1 = 0;
				log_frame_len = 0;
			} else if (buf[i] == 0xff) {
				log_state_len = 1;
			} else {
				log_state_len = 0;
			}
			break;
		case 1:		//type receive
			log_frame_type |= buf[i] << (8 * log_state_len);
			if (++log_state_len == 4) {
				log_rev_state++;
				log_state_len = 0;
			}
			break;
		case 2:		//sub_type receive
			log_frame_subtype |= buf[i] << (8 * log_state_len);
			if (++log_state_len == 4) {
				log_rev_state++;
				log_state_len = 0;
			}
			break;
		case 3:		//sub_type_1 receive
			log_frame_subtype_1 |= buf[i] << (8 * log_state_len);
			if (++log_state_len == 4) {
				log_rev_state++;
				log_state_len = 0;
			}
			break;
		case 4:		//data len receive
			log_frame_len |= buf[i] << (8 * log_state_len);
			if (++log_state_len == 4) {
				if (log_frame_len > FRAME_MAX_LEN) {
					printf("Invalid NCM frame len 0x%x.\n", log_frame_len);
				}
				if (log_frame_len == 0)
					log_rev_state += 2;
				else
					log_rev_state++;
				log_rev_len = 0;
				log_state_len = 0;
			}
			break;
		case 5:		//checksum receive
			log_frame_checksum = buf[i];
			if (log_frame_checksum != calc_log_checksum(log_frame_type, log_frame_subtype,
										log_frame_subtype_1, log_frame_len)) {
				printf("log frame checksum fail.\n");
				log_rev_state = 0;
			} else {
				log_rev_state++;
			}
			break;
		case 6:		//data receive
			if (log_rev_len < FRAME_MAX_LEN)
				log_frame_buf[log_rev_len] = buf[i];
			if (++log_rev_len == log_frame_len)
				log_rev_state++;
			break;
		case 7:		//tail receive
			if (log_state_len == 0 && buf[i] == 0x5a) {
				log_state_len++;
			} else if (log_state_len == 1 && buf[i] == 0xff) {
				log_state_len++;
			} else if (log_state_len == 2 && buf[i] == 0xa5) {
				log_state_len++;
			} else if (log_state_len == 3 && buf[i] == 0xff) {
				RevLogFrameHandler(log_frame_type, log_frame_subtype, log_frame_subtype_1,
					log_frame_buf, log_frame_len);
				log_rev_state = 0;
				log_state_len = 0;
			} else {
				log_state_len = 0;
			}
			break;
		default:
			break;
		}
	}
}

static void set_socket_win_net(Socket_t xSockets)
{
    WinProperties_t xWinProperties;

    memset(&xWinProperties, '\0', sizeof xWinProperties);

    xWinProperties.lTxBufSize   = ipconfigIPERF_TX_BUFSIZE;	/* Units of bytes. */
    xWinProperties.lTxWinSize   = ipconfigIPERF_TX_WINSIZE;	/* Size in units of MSS */
    xWinProperties.lRxBufSize   = ipconfigIPERF_RX_BUFSIZE;	/* Units of bytes. */
    xWinProperties.lRxWinSize   = ipconfigIPERF_RX_WINSIZE; /* Size in units of MSS */

    FreeRTOS_setsockopt( xSockets, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProperties, sizeof( xWinProperties ) );
}

static uint8_t ncm_data_buffer[65536] = {0};

static int NCMLogConnectServer(Socket_t socket, int port)
{
	struct freertos_sockaddr xAddress;
	int retries = 3;

	xAddress.sin_port = FreeRTOS_htons( port );
	xAddress.sin_addr = FreeRTOS_inet_addr_quick(192, 168, 2, 99);

	while (retries-- > 0) {
    	if(!FreeRTOS_connect(socket, &xAddress, sizeof(xAddress)))
			return 0;
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	return -1;
}

static int vCreateNCMLogClientSocket( void )
{
	SocketSet_t xFD_Set;
	Socket_t xReqSockets = FREERTOS_INVALID_SOCKET, xTfrSockets = FREERTOS_INVALID_SOCKET;
	static const TickType_t xNoTimeOut = pdMS_TO_TICKS( 10000 );
	BaseType_t ret = -1;
	EventBits_t events;

	xFD_Set = FreeRTOS_CreateSocketSet();

	xReqSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xReqSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt( xReqSockets,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xNoTimeOut,
                         sizeof( xNoTimeOut ) );
    set_socket_win_net(xReqSockets);

	if (!NCMLogConnectServer(xReqSockets, 10001)) {
		printf("Connect to 192.168.2.99:10001 ok.\n");
    } else {
		printf("Connect to 192.168.2.99:10001 fail.\n");
		FreeRTOS_closesocket(xReqSockets);
		FreeRTOS_DeleteSocketSet(xFD_Set);
		return -1;
	}
	log_req_socket = xReqSockets;

	xTfrSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xTfrSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt( xTfrSockets,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xNoTimeOut,
                         sizeof( xNoTimeOut ) );
    set_socket_win_net(xTfrSockets);

	if (!NCMLogConnectServer(xTfrSockets, 10002)) {
		printf("Connect to 192.168.2.99:10002 ok.\n");
    } else {
		printf("Connect to 192.168.2.99:10002 fail.\n");
		FreeRTOS_closesocket(xReqSockets);
		FreeRTOS_closesocket(xTfrSockets);
		FreeRTOS_DeleteSocketSet(xFD_Set);
		return -1;
	}
	log_tfr_socket = xTfrSockets;

	while (1) {
		FreeRTOS_FD_CLR(xReqSockets, xFD_Set, eSELECT_ALL);
		FreeRTOS_FD_SET(xReqSockets, xFD_Set, eSELECT_READ | eSELECT_EXCEPT);
		FreeRTOS_FD_CLR(xTfrSockets, xFD_Set, eSELECT_ALL);
		FreeRTOS_FD_SET(xTfrSockets, xFD_Set, eSELECT_READ | eSELECT_EXCEPT);

		ret = FreeRTOS_select( xFD_Set, pdMS_TO_TICKS(3000));//portMAX_DELAY
		if (ret < 0) {
			break;
		} else if(ret == 0) {
			if(net_log_timeout_flag) {
				printf("%s, net_log timeout: %d\r\n", __func__, __LINE__);
				usb_dwc2_reset(0, 1);
				net_log_timeout_flag = 0;
				break;
			}
			continue;
		}

		net_log_timeout_flag = 0;
		if( events = FreeRTOS_FD_ISSET ( xReqSockets, xFD_Set ) ) {
			if (events & eSELECT_READ) {
				ret = FreeRTOS_recv(xReqSockets, ncm_data_buffer, sizeof(ncm_data_buffer), 0);
				if (ret > 0) {
					printf("@recv buf size:%d\r\n", ret);
					//print_hex(ncm_data_buffer, ret, NULL);
					RevLogDataHandler(ncm_data_buffer, ret);
				} else {
					printf("FreeRTOS_recv err:%d\r\n", ret);
					break;
				}
			}
			if (events & eSELECT_EXCEPT) {
				printf("xReqSockets exception\r\n");
				break;
			}
		}

		if( events = FreeRTOS_FD_ISSET ( xTfrSockets, xFD_Set ) ) {
			if (events & eSELECT_READ) {
				ret = FreeRTOS_recv(xTfrSockets, ncm_data_buffer, sizeof(ncm_data_buffer), 0);
				if (ret > 0) {
					printf("@@recv buf size:%d\r\n", ret);
					//print_hex(ncm_data_buffer, ret, NULL);
					RevLogDataHandler(ncm_data_buffer, ret);
				} else {
					printf("FreeRTOS_recv err:%d\r\n", ret);
					break;
				}
			}
			if (events & eSELECT_EXCEPT) {
				printf("xTfrSockets exception\r\n");
				break;
			}
		}
	}

	FreeRTOS_closesocket(xReqSockets);
	FreeRTOS_closesocket(xTfrSockets);
	FreeRTOS_DeleteSocketSet(xFD_Set);
	log_req_socket = NULL;
	log_tfr_socket = NULL;

	return 0;
}

void ncm_log_demo_thread(void *param)
{
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	FF_Disk_t *sfdisk = FF_SFDiskInit(SF_MOUNT_PATH);
	if (!sfdisk) {
		printf("FF_SFDiskInit fail.\n");
		return;
	}
#endif

	log_frame_buf = pvPortMalloc(FRAME_MAX_LEN);
	if (!log_frame_buf) {
		printf("log_frame_buf malloc fail.\n");
		return;
	}

	while(1) {
		vCreateNCMLogClientSocket();
		vTaskDelay(pdMS_TO_TICKS(3000));
	}
}

void ncm_log_demo(void)
{
	if (xTaskCreate(ncm_log_demo_thread, "ncmlog", 32768, NULL,
			1, NULL) != pdPASS) {
		printf("create ncm log demo task fail.\n");
	}
}

#endif
