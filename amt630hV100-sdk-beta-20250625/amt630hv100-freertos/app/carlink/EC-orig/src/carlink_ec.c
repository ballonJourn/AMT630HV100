#include "carlink_ec.h"
#include "os_adapt.h"
#include <FreeRTOS_POSIX.h>
#include <task.h>
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DHCP.h"
#include "FreeRTOS_DHCP_Server.h"
#include <FreeRTOS_IP.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <FreeRTOS_Sockets.h>
#include <stdbool.h>
#include <stdio.h>
#include "board.h"
#include "timer.h"
#include "iot_wifi.h"
#include "mmcsd_core.h"
#include "carlink_video.h"
#include "carlink_utils.h"
#include "console.h"
#include "fsc_bt.h"
#include "sfud.h"

//#define CARLINK_EC 1
#if CARLINK_EC
//------ ECTiny code begin ------
#define USE_ECTINY_CODE			/*测试 ECTiny 代码 */

#ifdef USE_ECTINY_CODE
#ifdef  __cplusplus
extern "C" {
#endif

#include "ECTiny.h"
#include "ECTypes.h"

#ifdef  __cplusplus
};
#endif

#define AP_NO_PASSWD    0

#define printf_func   printf

#define FLASH_PRIV_TYPE_BYTE 0x1000
#define EC_FLASH_OFFSET           0X38000


#define EC_EVENT_BT_CONNECT               	0
#define EC_EVENT_BT_DISCONNECT            1
#define EC_EVENT_BT_DATA            		2
#define EC_EVENT_CLIENT_DHCP_READY	3
#define EC_EVENT_BUILD_NET_OK			4
#define EC_EVENT_WIFI_DISCONNECT			5
#define EC_EVENT_WIFI_CONNECT			    6

struct ec_event
{
	int type;
	union {
		int len;
		void* data;
		uint8_t para[32];
	}u;
};

static TaskHandle_t       ec_network_task = NULL;
static TaskHandle_t ec_ap_task = NULL;
static TaskHandle_t ec_sta_task = NULL;
static SemaphoreHandle_t ec_sta_sem = NULL;
static bool g_wifi_sta_inited = false;
static bool mInited = false;
static bool g_bt_mac_ready = false;
static char g_bt_mac[13] = {0};
static ECConfigHandle mECTinyCfg = NULL;
static IECCallBack* mECTinyCallback = NULL;
static IECVideoPlayer* mECTinyVideoPlayer = NULL;

static const uint8_t        ucIPAddress[4] = {192, 168, 13, 1};
static const uint8_t        ucNetMask[4] = {255, 255, 255, 0};
static const uint8_t        ucGatewayAddress[4] = {0, 0, 0, 0};
static const uint8_t        ucDNSServerAddress[4] = {8, 8, 8, 8};
static const uint8_t        ucMACAddress[6] = {0x30, 0x4a, 0x26, 0x78, 0xfd, 0x12};
static uint8_t                ap_ssid[64]     = {"ap63011"};
static uint8_t                ap_passwd[16]  = {"88888888"};
//static int g_dhcp_ready = 0;
static char g_ip_str[32] = {0};
static uint8_t                gphone_type = 0;
static ring_buffer_t ble_ring_data;
static QueueHandle_t     ec_event_queue = NULL;
static IECAccessDevice bleDev;
static IECAccessFile accessFile;
ECBTNetInfo gNetDeviceInfo = {0};
static ECQRInfo qr_info;

//extern uint8_t gDayNight;
static void ec_reset_wifi_ap_info(const char *btmac);


pthread_mutex_t bufLocker =     {                                        \
        .xIsInitialized = pdFALSE,           \
        .xMutex = { { 0 } },                 \
        .xTaskOwner = NULL,                  \
        .xAttr = { .iType = 0 }              \
    };

void onECConnectStatus(ECConnectedStatus status, ECConnectedType type)
{
    printf("\r\nite onECConnectStatus:status=%d,type=%d", status, type);
    if (status == EC_CONNECT_STATUS_CONNECT_SUCCEED)
    {
        //EC_uploadNightModeStatus(gDayNight);
        EC_startMirror();

        //EC_startIperfTcpServer("192.168.43.103",11150);
    }
}

void onMirrorStatus(ECMirrorStatus status)
{
    printf("\r\nonMirrorStatus:status=%d\r\n", status);
}

void onECStatusMessage(ECStatusMessage status)
{
    printf("\r\nITE onECStatusMessage:status=%d\r\n", status);
}

void onPhoneAppHUD(const ECNavigationHudInfo *data)
{
    printf("\r\nECNavigationHudInfo\r\n");
}

void onPhoneAppMusicInfo(const ECAppMusicInfo *data)
{
    printf("\r\nonPhoneAppMusicInfo\r\n");
}

void onPhoneAppInfo(const void *data, uint32_t length)
{
    printf("\r\nonPhoneAppInfo\r\n");
}

void onCallAction(ECCallType type, const char *name, const char *number)
{
    printf("\r\nonCallAction\r\n");
}

void onBulkDataReceived(const void *data, uint32_t length)
{
    printf("onBulkDataReceived\r\n");
}

void onMirrorInfoChanged(const ECVideoInfo *info)
{
    printf("\r\nonMirrorInfoChanged\r\n");
}

void onLicenseAuthFail(int32_t errCode, const char *errMsg)
{
    printf("\r\nonLicenseAuthFail\r\n");
}

void onLicenseAuthSuccess(int32_t code, const char *msg)
{
    printf("\r\nonLicenseAuthSuccess\r\n");
}

void onCarCmdNotified(const ECCarCmd *carCmd)
{
    printf("\r\nonCarCmdNotified\r\n");
}

void onInputStart(const ECInputInfo *info)
{
    printf("\r\nonInputStart\r\n");
}

void onInputCancel()
{
    printf("\r\nonInputCancel\r\n");
}

void onInputSelection(int32_t start, int32_t stop)
{
    printf("\r\nonInputSelection\r\n");
}

void onInputText(const char *text)
{
    printf("\r\nonInputText\r\n");
}

void onVRTextReceived(const ECVRTextInfo *info)
{
    printf("\r\nonVRTextReceived\r\n");
}

void onPageListReceived(const ECPageInfo *pages, int32_t length)
{
    printf("\r\nonPageListReceived\r\n");
}

void onPageIconReceived(const ECIconInfo *icons, int32_t length)
{
    printf("\r\nonPageIconReceived\r\n");
}

void onWeatherReceived(const char *data, int32_t length)
{
    printf("\r\nonWeatherReceived\r\n");

}

void onVRTipsReceived(const char *data, int32_t length)
{
    printf("\r\nonVRTipsReceived\r\n");
}

void  start_ap_thd(void* param) {
    int channel = 36;
#ifdef AP_USE_AUTO_CHANNEL
    int autoChannel = 0;
    uint8_t channelNum = sizeof(channel_set)/sizeof(channel_set[0]);
    autoChannel = wext_get_auto_chl("wlan0",channel_set, channelNum);
    if (autoChannel >= channel_set[0] && autoChannel <= channel_set[channelNum-1])
    {
        channel = autoChannel;
    }
    printf("start_ap_thd channel:%d, autoChannel:%d\n", channel, autoChannel);
#endif
#if !AP_NO_PASSWD
    int ret = start_ap(channel, (const char *)ap_ssid, (const char *)ap_passwd, 1);
#else
    int ret = start_ap(channel, (const char *)ap_ssid, "", 0);
#endif
	printf(" start_ap_thd_ret=%d\r\n",ret);

	vTaskDelete(NULL);
}

void start_sta_thd(void* param)
{
	xSemaphoreTake(ec_sta_sem, portMAX_DELAY );
	printf("------ start_sta_thd ssid=[%s],pwd=[%s] ------\r\n",gNetDeviceInfo.ssid, gNetDeviceInfo.pwd);
	if (!g_wifi_sta_inited) {
        printf("start_sta_thd init\r\n");
		g_wifi_sta_inited = true;
		wifi_initialize(eWiFiModeStation);// load wifi driver
        xSemaphoreGive(ec_sta_sem);
	} else {
		char ssid[32] = {0};
    	char pwd[16]  = {0};
		int ret = -1;
		strcpy(ssid, gNetDeviceInfo.ssid);
		strcpy(pwd, gNetDeviceInfo.pwd);
        printf("start_sta_thd start\r\n");
        xSemaphoreGive(ec_sta_sem);
		ret = start_sta(ssid, pwd, 1);
		if (ret != 0) {
			printf("start sta failed\r\n");
			xSemaphoreTake(ec_sta_sem, portMAX_DELAY );
			memset(&gNetDeviceInfo, 0, sizeof(gNetDeviceInfo));
			xSemaphoreGive(ec_sta_sem);
		}
	}

	vTaskDelete(NULL);
}

void onRequestBuildNet(const ECBTClientInfo *clientInfo)
{
    printf("onRequestBuildNet-----------------------\r\n");
    printf("TRACE[%s][%d]:",__func__ ,__LINE__);
	ECBTRequestBuildNetRly rpy;
	memset((void *)&rpy.netDeviceInfo, 0, sizeof(rpy.netDeviceInfo));
	printf("packageName        %s\r\n", clientInfo->packageName);
	printf("phoneName           %s\r\n", clientInfo->phoneName);
	printf("phoneID               %s\r\n", clientInfo->phoneID);
	printf("phoneType            %d\r\n", clientInfo->phoneType);
	printf("version                 %d\r\n", clientInfo->version);
    printf("TRACE[%s][%d]:",__func__ ,__LINE__);
	if (clientInfo->phoneType == 0) {// android
		rpy.status = EC_BT_REQUEST_BUILD_NET_USE_PHONE_AP;
		gphone_type = 0;

        printf("TRACE[%s][%d]:",__func__ ,__LINE__);
        EC_setRequestBuildNetRly(clientInfo->phoneID, &rpy);
        printf("TRACE[%s][%d]:",__func__ ,__LINE__);

        printf("TRACE[%s][%d]:",__func__ ,__LINE__);
        xSemaphoreTake(ec_sta_sem, portMAX_DELAY );
        g_wifi_sta_inited = false;
        memset(&gNetDeviceInfo, 0, sizeof(gNetDeviceInfo));
        xSemaphoreGive(ec_sta_sem);
        xTaskCreate(start_sta_thd, "ecThread", 2048 *4, NULL, configMAX_PRIORITIES / 4, &ec_sta_task);
        printf("TRACE[%s][%d]:",__func__ ,__LINE__);

	} else if (clientInfo->phoneType == 1) {//iphone
            printf("------ onRequestBuildNet begin\r\n");
            gphone_type = 1;
            rpy.status = EC_BT_REQUEST_BUILD_NET_NEED_PHONE_BUILD;
#if !AP_NO_PASSWD
            strcpy(rpy.netDeviceInfo.ssid, (char *)ap_ssid);
            strcpy(rpy.netDeviceInfo.pwd, (char *)ap_passwd);
            strcpy(rpy.netDeviceInfo.auth, "WAP2");
#else
	     strcpy(rpy.netDeviceInfo.ssid, (char *)ap_ssid);
            //strcpy(rpy.netDeviceInfo.pwd, (char *)ap_passwd);
            //strcpy(rpy.netDeviceInfo.auth, "");
#endif

        printf("TRACE[%s][%d]:",__func__ ,__LINE__);
        EC_setRequestBuildNetRly(clientInfo->phoneID, &rpy);
        printf("TRACE[%s][%d]:",__func__ ,__LINE__);
            xTaskCreate(start_ap_thd, "ecThread", 2048 *4, NULL, configMAX_PRIORITIES / 4, &ec_ap_task);

            printf("------ onRequestBuildNet end\r\n");
	}
}


void onRequestBuildNetCancel()
{
}


void onPhoneBuildNetFinish()
{
	struct ec_event ev = {0};
	ev.type = EC_EVENT_BUILD_NET_OK;

	if (-1 != ev.type && NULL != ec_event_queue) {
		xQueueSend(ec_event_queue, &ev, 0);
	}
}

void  onPhoneAPInfo(const ECBTNetInfo* netDeviceInfo)
{
    printf("------ onPhoneAPInfo ------\r\n");
    printf("onPhoneAPInfo ssid=[%s]\r\n",netDeviceInfo->ssid);
    printf("onPhoneAPInfo pwd=[%s]\r\n",netDeviceInfo->pwd);
    printf("onPhoneAPInfo auth=[%s]\r\n",netDeviceInfo->auth);
    printf("onPhoneAPInfo mac=[%s]\r\n",netDeviceInfo->mac);
    printf("onPhoneAPInfo name=[%s]\r\n",netDeviceInfo->name);
    printf("onPhoneAPInfo action=[%u]\r\n",netDeviceInfo->action);
    printf("------ onPhoneAPInfo ------\r\n");

	xSemaphoreTake(ec_sta_sem, portMAX_DELAY );
	if (!memcmp(netDeviceInfo->ssid, gNetDeviceInfo.ssid, strlen(netDeviceInfo->ssid) + 1) && 
		!memcmp(netDeviceInfo->pwd, gNetDeviceInfo.pwd, strlen(netDeviceInfo->pwd) + 1)) {
		xSemaphoreGive(ec_sta_sem);
		printf("recv same ssid info \r\n");
		return;
	}

    memset(&gNetDeviceInfo,0,sizeof(gNetDeviceInfo));
    strcpy(gNetDeviceInfo.ssid,netDeviceInfo->ssid);
    strcpy(gNetDeviceInfo.pwd,netDeviceInfo->pwd);
	xSemaphoreGive(ec_sta_sem);

//    static bool bRunOnce = true;
//    if(bRunOnce) {
	//start_sta(gNetDeviceInfo.ssid, gNetDeviceInfo.pwd, 1);
	xTaskCreate(start_sta_thd, "ecThread", 2048 *4, NULL, configMAX_PRIORITIES / 4, NULL);
//        bRunOnce = false;
//    }
}


IECCallBack * registerECCallback()
{
	IECCallBack * callBack = (IECCallBack*)malloc(sizeof(IECCallBack));
	memset(callBack, 0, sizeof(IECCallBack));
	callBack->onECConnectStatus = onECConnectStatus;
	callBack->onMirrorStatus = onMirrorStatus;
	callBack->onECStatusMessage = onECStatusMessage;
	callBack->onPhoneAppHUD = onPhoneAppHUD;
	callBack->onPhoneAppInfo = onPhoneAppInfo;
	callBack->onPhoneAppMusicInfo = onPhoneAppMusicInfo;
	callBack->onCallAction = onCallAction;
	callBack->onBulkDataReceived = onBulkDataReceived;
	callBack->onMirrorInfoChanged = onMirrorInfoChanged;
	callBack->onLicenseAuthFail = onLicenseAuthFail;
	callBack->onLicenseAuthSuccess = onLicenseAuthSuccess;
	callBack->onCarCmdNotified = onCarCmdNotified;
	callBack->onInputStart = onInputStart;
	callBack->onInputCancel = onInputCancel;
	callBack->onInputSelection = onInputSelection;
	callBack->onInputText = onInputText;
	callBack->onVRTextReceived = onVRTextReceived;
	callBack->onPageListReceived = onPageListReceived;
	callBack->onPageIconReceived = onPageIconReceived;
	callBack->onWeatherReceived = onWeatherReceived;
	callBack->onVRTipsReceived = onVRTipsReceived;
	callBack->onRequestBuildNet = onRequestBuildNet;
	callBack->onRequestBuildNetCancel = onRequestBuildNetCancel;
	callBack->onPhoneBuildNetFinish = onPhoneBuildNetFinish;
	callBack->onPhoneAPInfo = onPhoneAPInfo;

    return callBack;
}
void unregisterECCallback(IECCallBack * callBack)
{
    if (callBack)
    {
        free(callBack);
        callBack = NULL;
    }
}

//static uint32_t gts = 0;
// video player begin
void    vp_start(int32_t width, int32_t height)
{
    printf("\r\nstart VideoPlayer,width=%d,height=%d\r\n", width, height);
    
    h264_dec_ctx_init();
    //gts = get_timer(0);
    set_carlink_display_state(1);
}

void    vp_stop()
{
    //video_frame_s* frame = NULL;
    printf("\r\nstop VideoPlayer\r\n");

    set_carlink_display_state(0);
}

void    vp_play(const void *data, uint32_t read_len)
{
    //printf("\r\nplay VideoPlayer len = %d ts:%d ms\r\n", read_len, (get_timer(0) -gts) / 1000);
    //gts = get_timer(0);
    video_frame_s* frame = NULL;

get_retry:
    frame = get_h264_frame_buf();
    if (NULL == frame) {
        //printf("h264 frame is empty\r\n");
        vTaskDelay(pdMS_TO_TICKS(10));
        goto get_retry;
        //continue;
    }

    memcpy(frame->cur, data, read_len);
    frame->len     = read_len;

    notify_h264_frame_ready(&frame);
}

IECVideoPlayer * registerVideoPlayer()
{
    IECVideoPlayer* videoPlayer = (IECVideoPlayer*)malloc(sizeof(IECVideoPlayer));
    videoPlayer->start = vp_start;
    videoPlayer->stop = vp_stop;
    videoPlayer->play = vp_play;

    return videoPlayer;
}

void unregisterVideoPlayer(IECVideoPlayer* player)
{
    if (player)
    {
        player->stop();

        free(player);
        player = NULL;
    }
}

// Ôö¼ÓÒÚÁªµÄÊý¾Ý¶ÁÐ´½Ó¿Ú
uint32_t ec_access_size(const char* name)
{
    printf_func("TRACE[%s][%d]:name=%s\r\n",__func__ ,__LINE__,name);

    int32_t ret = 0;
    char num[5] = {0};
    sfud_flash *sflash = sfud_get_device(0);
    if( SFUD_SUCCESS == sfud_read(sflash, EC_FLASH_OFFSET, 4, (uint8_t *)num) )
    {
        ret = atoi(num);
        if (ret >= FLASH_PRIV_TYPE_BYTE)
        {
          printf_func("TRACE[%s][%d]:ec_access_size sfud_read error,clear name:%s ret:%d\r\n",__func__ ,__LINE__,name,ret);
            ret = 0;
        }
    }
    else
    {
        ret = 0;
    }

    printf_func("TRACE[%s][%d]:name=[%s],ret=%d\r\n",__func__ ,__LINE__,name,ret);
	return ret;
}

static int32_t ec_access_read(const char* name,void *data, uint32_t length)
{
    printf_func("TRACE[%s][%d]1:len=%d\r\n",
           __func__ ,__LINE__,length);

    int32_t ret = 0;
    char num[5] = {0};
    uint8_t* readData = malloc(FLASH_PRIV_TYPE_BYTE);
    memset(readData,0,sizeof(FLASH_PRIV_TYPE_BYTE));
    sfud_flash *sflash = sfud_get_device(0);
    if( SFUD_SUCCESS == sfud_read(sflash, EC_FLASH_OFFSET, FLASH_PRIV_TYPE_BYTE, readData) )
    {
        memcpy(num,readData,4);
        ret = atoi(num);
        if(ret == length)
        {
            memcpy(data,readData+4,length);
        }
        else
        {
            ret = 0;
        }
    }

    printf_func("TRACE[%s][%d]2:data=[%s],ret=%d\r\n",
           __func__ ,__LINE__,(char*)data,ret);

    free(readData);

    return ret;
}

static int32_t ec_access_write(const char* name,void *data, uint32_t length)
{
    printf_func("TRACE[%s][%d]1:data=[%s],len=%d\r\n",
                __func__ ,__LINE__,(char*)data,length);

    int32_t  ret = 0;
    char* writeData = malloc(length+4);
    sprintf(writeData,"%04d%s",length, (char*)data);
    sfud_flash *sflash = sfud_get_device(0);
    sfud_erase(sflash, EC_FLASH_OFFSET, FLASH_PRIV_TYPE_BYTE);
    if( SFUD_SUCCESS == sfud_erase_write(sflash, EC_FLASH_OFFSET, length+4, (void*)writeData) )
    {
        ret = length;
    }
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
	sfud_flush(sflash);
#endif

    free(writeData);
    printf_func("TRACE[%s][%d]2:ret=%d\r\n",
                __func__ ,__LINE__,ret);
    return ret;
}

static void ec_access_clear(const char* name)
{
    sfud_flash *sflash = sfud_get_device(0);
    sfud_erase(sflash, EC_FLASH_OFFSET, FLASH_PRIV_TYPE_BYTE);
    printf_func("TRACE[%s][%d]1:name=[%s]\r\n",
                __func__ ,__LINE__,name);
}


void ec_wifi_event_handler( WIFIEvent_t * xEvent )
{
    WIFIEventType_t xEventType = xEvent->xEventType;

    if (0) {
    } else if (eWiFiEventConnected                    == xEventType) {// meter is sta
        printf("\r\n The meter is connected to ap \r\n");
		struct ec_event ev = {0};
		ev.type = EC_EVENT_WIFI_CONNECT;

		if (-1 != ev.type && NULL != ec_event_queue) {
			xQueueSend(ec_event_queue, &ev, 0);
		}
    } else if (eWiFiEventDisconnected                == xEventType) {// meter is sta
        printf("\r\n The meter is disconnected from ap \r\n");
		struct ec_event ev = {0};
		ev.type = EC_EVENT_WIFI_DISCONNECT;

		if (-1 != ev.type && NULL != ec_event_queue) {
			xQueueSend(ec_event_queue, &ev, 0);
		}
    } else if (eWiFiEventAPStationConnected       == xEventType) {// meter is ap
        printf("\r\n The meter in AP is connected by sta %s \r\n", xEvent->xInfo.xAPStationConnected.ucMac);
    } else if (eWiFiEventAPStationDisconnected   == xEventType) {// meter is ap
        printf("\r\n The sta %s is disconnected from the meter \r\n", xEvent->xInfo.xAPStationDisconnected.ucMac);
    }
}

#if ( ipconfigUSE_DHCP_HOOK != 0 )
eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase,
                                            uint32_t ulIPAddress )
{
	eDHCPCallbackAnswer_t eReturn;
	struct ec_event ev = {0};

	sprintf(g_ip_str, "%d.%d.%d.%d\r\n", (ulIPAddress >> 0) & 0xFF, 
	(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
	printf("\r\n  eDHCPPhase:%d  ulIPAddress:%s state:%d \r\n", eDHCPPhase, g_ip_str, getDhcpClientState());
	if (getDhcpClientState() == 0)
	return eDHCPStopNoChanges;

	switch( eDHCPPhase )
	{
	case eDHCPPhaseFinished:
		//g_dhcp_ready = 1;
		ev.type = EC_EVENT_CLIENT_DHCP_READY;

		if (-1 != ev.type && NULL != ec_event_queue) {
			xQueueSend(ec_event_queue, &ev, 0);
		}
		eReturn = eDHCPContinue;
	break;
	case eDHCPPhasePreDiscover  :
		eReturn = eDHCPContinue;
	break;

	case eDHCPPhasePreRequest  :
		eReturn = eDHCPContinue;
	break;
	default :
		eReturn = eDHCPContinue;
	break;
	}

	return eReturn;
}
#endif
int32_t ec_ble_write(void * data, uint32_t len);
int32_t ec_ble_read(void * data, uint32_t len);
int32_t ec_ble_open();
void ec_ble_close();
extern void set_qr_text_buf(const char *str);

void* initECTiny(void* param)
{
    char uuid[32] = {0};
    if (mInited)
    {
        printf("\r\n------ ECTiny has inited ------\r\n");
    }
    else
    {
        printf("\r\n-------- Sample begin----------\r\n");

        printf("\r\n1.inti ECTiny config\r\n");
        mECTinyCfg = EC_createECConfig();

        while(!g_bt_mac_ready) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        {
            char ap_prefix[5] = {0};
            memcpy(ap_prefix, g_bt_mac + 8, 4);
            ec_reset_wifi_ap_info(ap_prefix);
            printf("ap_ssid:%s\r\n", ap_ssid);
        }

        sprintf(uuid, "CARBIT%s", g_bt_mac);
        printf("carbit  uuid :%s\r\n", uuid);
        EC_setBaseConfig(mECTinyCfg, uuid, "V0.0.1", "B:/");  //"CARBIT00000001"

        printf("\r\n2.register ECTiny callback functions\r\n");
        mECTinyCallback = registerECCallback();

        printf("\r\n3.register ECTiny Video player\r\n");
        mECTinyVideoPlayer = registerVideoPlayer();

        printf("\r\n4.set Mirror config\r\n");
        ECMirrorConfig mirrorCfg;
        memset(&mirrorCfg, 0, sizeof(ECMirrorConfig));

        //mirrorCfg.width = 1280;
        //mirrorCfg.height = 480;
        mirrorCfg.width = get_carlink_video_width();
        mirrorCfg.height = get_carlink_video_height();
        mirrorCfg.height = ((mirrorCfg.height + 0xf) & (~0xf));

        mirrorCfg.touchMode = 2;
        mirrorCfg.type = EC_VIDEO_TYPE_H264;
        mirrorCfg.quality = 4 * 1024 * 1024;
        mirrorCfg.capScreenMode = 0x08;

        printf("\r\n5.init ECTiny\r\n");

        accessFile.size = ec_access_size;
        accessFile.read = ec_access_read;
        accessFile.write = ec_access_write;
        accessFile.clear = ec_access_clear;
        EC_bindAccessFile(&accessFile);

        //EC_bindAccessFile(&license2File);
        EC_initialize(mECTinyCfg, mECTinyCallback);

        printf("\r\n6.set log info\r\n");
        EC_setLogInfo(EC_LOG_LEVEL_INFO, EC_LOG_OUT_STD, NULL, EC_LOG_MODULE_SDK | EC_LOG_MODULE_APP);

        printf("\r\n7.set Mirror config\r\n");
        EC_setMirrorConfig(&mirrorCfg);
        EC_setVideoPlayer(mECTinyVideoPlayer);

        printf("\r\n8.start EC work!\r\n");
        EC_start();

        printf("\r\n9.bindBtDevice!\r\n");

        //sleep(1);
        //EC_bindWIFIDevice(EC_TRANSPORT_ANDROID_WIFI, "192.168.43.1");
        //EC_bindWIFIDevice(EC_TRANSPORT_IOS_WIFI_APP, "192.168.13.20");

        //        ECQRInfo info;
        //        memset(&info,0, sizeof(ECQRInfo));
        //        strcpy(info.ssid,"QJ501");
        //        strcpy(info.pwd,"88888888");
        //        strcpy(info.auth,"WPA");
        //        info.action = EC_QR_ACTION_WIFI_AP_MODE_ACCESS_INTERNET;
        //
        //        EC_generateQRCodeUrl(&info);

        mInited = true;

        //        while (1) {
        //            sleep(20);
        //        }
    }


    printf("------ initECTiny end ------\n");
    if (1) {
        char ap_prefix[5] = {0};
        memcpy(ap_prefix, g_bt_mac + 8, 4);

        memset(&qr_info, 0, sizeof(ECQRInfo));
        qr_info.action = EC_QR_ACTION_WIFI_STATION_MODE | EC_QR_ACTION_WIFI_AP_MODE_CUSTOMIZED;
        memcpy(ap_prefix, g_bt_mac + 8, 4);
        //sprintf(qr_info.name, "ark630hv100_p2p_%s", ap_prefix);
        sprintf(qr_info.ssid, "%s", ap_ssid);
        sprintf(qr_info.pwd, (char const *)ap_passwd);
        sprintf(qr_info.auth, "WPA-PSK");
        printf("ssid:%s\r\n", qr_info.ssid);
        const char *UrlData = EC_generateQRCodeUrl(&qr_info);
        printf("++++++++++++++++++++++UrlData:%s+++++++++++++++++++++++++++\n", UrlData);
        #ifdef AWTK
        set_qr_text_buf(UrlData);
        #endif
    }

    return NULL;
}
#endif

//------ ECTiny code end ------


void* thdPingPhone(void* param)
{
    int ret = 0;
    int count = 0;
    uint32_t IPAddress = (20 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
    while(true) {
        ret = FreeRTOS_SendPingRequest(IPAddress, 8, 1000);
        printf("[TRACE][%s][%d]:count=%d,ping ret=%d\n",__FUNCTION__,__LINE__,count++,ret);
        sleep(2);
    }
}

#if 0
void testThread()
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, initECTiny, NULL);
    pthread_attr_destroy(&attr);
}
#else
int mmcsd_wait_sdio_ready(int32_t timeout);
static int carlink_ec_network_init()
{
    BaseType_t ret;
    ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);

    return (int)ret;
}

static void ec_reset_wifi_ap_info(const char *btmac)
{
	memset(ap_ssid, 0, sizeof(ap_ssid));
    sprintf((char *)ap_ssid,   "AP630_%s", btmac);
}

static void ec_bt_callback(char * cAtStr)
{
	char* cmd_para = NULL;
	//printf("\r\nfsc_bt_callback_ec %s\r\n", cAtStr);
	static struct ec_event ev = {0};
        static char m_tmp_buf[64];
	memset((void*)&ev, 0, sizeof(ev));
	ev.type = -1;

	if (0) {
	} else if (0 == strncmp(cAtStr, "+GATTDATA=",      10)) {
        //printf("fsc_bt_callback_ec=[%s]\r\n", cAtStr);
        char ble_buf[256] = {0};

        pthread_mutex_lock(&bufLocker);
		int data_str_len = (strlen(cAtStr) - 10), data_len;

		ev.type = EC_EVENT_BT_DATA;
		cmd_para = cAtStr + 10;
		data_len = data_str_len / 2;
		while (ring_buffer_get_bytes_free(&ble_ring_data) < data_len) {
			pthread_mutex_unlock(&bufLocker);
			vTaskDelay(pdMS_TO_TICKS(10));
			pthread_mutex_lock(&bufLocker);
		}
		string2hex(cmd_para, data_str_len, ble_buf, data_len);
		ring_buffer_write(&ble_ring_data, (uint8_t *)ble_buf, data_len);
        pthread_mutex_unlock(&bufLocker);

	} else if (0 == strncmp(cAtStr, "+GATTSTAT=1",11)) {
		ev.type = EC_EVENT_BT_DISCONNECT;
        // btEvent = EC_EVENT_BT_DISCONNECT;
        printf("TRACE[%s][%d]:EC_EVENT_BT_DISCONNECT\r\n",__func__ ,__LINE__);
	} else if (0 == strncmp(cAtStr, "+GATTSTAT=3",11)) {
        printf("TRACE[%s][%d]:EC_EVENT_BT_CONNECT\r\n",__func__ ,__LINE__);
		ev.type = EC_EVENT_BT_CONNECT;
	} else if (0 == strncmp(cAtStr, "+ADDR=",       6)) {
		char cmd_str[64] = {0};
		sprintf(cmd_str, "AT+NAME=EC_%s\r\n", (cAtStr + 6));
		//ec_reset_wifi_ap_info(cAtStr + 6);
		printf("ADDR:%s\r\n", cAtStr + 6);
		console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
		memset(cmd_str, 0, sizeof(cmd_str));
		sprintf(cmd_str, "AT+LENAME=EC_%s\r\n", (cAtStr + 6));
		console_send_atcmd(cmd_str, strlen(cmd_str));//get mac addr
		g_bt_mac_ready = true;
		memcpy(g_bt_mac, (cAtStr + 6), 12);
	} else if (0 == strncmp(cAtStr, "+VER",         4)) {
		char* cmd = "AT+ADDR\r\n";
		console_send_atcmd(cmd, strlen(cmd));//get mac addr
    } else if (0 == strncmp(cAtStr, "+NAME=", 6)) {
		char cmd_str[64] = {0};
		sprintf(cmd_str, "AT+LEADDR\r\n");
		console_send_atcmd(cmd_str, strlen(cmd_str));//get LE addr
	} else if (0 == strncmp(cAtStr, "+LEADDR=", 6)) {
      char hexMacAddr[6] = {0};
      memset(m_tmp_buf, 0, sizeof(m_tmp_buf));
      string2hex(&cAtStr[8], 12, hexMacAddr, sizeof(hexMacAddr));
      //memcpy(addr, &cAtStr[8], 12);
      sprintf(m_tmp_buf, "AT+ADVDATA=%s\r\n", hexMacAddr);
      console_send_atcmd(m_tmp_buf, 19);
   }      
	else if (0 == strncmp(cAtStr, "+GATTSENT=", 10)) {
          
        return;
    }

	if (-1 != ev.type && NULL != ec_event_queue) {
		xQueueSend(ec_event_queue, &ev, 0);
	}
}

int32_t ec_ble_read(void * data, uint32_t len)
{
    pthread_mutex_lock(&bufLocker);
	int data_len = 0;

	data_len = ring_buffer_get_bytes_used(&ble_ring_data);

	if (data_len > len) {
		data_len = len;
	}
	if (data_len > 0) {
		ring_buffer_read(&ble_ring_data, (uint8_t*)data, data_len);
        	pthread_mutex_unlock(&bufLocker);
		return data_len;
	}
    pthread_mutex_unlock(&bufLocker);
	return 0;
}

static int ble_write(unsigned char *data, int len)
{
    uint8_t *at_str = NULL, *at_str_ptr = NULL;
    uint8_t *at_prefix = "AT+GATTSEND=";
    int prefix_len = strlen((char *)at_prefix);
    int cmd_len = (len * 2 + prefix_len + 2);

    at_str = (uint8_t *)pvPortMalloc(cmd_len+1);
    memset(at_str,0,cmd_len+1);
    if (NULL == at_str)
        return -1;
    at_str_ptr = at_str;
    sprintf((char*)at_str_ptr, "%s", (char*)at_prefix);
    at_str_ptr += prefix_len;
    hex2str(data, len, (char *)at_str_ptr);
    at_str_ptr 		+= (len * 2);
    *at_str_ptr++ 	= '\r';
    *at_str_ptr++		= '\n';
    console_send_atcmd((char *)at_str, cmd_len);
    vPortFree(at_str);
    return len;
}

int32_t ec_ble_write(void *data, uint32_t len)
{
    int send = 0;
    int offset = 0;
    do
    {
        // 把data每次按10个字节发送。ble_write 中组合之后长度变成34.  10 * 2 + 12 + 2
        send = (len - offset) > 10 ? 10 : (len - offset);
        offset += ble_write((unsigned char *)data + offset, send);
        //usleep(2000*1000);
    } while (offset < len);

    return offset;
}

int32_t ec_ble_open()
{
	return 0;
}

void ec_ble_close()
{

}

static BaseType_t wifi_init()
{
	int status;
	WIFI_Context_init();
	WIFI_RegisterEvent(eWiFiEventMax, ec_wifi_event_handler);
	for (;;) {
		status = mmcsd_wait_sdio_ready((int32_t)portMAX_DELAY);
		if (status == MMCSD_HOST_PLUGED) {
			printf("detect sdio device\r\n");
			break;
		}
	}
	return 0;
}

static void setNetInterfaceInfo()
{
	struct ECNetInterfaceInfo info;
	uint32_t ulIPAddress = 0, netMask = 0;

	memset(&info, 0, sizeof(info));
	strcpy(info.name, "wlan0");
	if (gphone_type == 1)
		ulIPAddress = (ucIPAddress[0]) | (ucIPAddress[1] << 8) | (ucIPAddress[2] << 16) | (ucIPAddress[3] << 24);
	else
		ulIPAddress = FreeRTOS_GetIPAddress();
	sprintf(info.ip, "%d.%d.%d.%d", (ulIPAddress >> 0) & 0xFF,
				(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
	if (gphone_type == 1)
		netMask = (ucNetMask[0]) | (ucNetMask[1] << 8) | (ucNetMask[2] << 16) | (ucNetMask[3] << 24);
	else
		netMask = FreeRTOS_GetNetmask();
	sprintf(info.mask, "%d.%d.%d.%d", (netMask >> 0) & 0xFF,
				(netMask >> 8) & 0xFF, (netMask >> 16) & 0xFF, (netMask >> 24) & 0xFF);
	EC_setNetInterfaceInfo(&info, 1);
}

static void  taskECTinyWrapper(void* param)
{
    struct ec_event ev;
	int    android_ap_connect_flag = 0;

    while(1) {
        if (xQueueReceive(ec_event_queue, &ev, portMAX_DELAY) != pdPASS) {
            printf("%s xQueueReceive err!\r\n", __func__);
            continue;
        }

        //printf("TRACE[%s][%d]:ec_event_queue,get event:%d\r\n", __func__, __LINE__, ev.type);

        switch (ev.type) {
            case -1: {
                continue;
            }
            case EC_EVENT_BT_CONNECT: {
                printf("TRACE[%s][%d]:EC_bindBTDevice_1\r\n",__func__ ,__LINE__);
                bleDev.open       = ec_ble_open;
                bleDev.close      = ec_ble_close;
                bleDev.read       = ec_ble_read;
                bleDev.write      = ec_ble_write;
                EC_bindBTDevice(&bleDev);
                printf("TRACE[%s][%d]:EC_bindBTDevice_2\r\n",__func__ ,__LINE__);

                break;
            }
            case EC_EVENT_BT_DISCONNECT: {
                break;
            }
            case EC_EVENT_BT_DATA: {

                break;
            }
                // wifi is station, dhcp ready
            case EC_EVENT_CLIENT_DHCP_READY: {
                    ECNetWorkInfo netInfo = {0};
                    netInfo.state = EC_WIFI_STATE_CONNECTED;
                    EC_notifyWifiStateChanged(EC_WIFI_STATE_CHANGED_ACTION, &netInfo);
                    setNetInterfaceInfo();
                    printf("TRACE[%s][%d]:EC_EVENT_CLIENT_DHCP_READY\r\n",__func__ ,__LINE__);

                break;
            }
            case EC_EVENT_BUILD_NET_OK: {
                if (gphone_type == 1)
                {
                    ECNetWorkInfo netInfo = {0};
                    netInfo.state = EC_WIFI_STATE_CONNECTED;
                    EC_notifyWifiStateChanged(EC_WIFI_STATE_CHANGED_ACTION, &netInfo);
                }

                setNetInterfaceInfo();
                printf("TRACE[%s][%d]:EC_EVENT_BUILD_NET_OK--------------\r\n", __func__, __LINE__);
                break;
            }
            case EC_EVENT_WIFI_DISCONNECT:
            {
            	if (android_ap_connect_flag == 1) {
	                ECNetWorkInfo netInfo = {0};
	                netInfo.state = EC_WIFI_STATE_DISCONNECTED;
	                EC_notifyWifiStateChanged(EC_WIFI_STATE_CHANGED_ACTION, &netInfo);
	                printf("TRACE[%s][%d]:EC_EVENT_WIFI_DISCONNECT--------------\r\n", __func__, __LINE__);
            	}
				android_ap_connect_flag = 0;
                break;
            }
			case EC_EVENT_WIFI_CONNECT:
            {
                android_ap_connect_flag = 1;
                printf("TRACE[%s][%d]:EC_EVENT_WIFI_CONNECT--------------\r\n", __func__, __LINE__);
                break;
            }
            default:
                printf("TRACE[%s][%d]:error event:%d\r\n", __func__, __LINE__, ev.type);
        }
    }

    vTaskDelete(NULL);
}

void testThread()
{
	wifi_init();
	ring_buffer_init(&ble_ring_data, 256);
	ec_event_queue = xQueueCreate(1, sizeof(struct ec_event));
	if (ec_sta_sem == NULL)
		ec_sta_sem = xSemaphoreCreateMutex();
	carlink_ec_network_init();
	console_register_cb(NULL, ec_bt_callback);
	xTaskCreate(taskECTinyWrapper, "ecThread", 2048 *4, NULL, configMAX_PRIORITIES / 4, &ec_network_task);
	fsc_bt_main();printf("%s:%d\r\n", __func__, __LINE__);

	initECTiny(NULL);
}
#endif

static void  taskInitCarlink(void* param)
{
	testThread();
	vTaskDelete(NULL);
}

int carlink_ec_init(int argc,char ** argv)
{
	printf("------ TestECTiny begin ------\n");
	carlink_ey_video_init();
#if 0
    testThread();
#else
	/* 启动线程初始化：
	 * 1. initECTiny初始化延时过长，导致开机动画和UI显示切换之间存在黑屏时间，因此在线程中初始化
	 * 2. wifi模块未贴或异常时，wifi_init函数无法返回，导致UI无法正常显示。
	 */
	xTaskCreate(taskInitCarlink, "initThread", 2048 *4, NULL, configMAX_PRIORITIES / 4, NULL);
#endif
	printf("------ TestECTiny end ------\n");

	return 0;
}
#else
int carlink_ec_init(int argc,char ** argv)
{
    (void)argc;
    (void)argv;
    return 0;
}
#endif
