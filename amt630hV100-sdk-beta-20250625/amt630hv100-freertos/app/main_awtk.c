
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
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
#ifdef WIFI_SUPPORT
#include "carlink_ey.h"
#include "carlink_ec.h"
#include "ark_network.h"
#endif

#ifdef USE_ULOG
#ifdef ULOG_BACKEND_USING_CONSOLE
extern int ulog_console_backend_init(void);
#endif
#ifdef ULOG_EASYFLASH_BACKEND_ENABLE
#include "easyflash.h"
#include "ulog_easyflash.h"
#endif
#ifdef ULOG_FILE_BACKEND_ENABLE
#include "ulog_file.h"
#endif
#endif

#ifdef OTA_UPDATE_SUPPORT
#include "ota_update.h"
#endif

#include "config/hcn_config.h"
#include "mw_init/hcn_mw_init.h"

#define WIFI_TEST			0
#define BT_TEST				0
#define SDMMC_TEST			0
#define USB_DEV_PLUGED		0
#define USB_DEV_UNPLUGED	1
//#define TASK_STATUS_MONITOR

int carlink_aa_init();
int carlink_cp_init();

extern int usb_wait_stor_dev_pluged(uint32_t timeout);
extern void hub_usb_dev_reset(void);
extern int xm_vg_init (unsigned int heap_addr, unsigned int size);
#ifdef DELTA_UPDATE_SUPPORT
extern int delta_update(int filetype, size_t patchFileSize);
#endif

#ifdef AWTK

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#ifdef VG_DRIVER
#pragma data_alignment=1024
#ifdef __HCN_CONFIG_H__
#define VG_HEAP_SIZE  HCN_VG_HEAP_SIZE
#else
#define VG_HEAP_SIZE	0xc00000
#endif
__no_init static uint8_t vgHeap[VG_HEAP_SIZE];
#endif

#ifdef CARLINK_ENABLE
static char qr_text_buf[100] = {0};	//手机互联二维码数据缓存

int get_qr_text_buf(char *buf, int len)
{
	int ret = -1;
	int qr_len = strlen(qr_text_buf);

	if ((qr_len > 0) && (len > qr_len)) {
		strcpy(buf, qr_text_buf);
		ret = 0;
	}

	return ret;
}

void set_qr_text_buf(const char *str)
{
	if (strlen(str) < sizeof(qr_text_buf)) {
		strcpy(qr_text_buf, str);
	}
}
#endif

#if !defined(VG_ONLY) && !defined(AWTK)

#else
typedef int16_t lv_coord_t;
typedef uint8_t lv_indev_state_t;
typedef struct {
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;

enum { LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR };

enum {
    LV_KEY_UP        = 17,  /*0x11*/
    LV_KEY_DOWN      = 18,  /*0x12*/
    LV_KEY_RIGHT     = 19,  /*0x13*/
    LV_KEY_LEFT      = 20,  /*0x14*/
    LV_KEY_ESC       = 27,  /*0x1B*/
    LV_KEY_DEL       = 127, /*0x7F*/
    LV_KEY_BACKSPACE = 8,   /*0x08*/
    LV_KEY_ENTER     = 10,  /*0x0A, '\n'*/
    LV_KEY_NEXT      = 9,   /*0x09, '\t'*/
    LV_KEY_PREV      = 11,  /*0x0B, '*/
    LV_KEY_HOME      = 2,   /*0x02, STX*/
    LV_KEY_END       = 3,   /*0x03, ETX*/
};

typedef struct {
    lv_point_t point; /**< For LV_INDEV_TYPE_POINTER the currently pressed point*/
    uint32_t key;     /**< For LV_INDEV_TYPE_KEYPAD the currently pressed key*/
    uint32_t btn_id;  /**< For LV_INDEV_TYPE_BUTTON the currently pressed button*/
    int16_t enc_diff; /**< For LV_INDEV_TYPE_ENCODER number of steps since the previous read*/

    lv_indev_state_t state; /**< LV_INDEV_STATE_REL or LV_INDEV_STATE_PR*/
} lv_indev_data_t;
#endif


/* define dummy function to avoid linker error */
void SendTouchInputEvent(void *indata)
{
}

void SendTouchInputEventFromISR(void *indata)
{
}

void SendKeypadInputEvent(void *indata)
{
}
extern void carlink_send_key_event(uint8_t key, bool pressed);

void SendKeypadInputEventFromISR(void *indata)
{
	lv_indev_data_t* input = (lv_indev_data_t *)indata;
	//printf("isr %d:%d\n", input->key, input->state);

	carlink_send_key_event((uint8_t)input->key, (bool)input->state);
}
#ifdef WIFI_SUPPORT
#if WIFI_TEST
//#define RELTECK_WIFI_AP_MODE

#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_DHCP.h"
#include "carlink_ey.h"
#include "carlink_ey_video.h"
#include "iperf_task.h"
#include "iot_wifi.h"
#include "FreeRTOS_DHCP_Server.h"
#ifdef RELTECK_WIFI_AP_MODE
static const uint8_t ucIPAddress[4] = {192, 168, 13, 1};
#else
static const uint8_t ucIPAddress[4] = {192, 168, 13, 37};
#endif
static const uint8_t ucNetMask[4] = {255, 255, 255, 0};
//static const uint8_t ucGatewayAddress[4] = {192, 168, 13, 1};
static const uint8_t ucGatewayAddress[4] = {192, 168, 13, 1};
static const uint8_t ucDNSServerAddress[4] = {8, 8, 8, 8};
//static const uint8_t ucMACAddress[6] = {0x00, 0x0c, 0x29, 0x5d, 0x2e, 0x03};
//static const uint8_t ucMACAddress[6] = {0x68, 0xb9, 0xd3, 0xc1, 0x28, 0x03};
static const uint8_t ucMACAddress[6] = {0x30, 0x4a, 0x26, 0x78, 0xfd, 0x12};
uint8_t wifi_data_buffer[65536] = {0};
void ark_test_h264_dec();

struct test_header
{
	uint16_t id;
	uint16_t payload_len;
};
#if 0
static int vCreateTCPServerSocket( void )
{
	SocketSet_t xFD_Set;
	struct freertos_sockaddr xAddress, xRemoteAddr;
	Socket_t xSockets = FREERTOS_INVALID_SOCKET, xClientSocket = FREERTOS_INVALID_SOCKET;
	socklen_t xClientLength = sizeof( xAddress );
	static const TickType_t xNoTimeOut = portMAX_DELAY;

	BaseType_t ret = -1;
	BaseType_t xResult;
	struct test_header header;
	uint8_t header_buf[4];
	uint8_t* header_buf_ptr;
	const int header_len = sizeof(struct test_header);
	int header_buf_len = 0;
	uint8_t *h264SrcBuf = NULL;
	uint8_t *h264SrcBufPtr = NULL;
	int32_t h264SrcSize = 0, h264SrcSizePos = 0;
	uint8_t err_flag = 0;int i;
	video_frame_s* frame = NULL;

	xFD_Set = FreeRTOS_CreateSocketSet();
	xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt( xSockets,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xNoTimeOut,
                         sizeof( xNoTimeOut ) );
    xAddress.sin_port = ( uint16_t ) 11111;
    xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
    FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xSockets, 1 );
	//ark_test_h264_dec();

	carlink_ey_video_init();

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
				uint8_t pucBuffer[32] = {0};
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
				FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
				printf("Carlink: Received a connection from %s:%u\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
			}
			continue;
        } else if( FreeRTOS_FD_ISSET ( xClientSocket, xFD_Set ) ) {
			header_buf_ptr = header_buf;
			header_buf_len = header_len;
			err_flag = 0;
			while (header_buf_len > 0) {
				err_flag = 0;
				ret = FreeRTOS_recv(xClientSocket, (void*)header_buf_ptr, header_buf_len, 0);
				if (ret < 0) {
					err_flag = 1;
					printf("FreeRTOS_recv header err:%d\r\n", ret);
					break;
				}
				header_buf_ptr += ret;
				header_buf_len -= ret;
			}
			if (err_flag) {
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				video_frame_s* dummy = NULL;
				notify_h264_frame_ready(&dummy);
				continue;
			}
			/*printf("##header:");

			for (i = 0; i < header_len; i++) {
				printf("%02x ", header_buf[i]);
			}printf("\r\n");*/

			//READ_LE16(header_buf, header.id);
			//READ_LE16(header_buf + 2, header.payload_len);
			header.id = (header_buf[0] | (header_buf[1] << 8));
			header.payload_len = (header_buf[2] | (header_buf[3] << 8));
			printf("recv id:%d len:%d\r\n", header.id, header.payload_len);

			int retry_cnt = 0;
			h264SrcSize = header.payload_len;
get_retry:
			frame = get_h264_frame_buf();
			if (NULL == frame) {
				printf("h264 frame is empty\r\n");
				vTaskDelay(pdMS_TO_TICKS(10));
				goto get_retry;
				//continue;
			}

			h264SrcSizePos = h264SrcSize;
			h264SrcBufPtr  = frame->cur;
			h264SrcBuf     = frame->cur;
			frame->len     = h264SrcSize;
			err_flag       = 0;
			while (h264SrcSizePos > 0) {
				//printf("h264SrcSizePos:%d\r\n", h264SrcSizePos);
				ret = FreeRTOS_recv( xClientSocket, (void *)h264SrcBufPtr, h264SrcSizePos, 0);
				//printf("lBytes:%d h264SrcSizePos:%d\r\n", lBytes, h264SrcSizePos);
				if (ret < 0) {
					printf("FreeRTOS_recv err:%d\r\n", ret);
					err_flag = 1;
					break;
				}
				h264SrcBufPtr  += ret;
				h264SrcSizePos -= ret;
			}/*printf("read finished\r\n");

			printf("payload:");

			for (i = 0; i < 16; i++) {
				printf("%02x ", h264SrcBuf[i]);
			}printf("\r\n");*/

			if (err_flag) {
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				video_frame_s* dummy = NULL;
				notify_h264_frame_ready(&dummy);
				continue;
			}
			notify_h264_frame_ready(&frame);
		}
	}


	FreeRTOS_closesocket(xClientSocket);
	FreeRTOS_closesocket(xSockets);
}
#else
static int vCreateTCPServerSocket( void )
{
	SocketSet_t xFD_Set;
	struct freertos_sockaddr xAddress, xRemoteAddr;
	Socket_t xSockets = FREERTOS_INVALID_SOCKET, xClientSocket = FREERTOS_INVALID_SOCKET;
	socklen_t xClientLength = sizeof( xAddress );
	static const TickType_t xNoTimeOut = portMAX_DELAY;
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
    xAddress.sin_port = ( uint16_t ) 11111;
    xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
    FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xSockets, 1 );
	//ark_test_h264_dec();

	carlink_ey_video_init();

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
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
				FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
				printf("Carlink: Received a connection from %s:%u\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
			}
			continue;
        } else if( FreeRTOS_FD_ISSET ( xClientSocket, xFD_Set ) ) {

			ret = FreeRTOS_recv(xClientSocket, wifi_data_buffer, sizeof wifi_data_buffer, 0);
			if (ret > 0) {
				printf("recv buf size:%d\r\n", ret);
			} else {
				printf("FreeRTOS_recv err:%d\r\n", ret);
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				continue;
			}
		}
	}


	FreeRTOS_closesocket(xClientSocket);
	FreeRTOS_closesocket(xSockets);
        return 0;
}

#endif

#if ( ipconfigUSE_DHCP_HOOK != 0 )
eDHCPCallbackAnswer_t xApplicationDHCPHook2( eDHCPCallbackPhase_t eDHCPPhase,
                                            uint32_t ulIPAddress )
{
	eDHCPCallbackAnswer_t eReturn;
	//uint32_t ulStaticIPAddress, ulStaticNetMask;
    char ip_str[20] = {0};
        sprintf(ip_str, "%d.%d.%d.%d\r\n", (ulIPAddress >> 0) & 0xFF,
    	(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
    printf("\r\n  eDHCPPhase:%d  ulIPAddress:%s state:%d \r\n", eDHCPPhase, ip_str, getDhcpClientState());
	if (getDhcpClientState() == 0)
		return eDHCPStopNoChanges;

	switch( eDHCPPhase )
	{
	case eDHCPPhasePreDiscover  :
	  eReturn = eDHCPContinue;
	  break;

	case eDHCPPhasePreRequest  :
 #if 0
	  ulStaticIPAddress = FreeRTOS_inet_addr_quick( ucIPAddress[0],
	                                                ucIPAddress[1],
	                                                ucIPAddress[2],
	                                                ucIPAddress[3] );

	  ulStaticNetMask = FreeRTOS_inet_addr_quick( ucNetMask[0],
	                                              ucNetMask[1],
	                                              ucNetMask[2],
	                                              ucNetMask[3] );

	  ulStaticIPAddress &= ulStaticNetMask;
	  ulIPAddress &= ulStaticNetMask;
	  if( ulStaticIPAddress == ulIPAddress ) {
	    eReturn = eDHCPUseDefaults;
	  } else {
	    eReturn = eDHCPContinue;
	  }
     #else
     eReturn = eDHCPContinue;
     #endif
	  break;
	default :
	  eReturn = eDHCPContinue;
	  break;
	}

       return eReturn;
}
#endif

void wifi_test_event_handler( WIFIEvent_t * xEvent )
{
    WIFIEventType_t xEventType = xEvent->xEventType;

    if (0) {
    } else if (eWiFiEventConnected                    == xEventType) {// meter is sta
        printf("\r\n The meter is connected to ap \r\n");
    } else if (eWiFiEventDisconnected                == xEventType) {// meter is sta
        printf("\r\n The meter is disconnected from ap \r\n");
    } else if (eWiFiEventAPStationConnected       == xEventType) {// meter is ap
        printf("\r\n The meter in AP is connected by a sta \r\n");
    } else if (eWiFiEventAPStationDisconnected   == xEventType) {// meter is ap
        printf("\r\n The sta is disconnected from the meter \r\n");
    }
}

int wifi_sta_test_proc();
int wifi_ap_test_proc();
static void wifi_demo_test(void)
{
	BaseType_t ret = 0;
	unsigned int status;
	uint32_t IPAddress = (32 << 24) | (13 << 16) | (168 << 8) | (192 << 0);

	for (;;) {
		status = mmcsd_wait_sdio_ready((int32_t)portMAX_DELAY);
		if (status == MMCSD_HOST_PLUGED) {
			printf("detect sdio device\r\n");
			break;
		}
	}
#ifndef RELTECK_WIFI_AP_MODE
    setDhcpClientState(1);
#endif
       ret = ret;
	//vTaskDelay(pdMS_TO_TICKS(5000));//wait connect
	ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);
	//ark_wlan_init();
#ifdef RELTECK_WIFI_AP_MODE
	wifi_ap_test_proc();
#else
         WIFI_RegisterEvent(eWiFiEventMax, wifi_test_event_handler);
	wifi_sta_test_proc();
#endif
	//vTaskDelay(pdMS_TO_TICKS(8000));
	while(0) {
		//printf("send ping\r\n");
		FreeRTOS_SendPingRequest(IPAddress, 8, 1000);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	vTaskDelay(pdMS_TO_TICKS(1000));
#ifdef RELTECK_WIFI_AP_MODE
	setDhcpClientState(0);
	IPAddress = (20 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
	dhcpserver_start(ucIPAddress, IPAddress, 10);
#else
	setDhcpClientState(1);
#endif
	//vCreateTCPServerSocket();
	vIPerfInstall();
}
#endif
#endif

#ifdef SDMMC_SUPPORT
#if SDMMC_TEST
static void sdcard_read_thread(void *para)
{
	unsigned int status;

	for (;;) {
		status = mmcsd_wait_cd_changed(portMAX_DELAY);
		if (status == MMCSD_HOST_PLUGED) {
			printf("card inserted.\n");
#ifdef OTA_UPDATE_SUPPORT
			FF_FILE *fp = ff_fopen("/sd/update.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_WHOLE);
			}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
			fp = ff_fopen("/sd/emmcldr.bin", "rb");
#else
			fp = ff_fopen("/sd/spildr.bin", "rb");
#endif
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_FIRSTLDR);
			}

			fp = ff_fopen("/sd/stepldr.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_STEPLDR);
			}

			fp = ff_fopen("/sd/lnchemmc.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_LNCHEMMC);
			}
#else
			FF_FILE *fp = ff_fopen("/sd/update.bin", "rb");
			if (fp) {
				UpFileHeader header;
				SysInfo *sysinfo = GetSysInfo();
				if (ff_fread(&header, 1, sizeof(header), fp) == sizeof(header)) {
					if (header.magic != MKTAG('U', 'P', 'D', 'F')) {
						printf("Wrong update file, don't update.\n");
					} else {
						if (header.checksum != sysinfo->app_checksum) {
							printf("found different update file(0x%x-0x%x), update...\n",
								header.checksum, sysinfo->app_checksum);
							sysinfo->update_media_type = UPDATE_MEDIA_SD;
							sysinfo->update_status = UPDATE_STATUS_START;
							SaveSysInfo();
							wdt_cpu_reboot();
						} else {
							printf("the update file version is same, don't update.\n");
						}
					}
				};
				ff_fclose(fp);
			} else {
				printf("open update.bin fail.\n");
			}
#endif
		} else if (status == MMCSD_HOST_UNPLUGED) {
			printf("card removed.\n");
		}
	}
}

static void sdcard_read_demo(void)
{
	static StaticTask_t xSDReadTaskTCB;
	static StackType_t uxSDReadTaskStack[configMINIMAL_STACK_SIZE * 2];

	if (xTaskCreateStatic(sdcard_read_thread,
							"sdread",
							configMINIMAL_STACK_SIZE * 2,
							NULL,
							1,
							uxSDReadTaskStack,
							&xSDReadTaskTCB) == NULL) {
		printf("create sdread task fail.\n");
	}
}
#endif
#endif

#ifdef USB_SUPPORT
static void usb_read_thread(void *para)
{
	unsigned int status;

	for (;;) {
		status = usb_wait_stor_dev_pluged(portMAX_DELAY);
		if (status == USB_DEV_PLUGED) {
			printf("usb dev inserted.\n");
#ifdef OTA_UPDATE_SUPPORT
#ifdef DELTA_UPDATE_SUPPORT
			//Demo从U盘读取patch文件来模拟接收patch文件
			int filetype;
			char filename[32];
			FF_FILE *fp;
			size_t filesize;
			uint8_t *filebuf;
			size_t leftsize;
			size_t wsize;
			uint32_t offset;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sfud_flash *sflash = sfud_get_device(0);
#endif

			filebuf = pvPortMalloc(0x10000);
			if (!filebuf) {
				printf("%s filebuf malloc fail.\n", __func__);
				continue;
			}
			for (filetype = UPFILE_TYPE_WHOLE; filetype <= UPFILE_TYPE_STEPLDR; filetype++) {
				strcpy(filename, "/usb/");
				strcat(filename, g_upfilename[filetype]);
				strcpy(strrchr(filename, '.'), "_patch.bin");
				fp = ff_fopen(filename, "rb");
				if (!fp) {
					printf("not found patch file %s.\n", filename);
					continue;
				}
				offset = OTA_MEDIA_OFFSET;
				filesize = ff_filelength(fp);
				leftsize = filesize;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				sfud_erase(sflash, OTA_MEDIA_OFFSET, filesize);
#endif
				while (leftsize) {
					wsize = leftsize > 0x10000 ? 0x10000 : leftsize;
					ff_fread(filebuf, 1, wsize, fp);
#if DEVICE_TYPE_SELECT == EMMC_FLASH
					emmc_write(offset, wsize, filebuf);
#else
					sfud_write(sflash, offset, wsize, filebuf);
#endif
					offset += wsize;
					leftsize -= wsize;
				}
				ff_fclose(fp);
				delta_update(filetype, filesize);
			}
			vPortFree(filebuf);
#else
			FF_FILE *fp = ff_fopen("/usb/update.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_WHOLE);
			}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
			fp = ff_fopen("/usb/emmcldr.bin", "rb");
#else
			fp = ff_fopen("/usb/spildr.bin", "rb");
#endif
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_FIRSTLDR);
			}

			fp = ff_fopen("/usb/stepldr.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_STEPLDR);
			}

			fp = ff_fopen("/usb/lnchemmc.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_LNCHEMMC);
			}
#endif
#else
			FF_FILE *fp = ff_fopen("/usb/update.bin", "rb");
			if (fp) {
				UpFileHeader header;
				SysInfo *sysinfo = GetSysInfo();
				if (ff_fread(&header, 1, sizeof(header), fp) == sizeof(header)) {
					if (header.magic != MKTAG('U', 'P', 'D', 'F')) {
						printf("Wrong update file, don't update.\n");
					} else {
						if (header.checksum != sysinfo->app_checksum) {
							printf("found different update file(0x%x-0x%x), update...\n",
								header.checksum, sysinfo->app_checksum);
							sysinfo->update_media_type = UPDATE_MEDIA_USB;
							sysinfo->update_status = UPDATE_STATUS_START;
							SaveSysInfo();
							hub_usb_dev_reset();
							vTaskDelay(500);
							wdt_cpu_reboot();
						} else {
							printf("the update file version is same, don't update.\n");
						}
					}
				};
				ff_fclose(fp);
			} else {
				printf("open update.bin fail.\n");
			}
#endif
		} else if (status == USB_DEV_UNPLUGED) {
			printf("usb removed.\n");
		}
	}
}

static void usb_read_demo(void)
{
	if (xTaskCreate(usb_read_thread, "usbread", configMINIMAL_STACK_SIZE * 16, NULL,
			1, NULL) != pdPASS) {
		printf("create usbread task fail.\n");
	}
}
#endif
#ifdef USB_SUPPORT
extern int get_usb_mode();
extern int ark_network_init(void);
#endif

void awtk_thread(void *data)
{
	printf("awtk thread start.\n");

#if DEVICE_TYPE_SELECT != EMMC_FLASH
	/* initialize the spi flash */
	sfud_init();
#ifdef SPI0_QSPI_MODE
	sfud_qspi_fast_read_enable(sfud_get_device(0), 4);
#endif
#else
	mmcsd_wait_mmc_ready(portMAX_DELAY);
#endif

#ifdef USE_ULOG
	ulog_init();
#ifdef ULOG_BACKEND_USING_CONSOLE
	ulog_console_backend_init();
#endif
#ifdef ULOG_EASYFLASH_BACKEND_ENABLE
	easyflash_init();
	ulog_ef_backend_init();
#endif
#ifdef ULOG_FILE_BACKEND_ENABLE
	usb_wait_stor_dev_pluged(portMAX_DELAY);
	ulog_file_backend_init();
#endif
	TRACE_INFO("use ulog.\n");
	//read_all_flash_log();
#endif

	/* read sysinfo */
	ReadSysInfo();

	GetUpFileInfo();

	/* initialize carback */
#ifdef CARBACK_DETECT
	// carback_init();
#endif

	/* play animation */
#if ANIMATION_POLICY != ANIMATION_NONE
	animation_init();
	animation_start();
#endif

	/* uart rx demo */
	uart_rx_demo();

	/* can demo */
	//can_demo();

	/* read sd card demo */
#ifdef SDMMC_SUPPORT
#if SDMMC_TEST
	sdcard_read_demo();
#endif
#endif

#ifdef USB_SUPPORT
	extern int get_usb_mode();
	extern int ark_network_init(void);
	extern void ncm_update_demo();
	extern void ncm_log_demo();
	extern void wifi_update_demo(void);
	if (get_usb_mode()) {
		ark_network_init();
#ifdef NCM_UPDATE_SUPPORT
		ncm_update_demo();
#endif
#ifdef NCM_LOG_SUPPORT
		ncm_log_demo();
#endif
	} else {
#ifdef WIFI_UPDATE_SUPPORT
		wifi_update_demo();
#else
		usb_read_demo();
#endif
	}
#endif

#ifdef WIFI_SUPPORT
#if WIFI_TEST
	wifi_demo_test();
#else
#if USE_LWIP && (0 == CARLINK_EY && 0 == CARLINK_EC)
    //ark_network_init();
#endif

#if CARLINK_EY
	set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
	set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
	carlink_ey_init();
#endif
#if CARLINK_EC
	set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
	set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
	carlink_ec_init(0, NULL);
#endif

#if CARLINK_CP
	carlink_cp_init();
#endif

#if CARLINK_AA
	carlink_aa_init();
#endif
#endif
#endif

	/* read romfile */
	ReadRomFile();

#ifdef TP_SUPPORT
	extern int tp_init(void);
	tp_init();
#endif

#ifdef VG_DRIVER
	xm_vg_init((unsigned int)vgHeap, VG_HEAP_SIZE);
#else
	extern int gui_app_start(int lcd_w, int lcd_h);
	gui_app_start (OSD_WIDTH, OSD_HEIGHT);
#endif

// ark_lcd_osd_enable(LCD_UI_LAYER,0);
	hcn_mw_init();

    while(1) {
#ifdef TASK_STATUS_MONITOR
		static uint32_t idletick = 0;
		uint8_t CPU_RunInfo[1024];

		if (xTaskGetTickCount() - idletick > configTICK_RATE_HZ * 10) {
			memset(CPU_RunInfo,0,1024);
			vTaskList((char *)&CPU_RunInfo); //获取任务运行时间信息
			printf("---------------------------------------------\r\n");
			printf("Task          State   Priority  Stack    #\r\n");
			printf("%s", CPU_RunInfo);
			printf("---------------------------------------------\r\n");
			memset(CPU_RunInfo,0,1024);
			vTaskGetRunTimeStats((char *)&CPU_RunInfo);
			printf("Task          Abs Time          % Time\r\n");
			printf("%s", CPU_RunInfo);
			printf("---------------------------------------------\r\n\n");
			idletick = xTaskGetTickCount();
		}
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));       /*Just to let the system breath*/
    }
}

void vRegisterSampleCLICommands( void );
void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );
void main_awtk(void)
{
	/* Start the task that manages the command console for FreeRTOS+CLI. */
	vUARTCommandConsoleStart( ( configMINIMAL_STACK_SIZE * 3 ), tskIDLE_PRIORITY );

	/* Register the standard CLI commands. */
	vRegisterSampleCLICommands();

	xTaskCreate(awtk_thread, "awtk", 32768, NULL,
		tskIDLE_PRIORITY + 1, NULL);
}
#endif
