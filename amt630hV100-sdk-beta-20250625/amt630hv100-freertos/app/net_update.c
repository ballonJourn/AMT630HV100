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
#include "ff_sddisk.h"

#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_DHCP.h"
#include "iperf_task.h"
#include "iot_wifi.h"
#include "FreeRTOS_DHCP_Server.h"

extern FF_Disk_t *sdmmc_disk;

#ifdef NCM_UPDATE_SUPPORT

#define FRAME_HEADER_MAGIC	0xA5FF5AFF
#define FRAME_TAIL_MAGIC	0x5AFFA5FF
#define VERSION_LEN		64
#define FILENAME_LEN	128
#define MD5_LEN			32
#define MAX_PATH		256

typedef enum {
	IP_CANNOT_FILE_TFR = 7111,
	IP_FW_CANNOT_TFR = 7121,
	IP_ERR_DATA_LEN = 7122,
	IP_ERR_OFFSET = 7123,
	IP_ERR_FILE_CHECK = 7141,
	IP_SPEED_FAIL = 7301,
	IP_POWER_FAIL = 7302,
	IP_GEAR_FAIL = 7304,
	IP_UPDATE_ABORT = 7411,
	IP_ROLL_BACK_FAIL = 7511,
} ENegCode;

typedef struct {
	uint32_t header;
	uint32_t type;
	uint32_t sub_type;
	uint32_t len;
} HUFrame;

typedef struct {
	uint32_t header;
	uint32_t type;
	uint32_t sub_type;
	uint32_t rsp_flag;
	uint32_t len;
} IPFrame;

typedef struct {
	char hard_partnumber[VERSION_LEN];
	char hard_version[VERSION_LEN];
	char soft_partnumber[VERSION_LEN];
	char soft_version[VERSION_LEN];
} ReqVersionPosiRsp;

typedef struct {
	uint32_t negative_code;
} ReqVersionNegRsp;

typedef struct {
	char file_name[FILENAME_LEN];
	uint32_t file_length;
	char file_md5[MD5_LEN];
} ReqFileCheckRsp;

typedef struct {
	char file_name[FILENAME_LEN];
	uint32_t file_length;
	char file_md5[MD5_LEN];
	uint32_t offset;
} ReqFileWriteRsp;

static Socket_t net_client_socket;

#define FRAME_MAX_LEN		0x100000
static int net_rev_state = 0;
static int net_state_len = 0;
static int net_rev_len = 0;
static uint32_t net_frame_type = 0;
static uint32_t net_frame_subtype = 0;
static uint32_t net_frame_len = 0;
static int net_toolong_frame = 0;
static uint8_t *net_frame_buf;
static char cur_file_md5[MD5_LEN];
static char cur_file_name[MAX_PATH];

static void netSendFrame(uint32_t type, uint32_t subtype, uint32_t rsp_flag, void *data, uint32_t len)
{
	IPFrame *ipframe = NULL;
	uint8_t *tmp;
	int32_t leftsize;
	BaseType_t ret;
	u32 errcnt = 0;

	printf("send frame type=0x%x, subtype=0x%x, rsp_flag=%d, data=0x%x, len=%d.\n",
			type, subtype, rsp_flag, data, len);
	ipframe = (IPFrame*)pvPortMalloc(sizeof(IPFrame) + len + 4);
	if (ipframe) {
		ipframe->header = FRAME_HEADER_MAGIC;
		ipframe->type = type;
		ipframe->sub_type = subtype;
		ipframe->rsp_flag = rsp_flag;
		ipframe->len = len;
		tmp = (uint8_t*)ipframe + sizeof(IPFrame);
		if (len) {
			memcpy(tmp, data, len);
			tmp += len;
		}
		*(uint32_t *)tmp = FRAME_TAIL_MAGIC;
	}

	leftsize = sizeof(IPFrame) + len + 4;
	tmp = (uint8_t *)ipframe;
	while (leftsize > 0) {
		ret = FreeRTOS_send(net_client_socket, tmp, leftsize, 0);
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
	vPortFree(ipframe);
}

static int install_update_file(char *filename)
{
	int i;

	for (i = 0; i < UPFILE_TYPE_NUM; i++) {
		if (!strcmp(filename, g_upfilename[i])) {
			return update_from_media(OTA_MOUNT_PATH, i);
		}
	}

	return -1;
}

static u32 create_file_err_count = 0;
static void RevFrameHandler(uint32_t type, uint32_t subtype, uint8_t *data, uint32_t len)
{
	printf("rev frame type=0x%x, subtype=0x%x, data=0x%x, len=%d.\n", type, subtype, data, len);
	switch (type) {
	case 0x70:
		if (subtype == 0x01) {
			if (1) {
				ReqVersionPosiRsp rsp = {0};
				//strcpy(rsp.hard_partnumber, "aaa");
				//strcpy(rsp.hard_version, "01.00.00");
				strcpy(rsp.soft_partnumber, "23847069");
				strcpy(rsp.soft_version, "SW:-.-.-");
				netSendFrame(0x70, 0x01, 0x01, &rsp, sizeof(rsp));
			} else {
				ReqVersionNegRsp rsp = {0};
				rsp.negative_code = 7011;
				netSendFrame(0x70, 0x01, 0x00, &rsp, sizeof(rsp));
			}
		}
		break;
	case 0x71:
		if (subtype == 0x01) {
			FF_FILE *fp = NULL;
			ReqFileWriteRsp rsp = {0};
			char filename[FILENAME_LEN+4];
			FF_FILE *fmd5 = NULL;
			char md5name[FILENAME_LEN+16];
			char md5[MD5_LEN];
			uint32_t err_code =IP_CANNOT_FILE_TFR;

			if (len != FILENAME_LEN + 4 + MD5_LEN && len != FILENAME_LEN + 4) {
				printf("invalid len %d.\n", len);
				netSendFrame(0x71, 0x01, 0x00, &err_code, sizeof(err_code));
				break;
			}
			memcpy(rsp.file_name, data, FILENAME_LEN);
			memcpy(&rsp.file_length, data + FILENAME_LEN, 4);
			if (len == FILENAME_LEN + 4 + MD5_LEN) {
				memcpy(rsp.file_md5, data + FILENAME_LEN + 4, MD5_LEN);
				memcpy(cur_file_md5, data + FILENAME_LEN + 4, MD5_LEN);
			} else {
				memset(cur_file_md5, 0, MD5_LEN);
			}
			printf("rev update file %s, length is 0x%x.\n", rsp.file_name, rsp.file_length);

			strcpy(md5name, OTA_MOUNT_PATH "/");
			strcat(md5name, rsp.file_name);
			strcat(md5name, ".md5");
			strcpy(filename, OTA_MOUNT_PATH "/");
			strcat(filename, rsp.file_name);
			fp = ff_fopen(filename, "rb");
			if (fp) {
				fmd5 = ff_fopen(md5name, "rb");
				if (fmd5) {
					ff_fread(md5, 1, MD5_LEN, fmd5);
					if (memcmp(md5, cur_file_md5, MD5_LEN)) {
						printf("md5 is not same as previous, rev new file.\n");
						ff_fclose(fp);
						ff_remove(filename);
						ff_fclose(fmd5);
						ff_remove(md5name);
						fmd5 = NULL;
					} else {
						printf("md5 is same as previous, continue rev.\n");
						rsp.offset = ff_filelength(fp);
						ff_fclose(fmd5);
						ff_fclose(fp);
					}
				} else {
					ff_fclose(fp);
					ff_remove(filename);
				}
			}

			if (!fmd5) {
				fmd5 = ff_fopen(md5name, "wb");
				if (!fmd5) {
					uint32_t err_code = IP_CANNOT_FILE_TFR;
					netSendFrame(0x71, 0x01, 0x00, &err_code, sizeof(err_code));
					if (++create_file_err_count > 3) {
						FF_SDDiskFormatRemount(sdmmc_disk, SDMMC_MOUNT_PATH);
						create_file_err_count = 0;
					}
					break;
				}
				ff_fwrite(cur_file_md5, 1, MD5_LEN, fmd5);
				ff_fclose(fmd5);
				rsp.offset = 0;
			}
			netSendFrame(0x71, 0x01, 0x01, &rsp, sizeof(rsp));
		} else if (subtype == 0x02) {
			FF_FILE *fp;
			ReqFileWriteRsp rsp = {0};
			char filename[FILENAME_LEN + 4];
			uint32_t data_length;
			uint32_t offset;
			uint32_t err_code;

			memcpy(rsp.file_name, data, FILENAME_LEN);
			memcpy(&rsp.file_length, data + FILENAME_LEN, 4);
			memcpy(&data_length, data + FILENAME_LEN + 4, 4);
			memcpy(&offset, data + FILENAME_LEN + 8, 4);
			printf("rev update data %s, offset=0x%x, data_length=0x%x.\n",
				rsp.file_name, offset, data_length);

			if (len != FILENAME_LEN + 12 + data_length) {
				printf("invalid len %d.\n", len);
				err_code= IP_ERR_DATA_LEN;
				netSendFrame(0x71, 0x02, 0x00, &err_code, sizeof(err_code));
				break;
			}

			if (net_toolong_frame) {
				err_code= IP_ERR_DATA_LEN;
				netSendFrame(0x71, 0x02, 0x00, &err_code, sizeof(err_code));
				break;
			}

			strcpy(filename, OTA_MOUNT_PATH "/");
			strcat(filename, rsp.file_name);
			fp = ff_fopen(filename, "a+");
			if (fp) {
				rsp.offset = ff_filelength(fp);
				if (rsp.offset != offset) {
					printf("error! rev offset isn't equal to saved file length.\n");
					err_code= IP_FW_CANNOT_TFR;
					netSendFrame(0x71, 0x02, 0x00, &err_code, sizeof(err_code));
				} else {
					if (ff_fwrite(data + 140, 1, data_length, fp) == data_length) {
						memcpy(rsp.file_md5, cur_file_md5, MD5_LEN);
						netSendFrame(0x71, 0x02, 0x01, &rsp, sizeof(rsp));
					} else {
						err_code= IP_FW_CANNOT_TFR;
						netSendFrame(0x71, 0x02, 0x00, &err_code, sizeof(err_code));
					}
				}
				ff_fclose(fp);
			} else {
				printf("create %s fail.\n", filename);
				err_code= IP_FW_CANNOT_TFR;
				netSendFrame(0x71, 0x02, 0x00, &err_code, sizeof(err_code));
			}
		} else if (subtype == 0x03) {
			uint8_t reserved[256] = {0};
			printf("file transfer finished.\n");
			netSendFrame(0x71, 0x03, 0x01, reserved, 256);
		} else if (subtype == 0x04) {
			printf("start file check...\n");
			FF_FILE *fp;
			ReqFileCheckRsp rsp = {0};
			char filename[FILENAME_LEN+4];
			uint32_t err_code = IP_ERR_FILE_CHECK;

			if (len != FILENAME_LEN + 4 + MD5_LEN) {
				printf("invalid len %d.\n", len);
				netSendFrame(0x71, 0x04, 0x00, &err_code, sizeof(err_code));
				break;
			}
			memcpy(rsp.file_name, data, FILENAME_LEN);
			memcpy(&rsp.file_length, data + FILENAME_LEN, 4);
			memcpy(rsp.file_md5, data + FILENAME_LEN + 4, MD5_LEN);

			strcpy(filename, OTA_MOUNT_PATH "/");
			strcat(filename, rsp.file_name);
			fp = ff_fopen(filename, "rb");
			if (fp && ff_filelength(fp) == rsp.file_length) {
				md5_context ctx = {0};
				int leftsize = rsp.file_length;
				void *cbuf = pvPortMalloc(IMAGE_RW_SIZE);
				int rsize;
				unsigned char md5out[16];
				char md5string[MD5_LEN + 1];
				int i;

				if (cbuf) {
					md5_starts(&ctx);
					while (leftsize > 0) {
						rsize = leftsize > IMAGE_RW_SIZE ? IMAGE_RW_SIZE : leftsize;
						ff_fread(cbuf, 1, rsize, fp);
						md5_update(&ctx, cbuf, rsize);
						leftsize -= rsize;
					}
					md5_finish(&ctx, md5out);
					vPortFree(cbuf);
					for (i = 0; i < 16; i++)
						sprintf(md5string + 2*i, "%.2x", md5out[i]);
					if (!memcmp(md5string, rsp.file_md5, MD5_LEN)) {
						printf("file check ok.\n");
						ff_fclose(fp);
						strcpy(cur_file_name, rsp.file_name);
						netSendFrame(0x71, 0x04, 0x01, &rsp, sizeof(rsp));
						break;
					}
				}
			}
			printf("file check fail.\n");
			if (fp) {
				ff_fclose(fp);
				ff_remove(filename);
			}
			memset(cur_file_name, 0, MAX_PATH);
			netSendFrame(0x71, 0x04, 0x00, &err_code, sizeof(err_code));
		}
		break;
	case 0x72:
		if (subtype == 0x01) {
		} else if (subtype == 0x02) {
		}
		break;
	case 0x73:
		if (subtype == 0x01) {
			uint8_t reserved[256] = {0};
			printf("installation condition check ok\n");
			netSendFrame(0x73, 0x01, 0x01, reserved, 256);
		}
		break;
	case 0x74:
		if (subtype == 0x01) {
			uint8_t reserved[256] = {0};
			uint32_t err_code = IP_UPDATE_ABORT;
			FF_FILE *fp;
			unzFile zf;
			unz_global_info ginfo;
			unz_file_info finfo;
			char filename[256];
			char *buf;
			int leftsize, wsize;
			int ret;
			int i;
			char filepath[256 + 16];

			if (strlen(cur_file_name) == 0) {
				printf("not found update file.\n");
				netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
				break;
			}
			printf("start install update file...\n");
			strcpy(filepath, OTA_MOUNT_PATH "/");
			strcat(filepath, cur_file_name);
			zf = unzOpen(filepath);
			if (!zf) {
				printf("open zip file fail.\n");
				netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
				break;
			}
			buf = malloc(IMAGE_RW_SIZE);
			if (!buf) {
				netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
				break;
			}
			unzGetGlobalInfo(zf, &ginfo);
			for (i = 0; i < ginfo.number_entry; i++) {
				ret = unzGetCurrentFileInfo(zf, &finfo, filename, 256, NULL, 0, NULL, 0);
				if (ret != UNZ_OK) {
					printf("unzGetCurrentFileInfo fail.\n");
					netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
					break;
				}
				printf("unzip file %s.\n", filename);
				ret = unzOpenCurrentFile(zf);
				if (ret != UNZ_OK) {
					printf("unzOpenCurrentFile fail.\n");
					netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
					break;
				}
				leftsize = finfo.uncompressed_size;
				strcpy(filepath, OTA_MOUNT_PATH "/");
				strcat(filepath, filename);
				fp = ff_fopen(filepath, "wb");
				if (!fp) {
					printf("create %s fail.\n", filepath);
					unzCloseCurrentFile(zf);
					netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
					if (++create_file_err_count > 3) {
						FF_SDDiskFormatRemount(sdmmc_disk, SDMMC_MOUNT_PATH);
						create_file_err_count = 0;
					}
					break;
				}
				while (leftsize > 0) {
					wsize = leftsize > IMAGE_RW_SIZE ? IMAGE_RW_SIZE : leftsize;
					unzReadCurrentFile(zf, buf, wsize);
					if (ff_fwrite(buf, 1, wsize, fp) != wsize) {
						printf("ff_fwrite fail.\n");
						break;
					}
					leftsize -= wsize;
				}
				ff_fclose(fp);
				if (leftsize > 0) {
					netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
					break;
				}
				if (install_update_file(filename)) {
					printf("install update file %s fail.\n", filename);
					netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
					break;
				}
				unzCloseCurrentFile(zf);
				ret = unzGoToNextFile(zf);
				if (ret != UNZ_OK && ret != UNZ_END_OF_LIST_OF_FILE) {
					printf("unzGoToNextFile fail.\n");
					netSendFrame(0x74, 0x01, 0x00, &err_code, sizeof(err_code));
					break;
				}
			}
			free(buf);
			unzClose(zf);
			if (i == ginfo.number_entry && ret == UNZ_END_OF_LIST_OF_FILE)
				netSendFrame(0x74, 0x01, 0x01, reserved, 256);
		}
		break;
	case 0x75:
		if (subtype == 0x01) {
			uint8_t reserved[256] = {0};
			printf("roll back ok\n");
			netSendFrame(0x75, 0x01, 0x01, reserved, 256);
		}
		break;
	}
}

static void RevDataHandler(uint8_t *buf, int32_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		switch (net_rev_state) {
		case 0:		//head receive
			if (net_state_len == 0 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 1 && buf[i] == 0x5a) {
				net_state_len++;
			} else if (net_state_len == 2 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 3 && buf[i] == 0xa5) {
				net_rev_state++;
				net_state_len = 0;
				net_frame_type = 0;
				net_frame_subtype = 0;
				net_frame_len = 0;
			} else if (buf[i] == 0xff) {
				net_state_len = 1;
			} else {
				net_state_len = 0;
			}
			break;
		case 1:		//type receive
			net_frame_type |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				net_rev_state++;
				net_state_len = 0;
			}
			break;
		case 2:		//sub_type receive
			net_frame_subtype |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				net_rev_state++;
				net_state_len = 0;
			}
			break;
		case 3:		//data len receive
			net_frame_len |= buf[i] << (8 * net_state_len);
			if (++net_state_len == 4) {
				if (net_frame_len > FRAME_MAX_LEN) {
					printf("Invalid NCM frame len 0x%x.\n", net_frame_len);
					net_toolong_frame = 1;
				} else {
					net_toolong_frame = 0;
				}
				if (net_frame_len == 0)
					net_rev_state += 2;
				else
					net_rev_state++;
				net_rev_len = 0;
				net_state_len = 0;
			}
			break;
		case 4:		//data receive
			if (net_rev_len < FRAME_MAX_LEN)
				net_frame_buf[net_rev_len] = buf[i];
			if (++net_rev_len == net_frame_len)
				net_rev_state++;
			break;
		case 5:		//tail receive
			if (net_state_len == 0 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 1 && buf[i] == 0xa5) {
				net_state_len++;
			} else if (net_state_len == 2 && buf[i] == 0xff) {
				net_state_len++;
			} else if (net_state_len == 3 && buf[i] == 0x5a) {
				RevFrameHandler(net_frame_type, net_frame_subtype, net_frame_buf, net_frame_len);
				net_rev_state = 0;
				net_state_len = 0;
			} else {
				net_state_len = 0;
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

void print_hex(unsigned char *data, int len, const char* tag)
{
    unsigned long i, j, l;
    unsigned char tmp_str[140];
    unsigned char tmp_str1[10];

    for (i = 0; i < len; i += 16) {
        int n ;
        tmp_str[0] = '\0';
        n = i ;

        for (j = 0; j < 4; j++) {
            l = n % 16;

            if (l >= 10)
                tmp_str[3 - j] = (unsigned char)('A' + l - 10);
            else
                tmp_str[3 - j] = (unsigned char)(l + '0');

            n >>= 4 ;
        }

        tmp_str[4] = '\0';
        strcat((char *) tmp_str, ": ");

        /*
           Output the hex bytes
         */
        for (j = i; j < (i + 16); j ++) {
            int m ;

            if (j < len) {
                m = ((unsigned int)((unsigned char) * (data + j))) / 16 ;

                if (m >= 10)
                    tmp_str1[0] = 'A' + (unsigned char) m - 10;
                else
                    tmp_str1[0] = (unsigned char) m + '0';

                m = ((unsigned int)((unsigned char) * (data + j))) % 16 ;

                if (m >= 10)
                    tmp_str1[1] = 'A' + (unsigned char) m - 10;
                else
                    tmp_str1[1] = (unsigned char) m + '0';

                tmp_str1[2] = '\0';
                strcat((char *) tmp_str, (char *) tmp_str1);
                strcat((char *) tmp_str, " ");
            } else {
                strcat((char *) tmp_str, "   ");
            }
        }

        strcat((char *) tmp_str, "  ");
        l = strlen((char *) tmp_str);

        /* Output the ASCII bytes */
        for (j = i; j < (i + 16); j++) {
            if (j < len) {
                char c = * (data + j);

                if (c < ' ' || c > 'z') {
                    c = '.';
                }

                tmp_str[l ++] = c;
            } else {
                tmp_str[l ++] = ' ';
            }
        }

        tmp_str[l ++] = '\r';
        tmp_str[l ++] = '\n';
        tmp_str[l ++] = '\0';

        printf("%s\r\n", (const char *) tmp_str);
    }
}

//static uint8_t *testbuf;
//static uint32_t testbuf_len = 0;
static uint8_t ncm_data_buffer[65536] = {0};
static int vCreateNCMUpdateServerSocket( void )
{
	SocketSet_t xFD_Set;
	struct freertos_sockaddr xAddress, xRemoteAddr;
	Socket_t xSockets = FREERTOS_INVALID_SOCKET, xClientSocket = FREERTOS_INVALID_SOCKET;
	socklen_t xClientLength = sizeof( xAddress );
	static const TickType_t xNoTimeOut = pdMS_TO_TICKS( 10000 );
	BaseType_t ret = -1;
	BaseType_t xResult;

	xFD_Set = FreeRTOS_CreateSocketSet();
	xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt( xSockets,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xNoTimeOut,
                         sizeof( xNoTimeOut ) );
    set_socket_win_net(xSockets);
    //xAddress.sin_port = ( uint16_t ) 10003;
    xAddress.sin_port = FreeRTOS_htons( 10003 );
    FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xSockets, 3 );

	//testbuf = pvPortMalloc(0x200000);
	//memset(testbuf, 0, 0x200000);
	while (1) {
		FreeRTOS_FD_CLR(xSockets, xFD_Set, eSELECT_READ);
		FreeRTOS_FD_SET(xSockets, xFD_Set, eSELECT_READ);
		if (xClientSocket && xClientSocket != FREERTOS_INVALID_SOCKET) {
			FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
			FreeRTOS_FD_SET( xClientSocket, xFD_Set, eSELECT_READ );
		}

    	xResult = FreeRTOS_select( xFD_Set, portMAX_DELAY );
        if (xResult < 0) {
			break;
        }

		if( FreeRTOS_FD_ISSET ( xSockets, xFD_Set ) ) {
			xClientSocket = FreeRTOS_accept( xSockets, &xRemoteAddr, &xClientLength);
			if( ( xClientSocket != NULL ) && ( xClientSocket != FREERTOS_INVALID_SOCKET ) ) {
				char pucBuffer[32] = {0};
				//FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				//FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
				set_socket_win_net(xClientSocket);
				FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
				FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
				printf("NCM: Received a connection from %s:%u\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
				net_client_socket = xClientSocket;
				net_rev_state = 0;
				net_state_len = 0;
			}
			continue;
        } else if( FreeRTOS_FD_ISSET ( xClientSocket, xFD_Set ) ) {

			ret = FreeRTOS_recv(xClientSocket, ncm_data_buffer, sizeof(ncm_data_buffer), 0);
			if (ret > 0) {
				printf("@@recv buf size:%d\r\n", ret);
				//print_hex(ncm_data_buffer, 160, NULL);
				//print_hex(ncm_data_buffer + ret - 16, 16, NULL);
				RevDataHandler(ncm_data_buffer, ret);
				//memcpy(testbuf + testbuf_len, ncm_data_buffer, ret);
				//testbuf_len += ret;
			} else {
				printf("FreeRTOS_recv err:%d\r\n", ret);
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				net_rev_state = 0;
				net_state_len = 0;
				continue;
			}
		}
	}

	FreeRTOS_closesocket(xClientSocket);
	FreeRTOS_closesocket(xSockets);

	return 0;
}

void ncm_update_demo_thread(void *param)
{
	net_frame_buf = pvPortMalloc(FRAME_MAX_LEN);
	if (!net_frame_buf) {
		printf("net_frame_buf malloc fail.\n");
		return;
	}
	vCreateNCMUpdateServerSocket();

	while(1)
		vTaskDelay(portMAX_DELAY);
}

void ncm_update_demo(void)
{
	if (xTaskCreate(ncm_update_demo_thread, "ncmupdate", configMINIMAL_STACK_SIZE * 16, NULL,
			1, NULL) != pdPASS) {
		printf("create ncm update demo task fail.\n");
	}
}

#endif
