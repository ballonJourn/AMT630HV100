#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "carlink_video.h"
#include "carlink_common.h"
#include "AndroidAuto.h"
#include "board.h"
#include "task.h"
#include "vehicle_param/vehicle_param.h"
#if CARLINK_AA

struct AAHandle
{
	struct ICalinkEventCallbacks carlinkEventCB;
	struct IAACallbacks          carlinkAACB;
	
	bool   mInitDone;
	bool   mBTConnected;
	bool   mRfcommReady;
	char   mLocalBTMac[6];
	char   mRemoteBTMac[6];
	char   mIp[32];
	bool   mAAConnected;
	void*  mVideoHandle;
};
//extern int debug_buf_ref;

struct AAHandle gAACtx;
static bool  g_aa_disable = false;


static void android_auto_notify_event(struct carlink_event *ev, enum CARLINK_EVENT_TYPE type, bool disable_filter)
{
	ev->link_type = CARLINK_AUTO_WIRELESS;
	ev->disable_filter = disable_filter;
	ev->type = type;
	carlink_notify_event(ev);
}

static void start_aa(struct AAHandle* pctx)
{
	if (!pctx->mInitDone)
		return;
	//if (pctx->mBTConnected)
	//	return;
	if (!pctx->mRfcommReady)
		return;
	if (g_aa_disable)
		return;
	printf("%s:%d\r\n", __func__, __LINE__);

	g_auto_link_info->wifi_ssid 		= (char *)carlink_get_wifi_ssid();
	g_auto_link_info->wifi_passwd 		= (char *)carlink_get_wifi_passwd;
	android_auto_set_rfcomm_info((const char*)carlink_get_wifi_ssid(), 
		(const char*)carlink_get_wifi_passwd(), (const char*)carlink_get_wifi_mac(), 5, (const char*)pctx->mIp, 0);
	android_auto_start();
}

static void carlink_aa_input_event_proc(const struct carlink_event *ev)
{
	uint8_t key = ev->u.para[0];
	bool    pressed = (bool)ev->u.para[1];

	if (key == 19) {//right
		if (pressed)
		android_auto_send_key_event(21, 1);
		else
		android_auto_send_key_event(21, 0);	
	} else if (key == 20) {
		if (pressed)
		android_auto_send_key_event(22, 1);
		else
		android_auto_send_key_event(22, 0);	
	} else if (key == 18) {//previos
		if (pressed)
		android_auto_send_knob_event(65536, 1);
	} else if (key == 17) {//next
		if (pressed)
		android_auto_send_knob_event(65536, -1);
	} else if (key == 27) {// enter
		if (pressed)
		android_auto_send_key_event(23, 1);
		else
		android_auto_send_key_event(23, 0);
	} else {
		if (pressed)
			android_auto_send_key_event(3, 1);
		else
			android_auto_send_key_event(3, 0);
	}
}


static void onEventAA(void* ctx, const struct carlink_event *ev)
{
	enum CARLINK_EVENT_TYPE type;
	struct AAHandle* pctx = (struct AAHandle*)ctx;
	if (NULL == ev)
		return;
	if (ev->link_type != CARLINK_AUTO_WIRELESS && !ev->disable_filter)// skip not aa event
		return;

	type = ev->type;

	switch (type) {
	case CARLINK_EVENT_KEY_EVENT:
		if (!pctx->mAAConnected)
			break;
		carlink_aa_input_event_proc(ev);
		break;
	case -1: {
        break;
    }
	case CARLINK_EVENT_INIT_DONE:
		pctx->mInitDone = true;
		start_aa(pctx);
		break;
    case CARLINK_EVENT_BT_CONNECT: {
		pctx->mBTConnected = true;
		printf("\r\n[ouchunhua]aa connect\r\n");
		start_aa(pctx);
        break;
    }
	case CARLINK_EVENT_BT_AA_RFCOMM_READY: {
		pctx->mRfcommReady = true;
		start_aa(pctx);
        break;
    }
    case CARLINK_EVENT_BT_DISCONNECT: {
		printf("\r\n[ouchunhua]aa disconnect\r\n");
		vehicle_set_data(VEH_CARLINK_AA_STATUS, 0);
		pctx->mBTConnected = false;
		pctx->mRfcommReady = false;
        break;
    }
    case CARLINK_EVENT_WIFI_CONNECT: {
        break;
    }
	case CARLINK_EVENT_MSG_SESSION_CONNECT: {
        break;
    }
	case CARLINK_EVENT_MSG_SESSION_STOP: {
		printf("%s:%d\r\n", __func__, __LINE__);
		printf("\r\n============session stop============\r\n");
		start_aa(pctx);
        break;
    }
	
	default:
	break;
	}
}

static void aa_rfcomm_data_read(void* ctx, const void* buf, int len)
{
	struct AAHandle* pctx = (struct AAHandle*)ctx;

	if (!pctx->mRfcommReady)
		return;
	
	android_auto_rfcomm_read_data_proc((char*)buf, len);
}

static int rf_write_transfer_cb(void* cb_ctx, char *buf, int len)
{
	int ret = -1;
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;

	if (!pctx->mRfcommReady)
		return -1;
	ret = carlink_auto_rfcomm_data_write((unsigned char *)buf, len);

	return ret;
}

int g_v_count;
static void video_init_impl(void* cb_ctx, int w, int h, int x, int y)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
	printf("x:%d y:%d w:%d h:%d\r\n", x, y, w, h);
	set_carlink_active_video_info(x, y);
#if !DISABLE_CARLINK_H264_FRAME_BUF
	h264_dec_ctx_init();
    set_carlink_display_state(1);
#else
	pctx->mVideoHandle = h264_video_player_init();
#endif
	//carlink_bt_close();
	g_v_count = 0;
	//debug_buf_ref = 1;
}

static void video_uninit_impl(void* cb_ctx)
{
	printf("%s:%d\r\n", __func__, __LINE__);
#if !DISABLE_CARLINK_H264_FRAME_BUF
	set_carlink_display_state(0);
#else
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	h264_video_player_uninit(pctx->mVideoHandle);
#endif
	set_carlink_active_video_info(0, 0);
}

static void video_play_impl(void* cb_ctx, char *buf, int len)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
#if !DISABLE_CARLINK_H264_FRAME_BUF
	video_frame_s* frame = NULL;
	
	(void)pctx;

get_retry:
    frame = get_h264_frame_buf();
    if (NULL == frame) {
        //printf("h264 frame is empty\r\n");
        vTaskDelay(pdMS_TO_TICKS(10));
        goto get_retry;
        //continue;
    }

    memcpy(frame->cur, buf, len);
    frame->len     = len;

    notify_h264_frame_ready(&frame);
	if (g_v_count++ > 50) {//enable_malloc_debug = 1;
		//android_auto_get_video_focus(0);
	}
#else
	vehicle_set_data(VEH_CARLINK_AA_STATUS, 1);
	h264_video_player_proc(pctx->mVideoHandle, buf, len);
#endif
}

static void audio_init_callback_impl(void* cb_ctx, int type, int sample, int ch, int bits)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}
static void audio_uninit_callback_impl(void* cb_ctx, int type)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}
static void audio_play_impl(void* cb_ctx, int type, char *buf, int len)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}

static void mic_init_callback_impl(void* cb_ctx, int sample, int ch, int bits)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}
static void mic_uninit_callback_impl(void* cb_ctx)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}
static void mic_capture_impl(void* cb_ctx, char *buf, int len)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}

static void bt_paring_request_callback_impl(void* cb_ctx, const char *phoneAddr, int pairingMethod)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}

static void status_notify_impl(void* cb_ctx, int status_type)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	struct carlink_event ev = {0};

	printf("\r\n[ouchunhua]status notify:%d\r\n", status_type);
	if (status_type == LINK_REMOVED) {
		printf("%s:%d\r\n", __func__, __LINE__);
		android_auto_notify_event(&ev, CARLINK_EVENT_MSG_SESSION_STOP, 0);
	} else if (status_type == LINK_SUCCESS) {
		pctx->mAAConnected = true;
	}
}

static void json_msg_notify_impl(void* cb_ctx, char *buf, int len)
{
	struct AAHandle* pctx = (struct AAHandle*)cb_ctx;
	
	(void)pctx;
}



int _carlink_aa_init()
{
	int ret  = -1;
	struct AAHandle* pctx = &gAACtx;
	struct IAACallbacks*        carlinkAACBPtr = NULL;

	memset((void*)pctx, 0, sizeof(struct AAHandle));


	g_auto_link_info->width 		= CARLINK_VIDEO_WIDTH;
	g_auto_link_info->height 	= CARLINK_VIDEO_HEIGHT;
	g_auto_link_info->fps 		= 30;
	g_auto_link_info->density 	= 160;
	g_auto_link_info->disable_auto_audio = 1;
	g_auto_link_info->video_auto_start 	 = 1;

	set_carlink_display_info(0, 0, CARLINK_VIDEO_WIDTH, CARLINK_VIDEO_HEIGHT);
	set_carlink_video_info(CARLINK_VIDEO_WIDTH, CARLINK_VIDEO_HEIGHT, 30);

	ret = carlink_common_init();

	pctx->carlinkEventCB.onEvent 			= onEventAA;
	pctx->carlinkEventCB.rfcomm_data_read 	= aa_rfcomm_data_read;
	pctx->carlinkEventCB.cb_ctx 			= (void*)pctx;

	carlink_register_event_callbacks(&pctx->carlinkEventCB);

	ret = carlink_bt_wifi_init();

	carlinkAACBPtr = &pctx->carlinkAACB;
	carlinkAACBPtr->video_init 					= video_init_impl;
	carlinkAACBPtr->video_uninit 				= video_uninit_impl;
	carlinkAACBPtr->video_play 					= video_play_impl;
	carlinkAACBPtr->audio_init_callback 		= audio_init_callback_impl;
	carlinkAACBPtr->audio_uninit_callback 		= audio_uninit_callback_impl;
	carlinkAACBPtr->audio_play           		= audio_play_impl;
	carlinkAACBPtr->mic_init_callback 			= mic_init_callback_impl;
	carlinkAACBPtr->mic_uninit_callback 		= mic_uninit_callback_impl;
	carlinkAACBPtr->mic_capture 				= mic_capture_impl;
	carlinkAACBPtr->bt_paring_request_callback 	= bt_paring_request_callback_impl;
	carlinkAACBPtr->status_notify 				= status_notify_impl;
	carlinkAACBPtr->json_msg_notify 			= json_msg_notify_impl;
	
	carlinkAACBPtr->rf_read_transfer_cb 		= NULL;
	carlinkAACBPtr->rf_write_transfer_cb 		= rf_write_transfer_cb;
	carlinkAACBPtr->cb_ctx 						= (void*)pctx;
	ret = android_auto_init(&pctx->carlinkAACB);
	{
		char ip[4] = {0};
		carlink_get_ap_ip_addr(ip);
		sprintf(pctx->mIp, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	}
	

	{
		struct carlink_event ev = {0};
		android_auto_notify_event(&ev, CARLINK_EVENT_INIT_DONE, 0);
	}
	return ret;
}

static void  taskInitCarlinkAA(void* param)
{
	_carlink_aa_init();
	vTaskDelete(NULL);
}


int carlink_aa_init()
{
	xTaskCreate(taskInitCarlinkAA, "initThread", 2048 * 4, NULL, 1, NULL);

	return 0;
}


void carlink_aa_enable(int enable)
{
	if (enable) {
		g_aa_disable = false;
	} else {
		g_aa_disable = true;
	}
}

#else
int carlink_aa_init()
{
	return 0;
}

void carlink_aa_enable(int enable)
{
	(void)enable;
}

#endif

