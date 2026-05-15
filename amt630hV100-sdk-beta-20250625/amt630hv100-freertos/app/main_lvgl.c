
/**
 * @file main
 *
 */

#if !defined(VG_ONLY) && !defined(AWTK)

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/arklcd.h"
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
#include "lv_examples/src/lv_demo_printer/lv_demo_printer.h"
#include "xinbas/xinbas_demo.h"
#include "haoke/haoke_demo.h"
#include "lv_lib_png/lv_png.h"

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
#ifdef OTA_UPDATE_SUPPORT
#include "ota_update.h"
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


#define WIFI_TEST			0
#define BT_TEST				0

#define SDMMC_TEST			0
#undef TASK_STATUS_MONITOR

#define USB_DEV_PLUGED		0
#define USB_DEV_UNPLUGED	1
extern int usb_wait_stor_dev_pluged(uint32_t timeout);
extern void hub_usb_dev_reset(void);
#if CARLINK_AA
extern int carlink_aa_init();
#endif
#if CARLINK_CP
extern int carlink_cp_init();
#endif

static void hal_init(void);
#if !LV_TICK_CUSTOM
static void tick_thread(void *data);
#endif

#ifdef VG_DRIVER
#pragma data_alignment=1024
#ifdef REVERSE_TRACK
#define VG_HEAP_SIZE	0x600000
#else
#define VG_HEAP_SIZE	0xa00000
#endif
__no_init static uint8_t vgHeap[VG_HEAP_SIZE];
extern int xm_vg_init (unsigned int heap_addr, unsigned int size);
#endif

#define INPUT_QUEUE_LEN		64
static QueueHandle_t touch_input_mq;
static QueueHandle_t keypad_input_mq;

void SendTouchInputEvent(lv_indev_data_t *indata)
{
	xQueueSend(touch_input_mq, indata, 0);
}

void SendTouchInputEventFromISR(lv_indev_data_t *indata)
{
	xQueueSendFromISR(touch_input_mq, indata, 0);
}

void SendKeypadInputEvent(lv_indev_data_t *indata)
{
	xQueueSend(keypad_input_mq, indata, 0);
}

/*void SendKeypadInputEventFromISR(lv_indev_data_t *indata)
{
	xQueueSendFromISR(keypad_input_mq, indata, 0);
}*/

extern void carlink_send_key_event(uint8_t key, bool pressed);

void SendKeypadInputEventFromISR(void *indata)
{
	lv_indev_data_t* input = (lv_indev_data_t *)indata;
	//printf("isr %d:%d\n", input->key, input->state);

	carlink_send_key_event((uint8_t)input->key, (bool)input->state);
}


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
					if(header.magic != MKTAG('U', 'P', 'D', 'F')){
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
	if (xTaskCreate(sdcard_read_thread, "sdread", configMINIMAL_STACK_SIZE, NULL,
			1, NULL) != pdPASS) {
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

#ifdef AUDIO_REPLAY
#include "audio.h"

#define BUFSZ   4096

struct RIFF_HEADER_DEF
{
    char riff_id[4];     // 'R','I','F','F'
    uint32_t riff_size;
    char riff_format[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT_DEF
{
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct FMT_BLOCK_DEF
{
    char fmt_id[4];    // 'f','m','t',' '
    uint32_t fmt_size;
    struct WAVE_FORMAT_DEF wav_format;
};

struct DATA_BLOCK_DEF
{
    char data_id[4];     // 'R','I','F','F'
    uint32_t data_size;
};

struct wav_info
{
    struct RIFF_HEADER_DEF header;
    struct FMT_BLOCK_DEF   fmt_block;
    struct DATA_BLOCK_DEF  data_block;
};

#if 0
static void pcmplay_thread(void *param)
{
	#define PCM_BUF_SIZE	0x100000
	u32 *pcmBuf = pvPortMalloc(PCM_BUF_SIZE);
	if (pcmBuf) {
		struct audio_caps caps = {0};
		struct audio_device *audio;

		/* 打开 Audio 播放设备 */
		audio = audio_dev_open(AUDIO_FLAG_REPLAY);
		if (!audio) {
			printf("Open audio device fail.\n");
			return;
		}

		/* 设置采样率、通道、采样位数等音频参数信息 */
		caps.main_type               = AUDIO_TYPE_OUTPUT;   /* 输出类型（播放设备 ）*/
		caps.sub_type                = AUDIO_DSP_PARAM;             /* 设置所有音频参数信息 */
		caps.udata.config.samplerate = 44100;                               /* 采样率 */
		caps.udata.config.channels   = 2;                                   /* 采样通道 */
		caps.udata.config.samplebits = 16;                                  /* 采样位数 */
		audio_dev_configure(audio, &caps);

		/* Load external pcm file to waveBuf */
		printf("### pcmBuf:0x%x\n", (u32)pcmBuf);
		//get pcm file from SD card or use jlink to download pcm file to pcmBuf.
		//...
		CP15_clean_dcache_for_dma((uint32_t)pcmBuf, (uint32_t)pcmBuf + PCM_BUF_SIZE);

		/* play */
		while(1) {
			int i;
			char *buf = (char *)pcmBuf;
			for(i=0; i<PCM_BUF_SIZE/BUFSZ; i++) {
				audio_dev_write(audio, buf, BUFSZ);
				buf += BUFSZ;
			}
		}

		/* 关闭 Audio 设备 */
		audio_dev_close(audio);

		vPortFree(pcmBuf);
	}
}
#endif

#define ROMFILE_ADUIO

/* Parse wav file header info */
static int wavplay_parse_header(void *file, struct wav_info *info)
{
	int max_sub_chunk = 8;

#ifdef ROMFILE_ADUIO
	if (RomFileRead((RomFile *)file, &(info->header), sizeof(struct RIFF_HEADER_DEF)) <= 0)
		goto __exit;

	/* seek to fmt sub-chunk(Drop other sub-chunk in front of fmt chunk) */
	while(1)
	{
		if (RomFileRead((RomFile *)file, &(info->fmt_block),  sizeof(struct FMT_BLOCK_DEF)) <= 0)
			goto __exit;

		if(memcmp(info->fmt_block.fmt_id, "fmt ", 4) == 0)
			break;	/* "fmt " sub-chunk fond */

		/* sub-chunk "fmt " not fond, seek to the next sub-chunk */
		RomFileSeek((RomFile *)file, info->fmt_block.fmt_size - sizeof(struct WAVE_FORMAT_DEF), SEEK_CUR);

		/* timeout */
		if(max_sub_chunk-- <= 0)
			goto __exit;
	};
	if (info->fmt_block.fmt_size > sizeof(struct WAVE_FORMAT_DEF))
		RomFileSeek((RomFile *)file, info->fmt_block.fmt_size - sizeof(struct WAVE_FORMAT_DEF), SEEK_CUR);

	max_sub_chunk = 8;
	while(1)
	{
		if (RomFileRead((RomFile *)file, &(info->data_block),  sizeof(struct DATA_BLOCK_DEF)) <= 0)
			goto __exit;

		if(memcmp(info->data_block.data_id, "data", 4) == 0)
			break;	/* "data" sub-chunk fond */

		/* sub-chunk "data" not fond, seek to the next sub-chunk */
		RomFileSeek((RomFile *)file, info->data_block.data_size, SEEK_CUR);

		/* timeout */
		if(max_sub_chunk-- <= 0)
			goto __exit;
	};
#else
	if (ff_fread(&(info->header), sizeof(struct RIFF_HEADER_DEF), 1, (FF_FILE *)file) <= 0)
		goto __exit;

	/* seek to fmt sub-chunk(Drop other sub-chunk in front of fmt chunk) */
	while(1)
	{
		if (ff_fread(&(info->fmt_block),  sizeof(struct FMT_BLOCK_DEF), 1, (FF_FILE *)file) <= 0)
			goto __exit;

		if(memcmp(info->fmt_block.fmt_id, "fmt ", 4) == 0)
			break;	// "fmt " sub-chunk fond.

		/* sub-chunk "fmt " not fond, seek to the next sub-chunk */
		ff_fseek((FF_FILE *)file, info->fmt_block.fmt_size - sizeof(struct WAVE_FORMAT_DEF), FF_SEEK_CUR);

		/* timeout */
		if(max_sub_chunk-- <= 0)
			goto __exit;
	};
	if (info->fmt_block.fmt_size > sizeof(struct WAVE_FORMAT_DEF))
		ff_fseek((FF_FILE *)file, info->fmt_block.fmt_size - sizeof(struct WAVE_FORMAT_DEF), FF_SEEK_CUR);

	max_sub_chunk = 8;
	while(1)
	{
		if (ff_fread(&(info->data_block),  sizeof(struct DATA_BLOCK_DEF), 1, (FF_FILE *)file) <= 0)
			goto __exit;

		if(memcmp(info->data_block.data_id, "data", 4) == 0)
			break;	// "data" sub-chunk fond.

		/* sub-chunk "fmt " not fond, seek to the next sub-chunk */
		ff_fseek((FF_FILE *)file, info->data_block.data_size, FF_SEEK_CUR);

		/* timeout */
		if(max_sub_chunk-- <= 0)
			goto __exit;
	};
#endif

	return 0;
__exit:
	printf("%s fail.\n", __func__);

	return -1;
}

static void wavplay_thread(void *param)
{
#ifdef ROMFILE_ADUIO
    RomFile *file = NULL;
#else
	FF_FILE *file = NULL;
#endif
    uint8_t *buffer = NULL;
    struct wav_info *info = NULL;
    struct audio_caps caps = {0};
	struct audio_device *audio;
	int data_size;

#ifdef ROMFILE_ADUIO
    file = RomFileOpen("audio/sin1K.wav");
#else
	file = ff_fopen("/sd/audio/sin1K", "rb");
#endif
    if (!file)
    {
        printf("open file failed!\n");
        goto __exit;
    }

    buffer = pvPortMalloc(BUFSZ);
    if (buffer == NULL)
        goto __exit;

    info = (struct wav_info *) pvPortMalloc(sizeof(*info));
    if (info == NULL)
        goto __exit;

	/* 解析文件头信息 */
	if(wavplay_parse_header((void *)file, info) < 0)
        goto __exit;

    printf("wav information:\n");
    printf("samplerate %d\n", info->fmt_block.wav_format.SamplesPerSec);
    printf("channel %d\n", info->fmt_block.wav_format.Channels);

    /* 打开 Audio 播放设备 */
    audio = audio_dev_open(AUDIO_FLAG_REPLAY);
	if (!audio)
	{
		printf("Open audio device fail.\n");
		goto __exit;
	}

    /* 设置采样率、通道、采样位数等音频参数信息 */
    caps.main_type               = AUDIO_TYPE_OUTPUT;                           /* 输出类型（播放设备 ）*/
    caps.sub_type                = AUDIO_DSP_PARAM;                             /* 设置所有音频参数信息 */
    caps.udata.config.samplerate = info->fmt_block.wav_format.SamplesPerSec;    /* 采样率 */
    caps.udata.config.channels   = info->fmt_block.wav_format.Channels;         /* 采样通道 */
    caps.udata.config.samplebits = 16;                                          /* 采样位数 */
    audio_dev_configure(audio, &caps);

	data_size = info->data_block.data_size;

    while (data_size > 0)
    {
        int length;

        /* 从文件系统读取 wav 文件的音频数据 */
#ifdef ROMFILE_ADUIO
        length = RomFileRead(file, buffer, BUFSZ);
#else
		length = ff_fread(buffer, 1, BUFSZ, file);
#endif
		if(length > data_size)
			length = data_size;

        if (length <= 0)
            break;

        /* 向 Audio 设备写入音频数据 */
        audio_dev_write(audio, buffer, length);
		data_size -= length;
    }

    /* 关闭 Audio 设备 */
    audio_dev_close(audio, AUDIO_FLAG_REPLAY);

__exit:
    if (file)
#ifdef ROMFILE_ADUIO
        RomFileClose(file);
#else
		ff_fclose(file);
#endif

    if (buffer)
        vPortFree(buffer);

    if (info)
        vPortFree(info);

	while(1)
    	vTaskDelay(portMAX_DELAY);
}

static void wavplay_demo(void)
{
	if (xTaskCreate(wavplay_thread, "wavplay", configMINIMAL_STACK_SIZE, NULL,
			configMAX_PRIORITIES - 3, NULL) != pdPASS) {
		printf("create wavplay task fail.\n");
	}
}
#endif

#ifdef AUDIO_RECORD
#include "audio.h"
#define RECORD_BUF_SIZE 1024*1024
#define RECORD_REQ_SIZE 1024
static uint8_t *wavrecord_buffer = NULL;

static uint8_t *waverecord_get_a_new_buffer(int size)
{
	static int waverecord_index = 0;
	uint8_t *buffer = NULL;

	if(!wavrecord_buffer)
	{
		wavrecord_buffer = pvPortMalloc(RECORD_BUF_SIZE);
		if(!wavrecord_buffer)
			return NULL;
		waverecord_index = 0;
	}

	if(waverecord_index + size < RECORD_BUF_SIZE)
	{
		buffer = wavrecord_buffer + waverecord_index;
		waverecord_index += size;
	}
	else
	{
		printf("record done, start_addr:0x%x, size:0x%x\n", (u32)wavrecord_buffer, RECORD_BUF_SIZE);
		//CP15_clean_dcache_for_dma((uint32_t)wavrecord_buffer, (uint32_t)wavrecord_buffer + RECORD_BUF_SIZE);
		buffer = wavrecord_buffer;
		waverecord_index = 0;
	}

	return buffer;
}

static int wavrecord_callback(struct audio_device *audio)
{
	int ret = -1;

	/* save record data */
	if(audio && audio->record)
	{
		int size = audio->record->buf_info.total_size;
		if(size == RECORD_REQ_SIZE)
		{
			uint8_t *buffer = waverecord_get_a_new_buffer(size);
			if(buffer)
			{
				/* clear buffer */
				//memset(buffer, 0, size);

#if 1			/* method 1: use dynamic buffer to receive record data(without copy data). [Recommend this method] */
				audio_dev_record_set_param(audio, buffer, size);	//Set a new buffer to receive the next frame.
#else			/* method 2: use one buffer to receive record data(should copy data) */
				if(audio->record->buf_info.buffer)
					memcpy(buffer, audio->record->buf_info.buffer, size);	//Copy the record data to a destnation buffer.
#endif
				ret = 0;
			}
		}
		else
		{
			printf("%s() Invalid size:%d\n", __func__, size);
		}
	}
	return ret;
}

static void wavrecord_thread(void *param)
{
    uint8_t *buffer = NULL;
	struct audio_device *audio;
	int ret;

	audio = audio_dev_open(AUDIO_FLAG_RECORD);
    if(audio)
    {
	    /* set record sample information */
		struct audio_caps caps = {0};
	    caps.main_type               = AUDIO_TYPE_INPUT;	/* 输入类型（录音设备 ）*/
	    caps.sub_type                = AUDIO_DSP_PARAM;		/* 设置所有音频参数信息 */
	    caps.udata.config.samplerate = 16000;				/* 采样率 */
	    caps.udata.config.channels   = 2;					/* 采样通道 */
	    caps.udata.config.samplebits = 16;					/* 采样位数 */
		audio_dev_configure(audio, &caps);

		/* register record complete callback */
		audio_dev_register_record_callback(audio, wavrecord_callback);

		/* set record buffer and size */
		buffer = waverecord_get_a_new_buffer(RECORD_REQ_SIZE);
		ret = audio_dev_record_set_param(audio, buffer, RECORD_REQ_SIZE);
		if(ret)
		{
			printf("%s() audio_dev_record_set_param failed \n", __func__);
			goto exit;
		}

		/* start recording */
		audio_dev_record_start(audio);

		printf("wav record information:\n");
		printf("samplerate %d\n", caps.udata.config.samplerate);
		printf("channel %d\n", caps.udata.config.channels);
		printf("samplebits %d\n", caps.udata.config.samplebits);
    }

	while(1)
		vTaskDelay(portMAX_DELAY);
exit:
	if(audio)
	{
		/* stop record */
		audio_dev_record_stop(audio);

		/* 关闭 Audio 设备 */
		audio_dev_close(audio, AUDIO_FLAG_REPLAY);
	}

    if (wavrecord_buffer)
        vPortFree(wavrecord_buffer);
}

static void wavrecord_demo(void)
{
	if (xTaskCreate(wavrecord_thread, "wavrecord", configMINIMAL_STACK_SIZE, NULL,
			configMAX_PRIORITIES - 3, NULL) != pdPASS) {
		printf("create waverecord task fail.\n");
	}
}
#endif

#ifdef I2C_EEPROM_MASTER_DEMO
#define I2C_EEPROM_24C02_ADDR	0x50
#define I2C_EEPROM_WR_SIZE		8
static void eeprom_thread(void *param)
{
	struct i2c_adapter *adap = NULL;
	struct i2c_msg msg[2];
	unsigned char send_buf[I2C_EEPROM_WR_SIZE + 1];
	unsigned char rev_buf[I2C_EEPROM_WR_SIZE];
	int ret;
	u8 retries = 0;
	int i;

	if (!(adap = i2c_open("i2c0"))) {
		printf("open i2c0 fail.\n");
		return;
	}

	while(1) {
		/* write data */
		send_buf[0] = 0; /* word addr */
		printf("write data: ");
		for (i = 0; i < I2C_EEPROM_WR_SIZE; i++) {
			send_buf[1 + i] = rand() & 0xff;
			printf("%02x, ", send_buf[1 + i]);
		}
		printf("\n");
		msg[0].flags = !I2C_M_RD;
		msg[0].addr  = I2C_EEPROM_24C02_ADDR;
		msg[0].len   = I2C_EEPROM_WR_SIZE + 1;
		msg[0].buf   = send_buf;
		while (retries++ < 3) {
			ret = i2c_transfer(adap, msg, 1);
			if (ret == 1)
				break;
			else
				printf("i2c_transfer error %d.\n", ret);
		}

		memset(rev_buf, 0, sizeof(rev_buf));
		/* read data */
		msg[0].flags = !I2C_M_RD;
		msg[0].addr  = I2C_EEPROM_24C02_ADDR;
		msg[0].len   = 1;
		msg[0].buf   = send_buf;
		msg[1].flags = I2C_M_RD;
		msg[1].addr  = I2C_EEPROM_24C02_ADDR;
		msg[1].len   = I2C_EEPROM_WR_SIZE;
		msg[1].buf   = rev_buf;
		retries = 0;
		while (retries++ < 3) {
			ret = i2c_transfer(adap, msg, 2);
			if (ret == 2)
				break;
			else
				printf("i2c_transfer read error %d.\n", ret);
		}
		printf("read data: ");
		for (i = 0; i < I2C_EEPROM_WR_SIZE; i++)
			printf("%02x, ", rev_buf[i]);
		printf("\n");

		for (i = 0; i < I2C_EEPROM_WR_SIZE; i++) {
			if (send_buf[1 + i] != rev_buf[i])
				printf("i2c_transfer data[%d] err, 0x%02x, 0x%02x.\n", i, send_buf[1 + i], rev_buf[i]);
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

static void i2c_eeprom_demo(void)
{
	if (xTaskCreate(eeprom_thread, "eeprom", configMINIMAL_STACK_SIZE, NULL,
			configMAX_PRIORITIES - 4, NULL) != pdPASS) {
		printf("create eeprom task fail.\n");
	}
}
#endif


void lvgl_thread(void *data)
{
	printf("lvgl thread start.\n");

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
	carback_init();
#endif

	/* play animation */
#if ANIMATION_POLICY != ANIMATION_NONE
	animation_init();
	animation_start();
#endif

	/* uart rx demo */
	//uart_rx_demo();
#if BT_TEST
	fsc_bt_main();//feiyitong bt module
#endif

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
#if CARLINK_EY
    set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);	//设置屏幕显示区域（起始坐标和分辨率）
    set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);		//设置请求H264视频流参数
    carlink_ey_init();
#endif
#if CARLINK_EC
    set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
    set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
    carlink_ec_init(0, NULL);
#else
	//ark_network_init();
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

#ifdef AUDIO_REPLAY
	wavplay_demo();
#endif

#ifdef AUDIO_RECORD
	wavrecord_demo();
#endif

#if defined(I2C_EEPROM_SLAVE_DEMO) && (defined(I2C0_SLAVE_MODE) || defined(I2C1_SLAVE_MODE))
extern int i2c_slave_eeprom_init(void);
	i2c_slave_eeprom_init();
#endif

#ifdef I2C_EEPROM_MASTER_DEMO
	i2c_eeprom_demo();
#endif

#ifdef TP_SUPPORT
	extern int tp_init(void);
	tp_init();
#endif

    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL for LittlevGL*/
    hal_init();

#if defined(VG_DRIVER) && !defined(LVGL_VG_GPU)
	xm_vg_init((unsigned int)vgHeap, VG_HEAP_SIZE);
#endif

	lv_png_init();
	//lv_cpr_init();

    /*Check the themes too*/
    //lv_disp_set_default(lv_windows_disp);

    /*Run the v7 demo*/
    //lv_demo_widgets();
#if LV_USE_DEMO_PRINTER
    lv_demo_printer();
#else
#ifdef VG_DRIVER
#if defined(REVERSE_TRACK) || defined(LVGL_VG_GPU)
	haoke_demo();
#else
	xinbas_demo();
#endif
#else
	haoke_demo();
#endif
#endif


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
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(1));       /*Just to let the system breath*/
    }
}

#if LV_USE_DEMO_PRINTER

#if !(defined(ADC_TOUCH) || defined(TP_SUPPORT))
static inline void input_click(int x, int y)
{
	lv_indev_data_t indata;

	indata.point.x = x;
	indata.point.y = y;
	indata.state = LV_INDEV_STATE_PR;
	xQueueSend(touch_input_mq, &indata, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	xQueueSend(touch_input_mq, &indata, 0);
	vTaskDelay(pdMS_TO_TICKS(100));
	indata.state = LV_INDEV_STATE_REL;
	xQueueSend(touch_input_mq, &indata, 0);
}

void lvgl_input_thread(void *data)
{
	lv_indev_data_t indata;
	int i;

loop:
	/* click copy icon */
	vTaskDelay(pdMS_TO_TICKS(1000));
	input_click(250 * LCD_WIDTH / 1024, 250 * LCD_HEIGHT / 600);

	/* move brightness slider */
	vTaskDelay(pdMS_TO_TICKS(3000));
	indata.point.x = 648;
	indata.point.y = 240;
	indata.state = LV_INDEV_STATE_PR;
	xQueueSend(touch_input_mq, &indata, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	xQueueSend(touch_input_mq, &indata, 0);
	for (i = 1; i <= 16; i++) {
		indata.point.y = 240 - i * 2;
		xQueueSend(touch_input_mq, &indata, 0);
	}
	indata.state = LV_INDEV_STATE_REL;
	xQueueSend(touch_input_mq, &indata, 0);

	/* move hue slider */
	indata.point.x = 720;
	indata.point.y = 240;
	indata.state = LV_INDEV_STATE_PR;
	xQueueSend(touch_input_mq, &indata, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	xQueueSend(touch_input_mq, &indata, 0);
	for (i = 1; i <= 16; i++) {
		indata.point.y = 240 + i * 2;
		xQueueSend(touch_input_mq, &indata, 0);
	}
	indata.state = LV_INDEV_STATE_REL;
	xQueueSend(touch_input_mq, &indata, 0);

	/* click next icon */
	vTaskDelay(pdMS_TO_TICKS(4000));
	input_click(680, 408);

	/* click copies up button */
	vTaskDelay(pdMS_TO_TICKS(1000));
	indata.point.x = 940 * LCD_WIDTH / 1024;
	indata.point.y = 200 * LCD_HEIGHT / 600;
	for (i = 0; i < 8; i++) {
		indata.state = LV_INDEV_STATE_PR;
		xQueueSend(touch_input_mq, &indata, 0);
		vTaskDelay(pdMS_TO_TICKS(100));
		indata.state = LV_INDEV_STATE_REL;
		xQueueSend(touch_input_mq, &indata, 0);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	/* press color switch */
	vTaskDelay(pdMS_TO_TICKS(1000));
	input_click(870 * LCD_WIDTH / 1024 - 100, 360 * LCD_HEIGHT / 600);

	/* press vertical switch */
	vTaskDelay(pdMS_TO_TICKS(1000));
	input_click(1176 * LCD_WIDTH / 1024 - 200, 360 * LCD_HEIGHT / 600);

	/* click dpi dropdown */
	vTaskDelay(pdMS_TO_TICKS(1000));
	input_click(400 * LCD_WIDTH / 1024, 520 * LCD_HEIGHT / 600);
	vTaskDelay(pdMS_TO_TICKS(1000));
	input_click(318 * LCD_WIDTH / 1024, 420 * LCD_HEIGHT / 600);

	/* click print icon */
	vTaskDelay(pdMS_TO_TICKS(1000));
	input_click(848 * LCD_WIDTH / 1024, 500 * LCD_HEIGHT / 600);

	/* click continue icon */
	vTaskDelay(pdMS_TO_TICKS(3500));
	input_click(512 * LCD_WIDTH / 1024, 268 * LCD_HEIGHT / 600 + 200);

	vTaskDelay(pdMS_TO_TICKS(1000));
	goto loop;
}
#endif
#endif

#ifdef LVGL_VG_GPU
static void vg_init_thread(void *para)
{
	xm_vg_init((unsigned int)vgHeap, VG_HEAP_SIZE);

	for (;;)
		vTaskDelay(portMAX_DELAY);
}
#endif

void vRegisterSampleCLICommands( void );
void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );
void main_lvgl(void)
{
	/* Start the task that manages the command console for FreeRTOS+CLI. */
	vUARTCommandConsoleStart( ( configMINIMAL_STACK_SIZE * 3 ), tskIDLE_PRIORITY );

	/* Register the standard CLI commands. */
	vRegisterSampleCLICommands();
	/* Create a task to test driver */
#ifndef LVGL_VG_GPU
	xTaskCreate(lvgl_thread, "lvgl", 2048, NULL,
		tskIDLE_PRIORITY + 1, NULL);
#else
	xTaskCreate(vg_init_thread, "vginit", 2048, NULL,
		tskIDLE_PRIORITY + 1, NULL);
#endif

	touch_input_mq = xQueueCreate(INPUT_QUEUE_LEN, sizeof(lv_indev_data_t));
	if (touch_input_mq == NULL) {
		printf("create touch input message queue fail.\n");
		return;
	}

	keypad_input_mq = xQueueCreate(INPUT_QUEUE_LEN, sizeof(lv_indev_data_t));
	if (keypad_input_mq == NULL) {
		printf("create keypad input message queue fail.\n");
		return;
	}

#if LV_USE_DEMO_PRINTER
#if !(defined(ADC_TOUCH) || defined(TP_SUPPORT))
	xTaskCreate(lvgl_input_thread, "lvglinput", 1024, NULL, 8, NULL);
#endif
#endif

	return;
}
/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool touch_input_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	xQueueReceive(touch_input_mq, data, 0);

	if (uxQueueSpacesAvailable(touch_input_mq) < INPUT_QUEUE_LEN)
		return true;
	else
    	return false;
}

static bool keypad_input_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	if (xQueueReceive(keypad_input_mq, data, 0) == pdPASS) {
		printf("keypad_input_read %d %s.\n", data->key, data->state ? "press" : "release");
	}

	if (uxQueueSpacesAvailable(keypad_input_mq) < INPUT_QUEUE_LEN)
		return true;
	else
    	return false;
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    /* Add a display
     * Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    arklcd_init();

#if !ARKLCD_DOUBLE_BUFFERED
	static lv_disp_buf_t disp_buf;
	/*static lv_color_t buf1[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];
	lv_disp_buf_init(&disp_buf, buf1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX / 10);*/
	static lv_color_t buf1[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];
	static lv_color_t buf2[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];
	lv_disp_buf_init(&disp_buf, buf1, buf2, LV_HOR_RES_MAX * LV_VER_RES_MAX / 10);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);			/*Basic initialization*/
	disp_drv.buffer = &disp_buf;
    disp_drv.flush_cb = arklcd_flush;
    lv_disp_drv_register(&disp_drv);
#endif

    /* Add the virtual input device */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);			/*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_input_read;			/*This function will be called periodically (by the library) to get the mouse position and state*/

    /*Set cursor. For simplicity set a HOME symbol now.*/
	/* LV_IMG_DECLARE(mouse_cursor_icon);
	lv_indev_t * indev_mouse;
	indev_mouse = lv_indev_drv_register(&indev_drv);
    lv_obj_t * mouse_cursor = lv_img_create(lv_disp_get_scr_act(NULL), NULL);
    lv_img_set_src(mouse_cursor, &mouse_cursor_icon);
    lv_indev_set_cursor(indev_mouse, mouse_cursor); */
	lv_indev_drv_register(&indev_drv);

	/* Add the key input device */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_KEYPAD;
	indev_drv.read_cb = keypad_input_read;
	lv_indev_drv_register(&indev_drv);

#if !LV_TICK_CUSTOM
	{
		rt_thread_t tid;

	    /* Tick init.
	     * You have to call 'lv_tick_handler()' in every milliseconds
	     * Create an thread to do this*/
		tid = rt_thread_create("lvgltick", tick_thread, RT_NULL, 1024,
			RT_THREAD_PRIORITY_MAX/2, 10);
		if (tid != RT_NULL) rt_thread_startup(tid);
	}
#endif
}

#if !LV_TICK_CUSTOM
/**
 * A task to measure the elapsed time for LittlevGL
 * @param data unused
 * @return never return
 */
static void tick_thread(void *data)
{
    while(1) {
        lv_tick_inc(1);
        vTaskDelay(pdMS_TO_TICKS(1));   /*Sleep for 1 millisecond*/
    }
}
#endif

#endif
