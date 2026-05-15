#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


#include "iap.h"
#include "carplay.h"
#include "audio_callbacks.h"
#include "video_callbacks.h"
#include "carlink_cp.h"
//#include "DispatchLite.h"
#include "mycommon.h"
#include "ff_sfdisk.h"
#include "carlink_video.h"
#include "carlink_cp_priv.h"

#include "os_adapt.h"
#include <FreeRTOS_POSIX.h>
#include <task.h>
#include "carlink_common.h"
#include "board.h"


#if CARLINK_CP


struct carplay_ctx
{
	struct ICalinkEventCallbacks carlinkEventCB;

	int   mPhoneCarplayFlag;
	bool   mInitDone;
	bool   mBTConnected;
	bool   mIapReady;
	bool   mCarplayConnected;
	char   mLocalBTMac[6];
	char   mRemoteBTMac[6];
	char   mIp[32];
	char   mPhoneWifiMac[18];
	void*  mVideoHandle;
};

struct carplay_ctx g_cp_handle;
static bool  g_cp_disable = false;


static void carplay_init_parameter();
void start_mdnsd();
void start_mdnsd_posix();
void stop_mdnsd_posix();

#if (DEVICE_TYPE_SELECT == SPI_NOR_FLASH) || (DEVICE_TYPE_SELECT == SPI_NAND_FLASH)
#include "sfud.h"
//根据spi nor flash实际使用情况分配
#ifndef CARLINK_CP_OFFSET
#define CARLINK_CP_OFFSET 0X39000
#endif

#ifndef CARLINK_CP_SIZE
#define CARLINK_CP_SIZE 0x2000
#endif

static int g_sf_read_ptr;
static int32_t  cp_sf_op_flash(void* flash_handle, void* ctx, int open)
{
	g_sf_read_ptr = 0;
	return 0;
}

static int32_t  cp_sf_read_data(void* flash_handle, void *data, uint32_t length, uint32_t offset, void* ctx)
{
	printf("TRACE[%s][%d] start:len=%d\r\n", __func__ ,__LINE__,length);
	
	int32_t ret = 0;
	char num[5] = {0};
	uint8_t* readData = NULL;

	if (g_sf_read_ptr > CARLINK_CP_SIZE)
		return 0;

	readData = malloc(CARLINK_CP_SIZE);
	memset(readData, 0, CARLINK_CP_SIZE);
	sfud_flash *sflash = sfud_get_device(0);
	if (SFUD_SUCCESS == sfud_read(sflash, CARLINK_CP_OFFSET, CARLINK_CP_SIZE, readData)) {
		memcpy(num, readData, 4);
		ret = atoi(num);
		if(ret > 0) {
			if (ret > length)
				ret = length;
			if (ret > CARLINK_CP_SIZE)
				ret = (CARLINK_CP_SIZE - 4);
			memcpy(data, readData + 4, ret);
			g_sf_read_ptr += CARLINK_CP_SIZE;
		} else {
			ret = 0;
		}
	}

	free(readData);
	printf("TRACE[%s][%d] end: ret=%d\r\n", __func__ ,__LINE__,ret);
	return ret;

}

static int32_t  cp_sf_write_data(void* flash_handle, void *data, uint32_t length, uint32_t offset, void* ctx)
{
	int ret = 0;
	char* writeData = NULL;

	if ((length + 4) > CARLINK_CP_SIZE)
		length = CARLINK_CP_SIZE - 4;

	writeData = malloc(length + 4);

	printf("TRACE[%s][%d] start: len=%d\r\n", __func__ ,__LINE__, length);
    sprintf(writeData, "%04d", length);
	memcpy(writeData + 4, data, length);
	
    sfud_flash *sflash = sfud_get_device(0);
    sfud_erase(sflash, CARLINK_CP_OFFSET, CARLINK_CP_SIZE);
    if (SFUD_SUCCESS == sfud_erase_write(sflash, CARLINK_CP_OFFSET, length + 4, (void*)writeData)) {
        ret = length;
    }

    free(writeData);
	printf("TRACE[%s][%d] end:ret=%d\r\n", __func__ ,__LINE__,ret);
	return ret;
}
#endif

static void carplay_notify_event(struct carlink_event *ev, enum CARLINK_EVENT_TYPE type, bool disable_filter)
{
	ev->link_type = CARLINK_CARPLAY_WIRELESS;
	ev->disable_filter = disable_filter;
	ev->type = type;
	carlink_notify_event(ev);
}

static void iap2_link_status(void *ctx, IAP2_LINK_STATUS status){}
static int iap2_write_data(void *ctx, char *buf, int len)
{
	(void)ctx;
	return carlink_iap_data_write((unsigned char *)buf, len);
}
static void iap2_msg_time_update(void *ctx, long long time, int zone_offset){}
static void iap2_msg_gps(void *ctx, unsigned char session, int start){}
static void iap2_msg_gps_gprmc_data_status(void *ctx, int value_a, int value_v, int value_x){}
static void iap2_msg_identify(void *ctx, int type, int ok)
{
	if (ok && !g_link_info->enable_iap_carplay_sess) {
		start_mdnsd_posix();
	}
}
static void iap2_msg_wl_carplay_update(void *ctx, int status)//手机端"Carplay车载"是否开启
{
	struct carplay_ctx* pctx = (struct carplay_ctx*)ctx;
	
	pctx->mPhoneCarplayFlag = status;
	printf("%s:%d status:%d %d\r\n", __func__, __LINE__, status, pctx->mPhoneCarplayFlag);
}
//static void iap2_msg_language_update(void *ctx, const char *lang){}

static void carplay_session_started(void *ctx)
{
	struct carlink_event ev = {0};
	printf("%s is called\r\n", __func__);
	carplay_notify_event(&ev, CARLINK_EVENT_MSG_SESSION_CONNECT, 0);
}

static void carplay_session_Stop(void *ctx)
{
	struct carlink_event ev = {0};
	printf("%s is called\r\n", __func__);
	carplay_notify_event(&ev, CARLINK_EVENT_MSG_SESSION_STOP, 0);
}

static void carplay_session_modes_changed(void *ctx, const CarPlayModeState *	inState){}
static void carplay_session_requestUI(void *ctx)
{
	carplay_send_change_modes(CarplayTransferType_Take, CarplayTransferPriority_UserInitiated, 
		CarplayConstraint_UserInitiated, CarplayConstraint_Anytime,
		0, 0, 0, 0,
		0, 0, 0);
}

static void carplay_session_duck_audio(void *ctx, double inDurationSecs, double inVolume){}
static void carplay_session_unduck_audio(void *ctx, double inDurationSecs){}
//static void carplay_session_notify_device_name(void *ctx, const char *name, int len){}

static void carplay_disable_bt_session(void *ctx)
{
	struct carlink_event ev = {0};

	(void)ctx;

	if (g_link_info->disable_carplay_audio)
		return;
	carplay_notify_event(&ev, CARLINK_EVENT_MSG_DISABLE_BT, 0);
}

static int audio_start_callback_impl(void *ctx, int handle, int type, int rate, int bits, int channels)
{
	int ret = -1;

	ret = carlink_cp_audio_start(handle, type, rate, bits, channels);
	return ret;
}
static void audio_stop_callback_impl(void *ctx, int handle, int type)
{
	carlink_cp_audio_stop(handle, type);
}
//extern char key_value;

static int video_start_callback_impl(void *ctx)
{
#if !DISABLE_CARLINK_H264_FRAME_BUF
	h264_dec_ctx_init();
    set_carlink_display_state(1);
	//key_value = 1;
#else
	struct carplay_ctx* pctx = (struct carplay_ctx*)ctx;

	pctx->mVideoHandle = h264_video_player_init();
#endif
	return 0;
}

static void video_stop_callback_impl(void *ctx)
{
	//key_value = 0;
	printf("%s:%d\r\n", __func__, __LINE__);
#if !DISABLE_CARLINK_H264_FRAME_BUF
	set_carlink_display_state(0);
#else
	struct carplay_ctx* pctx = (struct carplay_ctx*)ctx;

	h264_video_player_uninit(pctx->mVideoHandle);
#endif
}
static int video_proc_data_callback_impl(void *ctx, char *buf, int len)
{
#if !DISABLE_CARLINK_H264_FRAME_BUF
	video_frame_s* frame = NULL;
	//printf("video_proc_data len:%d\r\n", len);

get_retry:
    frame = get_h264_frame_buf();
    if (NULL == frame) {
        printf("h264 frame is empty\r\n");
        vTaskDelay(pdMS_TO_TICKS(10));
        goto get_retry;
        //continue;
    }

    memcpy(frame->cur, buf, len);
    frame->len     = len;

    notify_h264_frame_ready(&frame);
#else
	struct carplay_ctx* pctx = (struct carplay_ctx*)ctx;
	h264_video_player_proc(pctx->mVideoHandle, buf, len);
#endif
	return 0;
}
static void carplay_viewArea_update_notify(void *ctx, int index){}
static void resetDspMode(void *ctx, int mode){}
static void carplay_msg_notify(void*ctx, const char* buf, int len){}

void test_carplay_modules();

static void start_cp(struct carplay_ctx* pctx)
{
	if (!pctx->mInitDone)
		return;
	//if (pctx->mBTConnected)
	//	return;
	if (!pctx->mIapReady)
		return;

	if (pctx->mCarplayConnected)
		return;

	if (g_cp_disable)
		return;

	g_link_info->enable_iap_carplay_sess = 1;
	if (g_link_info->enable_iap_carplay_sess == 0) {
		g_link_info->is_old_carplay_ver = 1;
	} else {
		g_link_info->is_old_carplay_ver = 0;
	}
	g_link_info->wifi_passwd = (char *)carlink_get_wifi_passwd();
	g_link_info->wifi_ssid   = (char *)carlink_get_wifi_ssid();
	g_link_info->wifi_channel = 36;
	
	{
		char ip[4] = {0};
		carlink_get_ap_ip_addr(ip);
		g_link_info->ip_v4_addr[0] = ip[0];
		g_link_info->ip_v4_addr[1] = ip[1];
		g_link_info->ip_v4_addr[2] = ip[2];
		g_link_info->ip_v4_addr[3] = ip[3];
	}
	
	carplay_start();
}

static void carlink_cp_input_event_proc(const struct carlink_event *ev)
{
	uint8_t key = ev->u.para[0];
	bool    pressed = (bool)ev->u.para[1];
	//printf("key %d %d\n", key, (int)pressed);

	if  (0) {
	} else if (key == 19) {
		/*if (!pressed)
			sendKnobInfo(0, 0, 0, 0, 0, 0);//up在主界面向上移动光标
		else
			sendKnobInfo(0, 0, 0, 0, 1, 0);*/
		if (pressed) {//退后台，手机停止发送carplay视频流
			carplay_send_change_modes(CarplayTransferType_Take, CarplayTransferPriority_UserInitiated, 
				CarplayConstraint_Never, CarplayConstraint_Never,
				0, 0, 0, 0, 0, 0, 0);
		}
	} else if (key == 20) {
		/*if (!pressed)
			sendKnobInfo(0, 0, 0, 0, 0, 0); //down在主界面向下移动光标
		else
			sendKnobInfo(0, 0, 0, 0, -1, 0);*/
		if (pressed) {//回前台，手机重新发送carplay视频流
			carplay_send_change_modes(CarplayTransferType_Untake,
				0, 0, 0,
				0, 0, 0, 0, 0, 0, 0);
			request_UI(NULL);
		}
	} else if (key == 18) {//previos
		if (pressed)
			sendKnobInfo(0, 0, 0, 0, 0, 1);
		//else
		//	sendKnobInfo(0, 0, 0, 0, 0, 0);

	} else if (key == 17) {//next
		if (pressed)
			sendKnobInfo(0, 0, 0, 0, 0, -1);
		//else
		//	sendKnobInfo(0, 0, 0, 0, 0, 0);
	} else if (key == 27) {// enter
		if (pressed)
			sendKnobInfo(1, 0, 0, 0, 0, 0);
		else
			sendKnobInfo(0, 0, 0, 0, 0, 0);
	}
}

static void onEventCarplay(void* ctx, const struct carlink_event *ev)
{
	enum CARLINK_EVENT_TYPE type;
	struct carplay_ctx* pctx = (struct carplay_ctx*)ctx;
	
	if (NULL == ev)
		return;
	if (ev->link_type != CARLINK_CARPLAY_WIRELESS && !ev->disable_filter)// skip not aa event
		return;

	type = ev->type;


    switch (type) {
		case CARLINK_EVENT_KEY_EVENT:
			if (!pctx->mCarplayConnected)
				break;
			carlink_cp_input_event_proc(ev);
			break;
        case -1: {
            break;
        }
		case CARLINK_EVENT_INIT_DONE:
			pctx->mInitDone = true;printf("%s:%d\r\n", __func__, __LINE__);
			start_cp(pctx);
			break;
        case CARLINK_EVENT_BT_CONNECT: {
            break;
        }
		case CARLINK_EVENT_BT_IAP_READY: {
			pctx->mIapReady = true;
			printf("%s:%d\r\n", __func__, __LINE__);
			start_cp(pctx);
            break;
        }
        case CARLINK_EVENT_BT_DISCONNECT: {
			printf("bt disconnect, iap disconnect %d %d\r\n", pctx->mIapReady, pctx->mPhoneCarplayFlag);
			if (pctx->mIapReady && !pctx->mPhoneCarplayFlag) {
				//手机端"Carplay车载"关闭，同时苹果手机蓝牙也关闭了，执行一下carplay stop
				carplay_stop();
			}
			pctx->mIapReady = false;
            break;
        }
        case CARLINK_EVENT_WIFI_CONNECT: {
            break;
        }
		case CARLINK_EVENT_MSG_SESSION_CONNECT: {
			printf("CARLINK_EVENT_MSG_SESSION_CONNECT\r\n");
			pctx->mCarplayConnected = 1;
            break;
        }
		case CARLINK_EVENT_MSG_SESSION_STOP: {
			if (0 == pctx->mCarplayConnected) {
				printf("carplay connect timeout!\n");
				if (pctx->mPhoneCarplayFlag) {
					carlink_restart_bt_wifi();
				}
			} else {
				pctx->mCarplayConnected = 0;
				carlink_bt_open();
				printf("%s:%d\r\n", __func__, __LINE__);
				start_cp(pctx);
			}
            break;
        }
		case CARLINK_EVENT_MSG_DISABLE_BT: {
			printf("%s:%d status:%d\r\n", __func__, __LINE__, pctx->mPhoneCarplayFlag);
			carlink_bt_close();
            break;
        }
		
		default:
		break;
    }
}

static void carplay_iap_data_read(void* ctx, const void* buf, int len)
{
	struct carplay_ctx* pctx = (struct carplay_ctx*)ctx;

	if (!pctx->mIapReady)
		return;
	
	iap2_read_data_proc((char*)buf, len);
}


static void  taskInitCarlinkCpProc(void* param)
{
	struct carplay_ctx* pctx = &g_cp_handle;
	struct carlink_event ev = {0};
	int ret  = -1;
#if 0
	carplay_init_parameter();
	carlink_bt_wifi_init();
	//start_mdnsd();
	vTaskDelay(pdMS_TO_TICKS(2000));
	carplay_modules_test();
	vTaskDelete(NULL);
	return;
#endif
	
	(void)param;

	iap2_callbacks iap2_cbs;
	memset((void *)&iap2_cbs, 0, sizeof(iap2_cbs));
	iap2_cbs.iap2_link_status_cb 				= iap2_link_status;
	iap2_cbs.iap2_write_data_cb                 = iap2_write_data;
	iap2_cbs.iap2_msg_time_update_cb 			= iap2_msg_time_update;
	iap2_cbs.iap2_msg_gps_cb 					= iap2_msg_gps;
    iap2_cbs.iap2_msg_gps_gprmc_data_status_cb	= iap2_msg_gps_gprmc_data_status;
	iap2_cbs.iap2_msg_identify_cb 				= iap2_msg_identify;
	iap2_cbs.iap2_msg_wl_carplay_update_cb 		= iap2_msg_wl_carplay_update;
    iap2_cbs.iap2_msg_msg_json_cb           	= carplay_msg_notify;
	iap2_cbs.ctx = (void *)(pctx);
	iap2_register_callbacks(&iap2_cbs);
	
	carplaySessionCbs carplay_cbs;
	memset((void *)&carplay_cbs, 0, sizeof(carplay_cbs));
	carplay_cbs.SessionStarted 				= carplay_session_started;
	carplay_cbs.SessionStop 				= carplay_session_Stop;
	carplay_cbs.SessionModesChanged			= carplay_session_modes_changed;
	carplay_cbs.SessionRequestUI 			= carplay_session_requestUI;
	carplay_cbs.SessionDuckAudio 			= carplay_session_duck_audio;
	carplay_cbs.SessionUnduckAudio 			= carplay_session_unduck_audio;
	carplay_cbs.DisableBtSession 			= carplay_disable_bt_session;
    carplay_cbs.ViewAreaUpdateNotify        = carplay_viewArea_update_notify;
    carplay_cbs.MsgNotify                   = carplay_msg_notify;
	carplay_cbs.ctx 						= (void *)(pctx);
	carplay_register_callbacks((void *)(&carplay_cbs));
	
	video_callbacks_t video_cbs;
	memset((void *)&video_cbs, 0, sizeof(video_cbs));
	video_cbs.video_start_callback 			= video_start_callback_impl;
	video_cbs.video_stop_callback			= video_stop_callback_impl;
	video_cbs.video_proc_data_callback		= video_proc_data_callback_impl;
	video_cbs.ctx							= (void*)pctx;
	video_register_callbacks((void *)(&video_cbs));

	audio_callbacks_t audio_cbs;
	memset((void *)&audio_cbs, 0, sizeof(audio_cbs));
	audio_cbs.audio_start_callback 			= audio_start_callback_impl;
	audio_cbs.audio_stop_callback			= audio_stop_callback_impl;
    audio_cbs.audio_reset_DSP_mode_callback = resetDspMode;
	audio_cbs.ctx							= NULL;
	audio_register_callbacks((void *)(&audio_cbs));

	set_carlink_display_info(0, 0, CARLINK_VIDEO_WIDTH, CARLINK_VIDEO_HEIGHT);
	set_carlink_video_info(CARLINK_VIDEO_WIDTH, CARLINK_VIDEO_HEIGHT, 30);
	carplay_init_parameter();
	
	ret = carlink_common_init();
	if (0 != ret) {
		printf("%s:%d failed\n", __func__, __LINE__);
	}

	pctx->carlinkEventCB.onEvent			= onEventCarplay;
	pctx->carlinkEventCB.rfcomm_data_read	= carplay_iap_data_read;
	pctx->carlinkEventCB.cb_ctx 			= (void*)pctx;

	carlink_register_event_callbacks(&pctx->carlinkEventCB);

#if 1
	carlink_bt_wifi_init();
	carplay_init();

	carplay_notify_event(&ev, CARLINK_EVENT_INIT_DONE, 0);
#else
	carplay_modules_test();
#endif

	vTaskDelete(NULL);
}

static void carplay_init_parameter()
{
	g_link_info->link_type 							= CARPLAY_WIRELESS;

	g_link_info->iap2_name = IAP2NAME;
	g_link_info->iap2_modelIdentifier = IAP2MODEID;
	g_link_info->iap2_manfacturer =MANFACTURER;
	g_link_info->iap2_serialnumber = SERIALNUMBER;
	g_link_info->iap2_sw_ver = SWVER;
	g_link_info->iap2_hw_ver = HWVER;
	g_link_info->iap2_vehicleInfo_name = VEHICLENAME;
	g_link_info->iap2_product_uuid = "101375-0068";

	g_link_info->manfacturer = MANFACTURER;
	g_link_info->oem_icon_label = OEMICONLABEL;
	g_link_info->oem_icon_path = OEMICONPATH;
	g_link_info->os_info = OSINFO;
	g_link_info->iOSVersionMin = iOS_VER_MIN;
	g_link_info->limited_ui_elements = LIMITEDUIELEMENTS;
	g_link_info->guuid = DEFAULTUUID;
	g_link_info->devid = DEFAULTDEVID;

	g_link_info->oem_icon_visible = 0;//ISOEMICONVISIBLE;
	g_link_info->limited_ui = ISLIMITEDUI;
	g_link_info->right_hand_driver = ISRIGHTHANDDRIVER;
	g_link_info->night_mode = 0;
	g_link_info->has_knob = HAS_KNOB;
	g_link_info->has_telbutton = 1;
	g_link_info->has_mediabutton = 1;
	g_link_info->has_proxsensor = HAS_PROXSENSOR;
	g_link_info->has_EnhancedReqCarUI = HAS_EnhancedRequestCarUI;
	g_link_info->has_ETCSupported = HAS_ETC;
	g_link_info->HiFiTouch = 1;
	g_link_info->LoFiTouch = 0;

	g_link_info->usb_country_code = kUSBCountryCodeUS;
	g_link_info->tp_verndor_code = kUSBVendorTouchScreen;
	g_link_info->tp_product_code = kUSBProductTouchScreen;
	g_link_info->tel_verndor_code = kUSBVendorTeleButtons;
	g_link_info->tel_product_code = kUSBProductTeleButtons;
	g_link_info->knob_verndor_code = kUSBVendorKnobButtons;
	g_link_info->knob_product_code = kUSBProductKnobButtons;
	g_link_info->proxsensor_verndor_code = kUSBVendorProxSensor;
	g_link_info->proxsensor_product_code = kUSBProductProxSensor;

	g_link_info->width = CARLINK_VIDEO_WIDTH;//pixel
	g_link_info->height = CARLINK_VIDEO_HEIGHT;
	g_link_info->fps = 30;
	g_link_info->screen_width_phy = CARLINK_VIDEO_WIDTH;
	g_link_info->screen_height_phy = CARLINK_VIDEO_HEIGHT;

	g_link_info->icurrent = 1000;
	g_link_info->enable_iap_carplay_sess = 1;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	g_link_info->keychain_path_dir = "/sf";
	{
		static carlink_flash_io io = {0};
		io.op_flash = cp_sf_op_flash;
		io.read_data = cp_sf_read_data;
		io.write_data = cp_sf_write_data;
    	//sfud_erase(sfud_get_device(0), CARLINK_CP_OFFSET, CARLINK_CP_SIZE);
		register_carlink_flash_io_interface(&io);
	}
#else
	g_link_info->keychain_path_dir = DEVICE_PARTITION_NAME;
#endif
	g_link_info->is_old_carplay_ver = 0;
	g_link_info->enable_single_ui = 1;
	
	g_link_info->disable_carplay_audio = 1;//1. audio is stream to bt;

	//g_link_info->mfi_ic_addr = 0x22; //cp2.0的地址由reset脚决定
	g_link_info->mfi_ic_addr = 0x20; //cp3.0
	g_link_info->mfi_ic_i2c_bus_num = 0;

	g_link_info->area[0].w = CARLINK_VIDEO_WIDTH;
    g_link_info->area[0].h = CARLINK_VIDEO_HEIGHT;
    g_link_info->area[0].x = 0;
    g_link_info->area[0].y = 0;

	g_link_info->area[1].w = 1024;
    g_link_info->area[1].h = 600;
    g_link_info->area[1].x = 0;
    g_link_info->area[1].y = 0;

	g_link_info->view_area_index = 0;
	
}

void carplay_modules_test();
int carlink_cp_init()
{
	xTaskCreate(taskInitCarlinkCpProc, "CpinitThread", 1024 * 32, NULL, configMAX_PRIORITIES / 4, NULL);
	return 0;
}

void carlink_cp_enable(int enable)
{
	if (enable) {
		g_cp_disable = false;
	} else {
		g_cp_disable = true;
	}
}


//int mdnsd_task();
static void  taskMdnsdProc(void* param)
{
	//mdnsd_task();
	vTaskDelete(NULL);
}

void start_mdnsd()
{

	xTaskCreate(taskMdnsdProc, "CpinitThread", 2048 * 4, NULL, 2, NULL);
}

#else
int carlink_cp_init()
{
	return 0;
}

void carlink_cp_enable(int enable)
{
	(void)enable;
}


#endif

