#ifndef _ANDROID_AUTO_H_H
#define _ANDROID_AUTO_H_H

typedef enum
{
    LINK_UNSUPPORTED= 0xff,
    LINK_CONNECTED = 1,        
    LINK_DISCONNECTED = 2,      
    LINK_STARTING = 3,          
    LINK_SUCCESS = 4,           
    LINK_FAIL = 5,             
    LINK_EXITING = 6,           
    LINK_EXITED = 7 ,          
    LINK_REMOVED = 8,           
    LINK_INSERTED = 9,          
    LINK_NOT_INSERTED = 10,     
    LINK_NOT_INSTALL = 11,      
    LINK_CALL_PHONE = 12,       
    LINK_CALL_PHONE_EXITED = 13,
    LINK_MUTE =14,              
    LINK_UNMUTE  = 15,          
    LINK_NODATA = 16,          
    LINK_VIDEOREADY = 17,       
    LINK_BT_DISCONNECT = 18,    
    LINK_FAILED_EAP = 19,       
    LINK_FAILED_UNSTART = 20,  
	LINK_AUTO_BT_UNPAIRED = 21,
	LINK_AUTO_BT_PAIRED = 22,
	LINK_AUTO_BT_REQUEST = 23,
	LINK_EXIT_PROCESS = 24,
    LINK_KILL_PROCESS = 25,

    LINK_SOCKET_TRUST = 26,
    LINK_OPEN_CARLIFE = 27,
    LINK_VOLUME_START = 28,
    LINK_VOLUME_STOP = 29,
    LINK_RECONNECT = 30, 
    LINK_ILLLIGHT_ON = 31,
    LINK_ILLLIGHT_OFF = 32,

    LINK_SIRI_START = 33,
    LINK_SIRI_STOP = 34,
    LINK_ASSIST_START = 35,
    LINK_ASSIST_STOP  = 36, 
    LINK_TEL_START = 37,
    LINK_TEL_STOP = 38,
    LINK_MUSIC_START = 39,
    LINK_MUSIC_STOP = 40,
    
    LINK_SCREEN_CONTROLLER = 62,
    LINK_SCREEN_ACCESSORY = 63,
    
    LINK_TAKE_AUDIO = 80,
    LINK_UNTAKE_AUDIO = 81,

    LINK_USE_USB0 = 82,
    LINK_USE_USB1 = 83,
    LINK_NO_ERROR = 0,
    LINK_BTCONNECT_ERROR = -1000, 
    LINK_BTCOMM_ERROR = -1001,
    LINK_BTAUTH_ERROR = -1002, 
    LINK_BTIDRECJECT_ERROR = -1003,
}Link_STATUS;
	
struct IAACallbacks
{
	void (*video_init)(void* cb_ctx, int w, int h, int x, int y);
	void (*video_uninit)(void* cb_ctx);
	void (*video_play)(void* cb_ctx, char *buf, int len);

	void (*audio_init_callback)(void* cb_ctx, int type, int sample, int ch, int bits);
	void (*audio_uninit_callback)(void* cb_ctx, int type);
	void (*audio_play)(void* cb_ctx, int type, char *buf, int len);

	void (*mic_init_callback)(void* cb_ctx, int sample, int ch, int bits);
	void (*mic_uninit_callback)(void* cb_ctx);
	void (*mic_capture)(void* cb_ctx, char *buf, int len);
	
	void (*bt_paring_request_callback)(void* cb_ctx, const char *phoneAddr, int pairingMethod);

	void (*status_notify)(void* cb_ctx, int status_type);

	int (*rf_read_transfer_cb)(void* cb_ctx, char *buf, int len);//@Deprecated
	int (*rf_write_transfer_cb)(void* cb_ctx, char *buf, int len);
	
	
	void (*json_msg_notify)(void* cb_ctx, char *buf, int len);
	
	void* cb_ctx;
};

typedef struct __auto_cfg_info
{
	short		width;//pixel
	short		height;
	short       density;
	short		fps;
	char*   	wifi_ssid;
	char*   	wifi_passwd;
	char   		wifi_channel;
	bool 		disable_auto_audio;
	bool       	video_auto_start;

} auto_cfg_info;

extern auto_cfg_info  *g_auto_link_info;

int android_auto_init(struct IAACallbacks* cbs);
int android_auto_rfcomm_read_data_proc(char *buf, int len);
void android_auto_start();
void android_auto_set_rfcomm_info(const char * ssid, const char * passwd, const char * mac, int securityMode, const char * ip, int port);
void android_auto_set_BT_paring_status(bool paired, const char *pinCode, int status);
void android_auto_send_touch_event(unsigned int x, unsigned int y, int action);
void android_auto_send_mul_touch_event(int pointer_id, unsigned int pointer_x, unsigned int pointer_y, int event_type);
void android_auto_send_key_event(unsigned int keycode, int press);
void android_auto_send_knob_event(unsigned int keycode, int delta);
void android_auto_set_night_mode(bool night);

void android_auto_get_video_focus(int mode);
void android_auto_release_video_focus(int mode);
void android_auto_get_audio_focus(int mode);
void android_auto_release_audio_focus(int mode);

#endif
