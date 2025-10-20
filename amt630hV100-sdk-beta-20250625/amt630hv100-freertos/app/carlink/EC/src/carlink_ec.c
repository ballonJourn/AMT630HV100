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
#include <stdbool.h>
#include <stdio.h>
#include "board.h"
#include "timer.h"
#include "sfud.h"
#include "carlink_common.h"
#include "carlink_video.h"
#include "config/hcn_config.h"
#include "log/hcn_log.h"
#include "carlink_cb/hcn_carlink_provide.h"

#if CARLINK_EC
#include "ECTiny.h"
#include "ECTypes.h"
#include "config/hcn_carlink_def.h"

#define ENABLE_EC_DASHCAM  0

#define printf_func   printf

#define FLASH_PRIV_TYPE_BYTE 		0x1000
#define EC_FLASH_OFFSET           	0X38000


static bool mInited = false;
static ECConfigHandle mECTinyCfg = NULL;
static IECCallBack* mECTinyCallback = NULL;
static IECVideoPlayer* mECTinyVideoPlayer = NULL;
static IECAccessFile accessFile;
static ECQRInfo qr_info;
extern int wps_connect_done;
static bool  g_ec_disable = false;

struct ICalinkEventCallbacks gCarlinkECEventCB;
//static IECAccessDevice bleDev;

static const uint8_t        ap_ucip_address[4] = {192, 168, 13, 1};
static const uint8_t        ucNetMask[4] = {255, 255, 255, 0};
//static const uint8_t        ucGatewayAddress[4] = {0, 0, 0, 0};
static uint8_t                gphone_type = 0;
static uint8_t   android_ap_connect_flag = 0;
static uint8_t   apple_connect_flag = 0;

#if ENABLE_EC_DASHCAM
int start_http_camera();
void stop_http_camera();
#endif
#ifdef AWTK
void set_qr_text_buf(const char *str);
#endif

#if 0
static void ec_notify_event(struct carlink_event *ev, enum CARLINK_EVENT_TYPE type, bool disable_filter)
{
	ev->link_type = CARLINK_ECLINK_WIRELESS;
	ev->disable_filter = disable_filter;
	ev->type = type;
	carlink_notify_event(ev);
}
#endif

void onECConnectStatus(ECConnectedStatus status, ECConnectedType type)
{
    printf("\r\nite onECConnectStatus:status=%d,type=%d", status, type);
    if (status == EC_CONNECT_STATUS_CONNECT_SUCCEED)
    {
        //EC_uploadNightModeStatus(gDayNight);
        EC_startMirror();

        //EC_startIperfTcpServer("192.168.43.103",11150);
        printf("\r\nEC start mirror\r\n");
        if (get_hcn_callback() && get_hcn_callback()->onHcnLinkConnect) {
            get_hcn_callback()->onHcnLinkConnect();
        }
    }
}

void onMirrorStatus(ECMirrorStatus status)
{
    printf("\r\nonMirrorStatus:status=%d\r\n", status);

#if ENABLE_EC_DASHCAM
	if (status == EC_MIRROR_STATUS_MIRROR_STARTED)
		start_http_camera();
#endif
}

void onECStatusMessage(ECStatusMessage status)
{
#if 0
    printf("\r\nITE onECStatusMessage:status=%d\r\n", status);
#endif
}

void onPhoneAppHUD(const ECNavigationHudInfo *data)
{
    if (get_hcn_callback() && get_hcn_callback()->onHcnEasyNavigation) {
        if (data) {
            hcnNavigationHudInfo navi_data;
            memset(&navi_data, 0, sizeof(navi_data));

            navi_data.status = data->status;
            navi_data.naviIcon = data->naviIcon;
            navi_data.destinationRemainingDistance =\
            data->destinationRemainingDistance;
            navi_data.roadRemainingDistance = data->roadRemainingDistance;
            navi_data.signalIntensity = data->signalIntensity;
            snprintf(navi_data.currentRoad,sizeof(navi_data.currentRoad), 
                    "%s", data->currentRoad);
            snprintf(navi_data.nextRoad, sizeof(navi_data.nextRoad), 
                    "%s", data->nextRoad);

            get_hcn_callback()->onHcnEasyNavigation(&navi_data);
        }
    }
}

void onPhoneAppMusicInfo(const ECAppMusicInfo *data)
{
    printf("\r\nonPhoneAppMusicInfo\r\n");
}

void onPhoneAppInfo(const void *data, uint32_t length)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("\r\nonPhoneAppInfo\r\n");
    hcn_log_info("\r\nonPhoneAppInfo data:%s\r\n", (const char *)data);
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

    if (get_hcn_callback() && get_hcn_callback()->onHcnLicenseStatus) {
        get_hcn_callback()->onHcnLicenseStatus(0);
    }
}

void onLicenseAuthSuccess(int32_t code, const char *msg)
{
    printf("\r\nonLicenseAuthSuccess\r\n");

    if (get_hcn_callback() && get_hcn_callback()->onHcnLicenseStatus) {
        get_hcn_callback()->onHcnLicenseStatus(1);
    }
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
    if (data) {
        printf("weather data:%s\r\n", data);
        if (get_hcn_callback() && get_hcn_callback()->onHcnWeatherReceived) {
            get_hcn_callback()->onHcnWeatherReceived(data);
        }
    }
}

void onVRTipsReceived(const char *data, int32_t length)
{
    printf("\r\nonVRTipsReceived\r\n");
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
}


void onRequestBuildNetCancel()
{
}

void onPhoneBuildNetFinish(const char* ip)
{
    printf("\r\n[ouchunhua]onPhoneBuildNetFinish ip=%s\r\n", ip);

    struct carlink_event ev = {0};
    ev.type = CARLINK_EVENT_BUILD_NET_OK;
    ev.link_type = CARLINK_ECLINK;

	carlink_notify_event(&ev);
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
#if DISABLE_CARLINK_H264_FRAME_BUF
static void* gECVideoHandle;
#endif
void    vp_start(int32_t width, int32_t height)
{
    printf("\r\nstart VideoPlayer,width=%d,height=%d\r\n", width, height);
#if !DISABLE_CARLINK_H264_FRAME_BUF  
    h264_dec_ctx_init();
    //gts = get_timer(0);
    set_carlink_display_state(1);
#else
	gECVideoHandle = h264_video_player_init();
#endif
    if (get_hcn_callback() && get_hcn_callback()->onHcnVideoStatus) {
        get_hcn_callback()->onHcnVideoStatus(1);
    }
}

void    vp_stop()
{
    //video_frame_s* frame = NULL;
    printf("\r\nstop VideoPlayer\r\n");
#if !DISABLE_CARLINK_H264_FRAME_BUF  
    set_carlink_display_state(0);
#else
	h264_video_player_uninit(gECVideoHandle);
#endif
    if (get_hcn_callback() && get_hcn_callback()->onHcnVideoStatus) {
        get_hcn_callback()->onHcnVideoStatus(0);
    }
}

void    vp_play(const void *data, uint32_t read_len)
{
    //printf("\r\nplay VideoPlayer len = %d ts:%d ms\r\n", read_len, (get_timer(0) -gts) / 1000);
    //gts = get_timer(0);
#if !DISABLE_CARLINK_H264_FRAME_BUF  
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
#else
	h264_video_player_proc(gECVideoHandle, data, read_len);
#endif
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

static int32_t ec_access_read(const char* name,void *data, uint32_t length, uint32_t offset)
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

static int32_t ec_access_write(const char* name,void *data, uint32_t length, uint32_t offset)
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

static void setNetInterfaceInfo()
{
	struct ECNetInterfaceInfo info;
	uint32_t ulIPAddress = 0, netMask = 0;

	memset(&info, 0, sizeof(info));
	strcpy(info.name, "wlan0");
	if (gphone_type == 1)
		ulIPAddress = (ap_ucip_address[0]) | (ap_ucip_address[1] << 8) | (ap_ucip_address[2] << 16) | (ap_ucip_address[3] << 24);
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

static void onEventEC(void* ctx, const struct carlink_event *ev)
{
	enum CARLINK_EVENT_TYPE type;
	
	if (NULL == ev)
		return;
	if (ev->link_type != CARLINK_ECLINK_WIRELESS && !ev->disable_filter)// skip not ec event
		return;

	type = ev->type;

	switch (type) {
        case -1: {
            break;
        }

        case CARLINK_EVENT_INIT_DONE:
            break;

        case CARLINK_EVENT_BT_CONNECT: {
            printf("TRACE[%s][%d]:EC_BT_CONNECT_1\r\n",__func__ ,__LINE__);
            printf("TRACE[%s][%d]:EC_BT_CONNECT_2\r\n",__func__ ,__LINE__);
            break;
        }

        case CARLINK_EVENT_BT_AA_RFCOMM_READY: {
            break;
        }

        case CARLINK_EVENT_BT_DISCONNECT: {
            printf("TRACE[%s][%d]:EC_BT_DISCONNECT\r\n",__func__ ,__LINE__);
            break;
        }

        case CARLINK_EVENT_CLIENT_DHCP_READY: {
            if (gphone_type == 1) {
                if(apple_connect_flag == 1) {
                    printf("TRACE[%s][%d]:dhpc has been ready\r\n",__func__ ,__LINE__);
                    break;
                }
                apple_connect_flag = 1;
			}

            ECNetWorkInfo netInfo = {0};
            netInfo.state = EC_WIFI_STATE_CONNECTED;
            EC_notifyWifiStateChanged(EC_WIFI_STATE_CHANGED_ACTION, &netInfo);
            setNetInterfaceInfo();
            printf("TRACE[%s][%d]:EC_EVENT_CLIENT_DHCP_READY\r\n",__func__ ,__LINE__);

            break;
        }

        case CARLINK_EVENT_BUILD_NET_OK: {
            if (gphone_type == 1)
            {
                ECNetWorkInfo netInfo = {0};
                netInfo.state = EC_WIFI_STATE_CONNECTED;
                EC_notifyWifiStateChanged(EC_WIFI_STATE_CHANGED_ACTION, &netInfo);
                apple_connect_flag = 0;
            }

            setNetInterfaceInfo();
            printf("TRACE[%s][%d]:EC_EVENT_BUILD_NET_OK--------------\r\n", __func__, __LINE__);
            break;
        }

        case CARLINK_EVENT_WIFI_DISCONNECT: {
            if (android_ap_connect_flag == 1) {
                ECNetWorkInfo netInfo = {0};
                netInfo.state = EC_WIFI_STATE_DISCONNECTED;
                EC_notifyWifiStateChanged(EC_WIFI_STATE_CHANGED_ACTION, &netInfo);
                printf("TRACE[%s][%d]:EC_EVENT_WIFI_DISCONNECT--------------\r\n", __func__, __LINE__);
            }
			android_ap_connect_flag = 0;
            break;
        }

        case CARLINK_EVENT_WIFI_CONNECT: {
            android_ap_connect_flag = 1;
            printf("TRACE[%s][%d]:EC_EVENT_WIFI_CONNECT--------------\r\n", __func__, __LINE__);
            break;
        }

        case CARLINK_EVENT_AP_DISCONNECT: {
            printf("TRACE[%s][%d]:EC_EVENT_AP_DISCONNECT--------------\r\n", __func__, __LINE__);
            break;
        } 

        case CARLINK_EVENT_AP_CONNECT: {
            printf("TRACE[%s][%d]:EC_EVENT_AP_CONNECT--------------\r\n", __func__, __LINE__);
            break;
        }

        case CARLINK_EVENT_MSG_SESSION_CONNECT: {
            break;
        }

        case CARLINK_EVENT_MSG_SESSION_STOP: {
            break;
        }
        
        default:
        break;
	}
}

static void ec_ble_data_read(void* ctx, const void* buf, int len)
{
	(void)ctx;
	(void)buf;
	(void)len;
}

void* initECTiny(void* param)
{
    char uuid[32] = {0};
	const char *bt_mac = carlink_get_bt_mac();

	if (NULL == bt_mac) {
		printf("\r\n------ bt not ready ------\r\n");
		return NULL;
	}
	
    if (mInited)
    {
        printf("\r\n------ ECTiny has inited ------\r\n");
		return NULL;
    }

    printf("\r\n-------- Sample begin----------\r\n");

    printf("\r\n1.inti ECTiny config\r\n");
    mECTinyCfg = EC_createECConfig();

	memset(&qr_info, 0, sizeof(ECQRInfo));
	qr_info.action = EC_QR_ACTION_WIFI_P2P_MODE | EC_QR_ACTION_WIFI_AP_MODE_CUSTOMIZED;
	sprintf(qr_info.name, "%s", carlink_get_wifi_p2p_name());
	sprintf(qr_info.ssid, "%s", carlink_get_wifi_ssid());
	sprintf(qr_info.pwd, "%s", carlink_get_wifi_passwd());
	sprintf(qr_info.auth, "WPA-PSK");

	sprintf(uuid, "CARBIT%s", bt_mac);
	printf("carbit  uuid :%s\r\n", uuid);

#ifdef HCN_CARLINK_PROTOTYPE_MODE
    memset(uuid, 0, sizeof(uuid));
    hcn_log_info("use prototype mode uuid:%s\r\n", HCN_CHINESE_UUID);
    snprintf(uuid, sizeof(uuid), "%s", HCN_CHINESE_UUID);
#endif

	//EC_setBaseConfig(mECTinyCfg, uuid, "V0.0.1", "B:/");  //"CARBIT00000001"
    EC_setBaseConfig(mECTinyCfg, uuid, ECSDK_VERSION, "B:/");

    int32_t value = EC_SUPPORT_CONNECT_WIFIDIRECT;
    EC_setCommonConfig1(mECTinyCfg, "car_supportConnect", value);

    printf("\r\n2.register ECTiny callback functions\r\n");
    mECTinyCallback = registerECCallback();

    printf("\r\n3.register ECTiny Video player\r\n");
    mECTinyVideoPlayer = registerVideoPlayer();

    printf("\r\n4.set Mirror config\r\n");
    ECMirrorConfig mirrorCfg;
    memset(&mirrorCfg, 0, sizeof(ECMirrorConfig));

    mirrorCfg.width = get_carlink_video_width();
    mirrorCfg.height = get_carlink_video_height();
    mirrorCfg.height = ((mirrorCfg.height + 0xf) & (~0xf));

    mirrorCfg.touchMode = 2;
    mirrorCfg.type = EC_VIDEO_TYPE_H264;
    mirrorCfg.quality = 4 * 1024 * 1024;
    mirrorCfg.capScreenMode = 0x08;

#ifdef HCN_CARLINK_SAFEAREA_ENABLE
    mirrorCfg.screenPhysicsWidth = HCN_EC_screenPhysicsWidth;
    mirrorCfg.screenPhysicsHeight = HCN_EC_screenPhysicsHeight;

     /**
        ECVideoView ec_v_view = {
        0,
        {1, 0, 0, HCN_LCD_EC_WIDTH, HCN_LCD_EC_HEIGHT}, // viewArea
        {1, 98, 147, 348, 270}  // safeArea
        mirrorCfg.viewConfig.viewGroup = &ec_v_view;    
        }; 
        **/
    
    ECVideoView ec_v_view = {
        0,
        {HCN_EC_VIDEO_viewArea_present, HCN_EC_VIDEO_viewArea_originXPixels, 
            HCN_EC_VIDEO_viewArea_originYPixels, HCN_EC_VIDEO_viewArea_widthPixels, 
            HCN_EC_VIDEO_viewArea_heightPixels}, // viewArea

        {HCN_EC_VIDEO_safeArea_present, HCN_EC_VIDEO_safeArea_originXPixels, 
        HCN_EC_VIDEO_safeArea_originYPixels, HCN_EC_VIDEO_safeArea_widthPixels, 
        HCN_EC_VIDEO_safeArea_heightPixels}  // safeArea
    };

    mirrorCfg.viewConfig.viewGroup = &ec_v_view;
    mirrorCfg.viewConfig.initArea = HCN_EC_VIEW_CONFIG_initArea;
    mirrorCfg.viewConfig.viewCount = HCN_EC_VIEW_CONFIG_viewCount;
#endif

    printf("\r\n5.init ECTiny\r\n");

	accessFile.size = ec_access_size;
	accessFile.read = ec_access_read;
	accessFile.write = ec_access_write;
	accessFile.clear = ec_access_clear;
	EC_bindAccessFile(&accessFile);

    //EC_bindAccessFile(&license2File);
    EC_initialize(mECTinyCfg, mECTinyCallback);

    printf("\r\n6.set log info\r\n");

#ifdef HCN_DEBUG_CARLINK_LOG_LEVEL
	EC_setLogInfo(EC_LOG_LEVEL_DEBUG, EC_LOG_OUT_STD, NULL, EC_LOG_MODULE_SDK | EC_LOG_MODULE_APP);
#else
    EC_setLogInfo(EC_LOG_LEVEL_ERROR, EC_LOG_OUT_STD, NULL, EC_LOG_MODULE_SDK | EC_LOG_MODULE_APP);
#endif

    printf("\r\n7.set Mirror config\r\n");
    EC_setMirrorConfig(&mirrorCfg);
    EC_setVideoPlayer(mECTinyVideoPlayer);

	mInited = true;

    printf("\r\n8.start EC work!\r\n");

	if (!g_ec_disable)
    	EC_start();

    printf("\r\n9.bindBtDevice!\r\n");
    

    printf("------ initECTiny end ------\n");
    printf("carbit  uuid :%s\r\n", uuid);
    printf("carbit  uuid :%s\r\n", uuid);
	const char *UrlData = EC_generateQRCodeUrl(&qr_info);
	printf("++++++++++++++++++++++UrlData:%s+++++++++++++++++++++++++++\n", UrlData);

#ifdef AWTK
	set_qr_text_buf(UrlData);
#endif

    return NULL;
}

void testThread()
{
	int ret = carlink_common_init();
	ret = carlink_bt_wifi_init();

	gCarlinkECEventCB.onEvent = onEventEC;
	gCarlinkECEventCB.rfcomm_data_read = ec_ble_data_read;
	carlink_register_event_callbacks(&gCarlinkECEventCB);
	initECTiny(NULL);
}

static void  taskInitCarlink(void* param)
{
	testThread();
	vTaskDelete(NULL);
}

int carlink_ec_init(int argc,char ** argv)
{
	printf("------ TestECTiny begin ------\n");
	carlink_ey_video_init();
	
	/* 启动线程初始化：
	 * 1. initECTiny初始化延时过长，导致开机动画和UI显示切换之间存在黑屏时间，因此在线程中初始化
	 * 2. wifi模块未贴或异常时，wifi_init函数无法返回，导致UI无法正常显示。
	 */
	xTaskCreate(taskInitCarlink, "initThread", 2048 *4, NULL, configMAX_PRIORITIES / 4, NULL);
	printf("------ TestECTiny end ------\n");

	return 0;
}

void carlink_ec_enable(int enable)
{
	if (enable) {
		if (g_ec_disable) {
			if (mInited)
				EC_start();
		}
		g_ec_disable = false;
	} else {
		if (!g_ec_disable) {
			if (mInited)
				EC_stop();
		}
		g_ec_disable = true;
	}
}


#else
int carlink_ec_init(int argc,char ** argv)
{
    (void)argc;
    (void)argv;
    return 0;
}

void carlink_ec_enable(int enable)
{
	(void)enable;
}

#endif

