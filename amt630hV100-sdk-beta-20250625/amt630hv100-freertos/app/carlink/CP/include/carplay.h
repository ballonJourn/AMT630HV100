#ifndef __CARPLAY_IF_H_H
#define __CARPLAY_IF_H_H

#ifdef __cplusplus
extern "C" {
#endif

/*#ifndef CARLINK_LINK_TYPE
#define CARLINK_LINK_TYPE
typedef enum
{
	CARPLAY = 0x00,
	CARLIFE,
	ANDROID_CARLIFE,
	ANDROID_MIRROR = 3,
	IOS_CARLIFE,
	ANDROID_AUTO = 5,
	ECLINK = 0x06,
	CARPLAY_WIRELESS
}Link_TYPE;
#endif*/

typedef enum
{
	CALL_ACCEPT = 2,
	CALL_DROP = 3,
}CALL;

typedef enum
{
	MEDIA_NONE = 0,
	MEDIA_PLAY,
	MEDIA_PAUSE,
	MEDIA_PLAY_PAUSE,
	MEDIA_NEXT,
	MEDIA_PREVIOUS
}MEDIA;

typedef enum
{
	AUDIO_MEDIA = 0,
	AUDIO_TELEPHONE,
	AUDIO_RECOGNITION,
	AUDIO_ALERT,
    AUDIO_REC,
    AUDIO_ALT,
    AUDIO_AUX_IN,
    AUDIO_AUX_OUT
} AUDIO_TYPE;

/*

typedef enum _IAP2_LINK_STATUS
{
	IAP2_INIT = -1,
	IAP2_USB_HOST_INSERTED,
	IAP2_USB_HOST_REMOVE,
	IAP2_USB_SWITCH_FAIL,
	IAP2_CONNECT,
	IAP2_DISCONNECT,
}IAP2_LINK_STATUS;*/

#ifndef AirplayModeStateAlias
#define AirplayModeStateAlias

typedef int		CarplayEntity;
#define CarplayEntity_NotApplicable			0
#define CarplayEntity_Controller			1
#define CarplayEntity_Accessory				2

typedef int		CarplayTransferType;
#define CarplayTransferType_NotApplicable				0
#define CarplayTransferType_Take						1 // Transfer ownership permanently.
#define CarplayTransferType_Untake						2 // Release permanent ownership.
#define CarplayTransferType_Borrow						3 // Transfer ownership temporarily.
#define CarplayTransferType_Unborrow					4 // Release temporary ownership.


typedef int		CarplayTransferPriority;
#define CarplayTransferPriority_NotApplicable				0
#define CarplayTransferPriority_NiceToHave					100 // Transfer succeeds only if constraint is <= Anytime.
#define CarplayTransferPriority_UserInitiated				500 // Transfer succeeds only if constraint is <= UserInitiated.


typedef int		CarplayConstraint;
#define CarplayConstraint_NotApplicable			0
#define CarplayConstraint_Anytime					100  // Resource may be taken/borrowed at any time.
#define CarplayConstraint_UserInitiated			500  // Resource may be taken/borrowed if user initiated.
#define CarplayConstraint_Never					1000 // Resource may never be taken/borrowed.


typedef int		CarplayTriState;
#define CarplayTriState_NotApplicable		0
#define CarplayTriState_False				-1
#define CarplayTriState_True				1


typedef int		CarplaySpeechMode;
#define CarplaySpeechMode_NotApplicable			0
#define CarplaySpeechMode_None						-1 // No speech-related states are active.
#define CarplaySpeechMode_Speaking					1  // Device is speaking to the user.
#define CarplaySpeechMode_Recognizing				2  // Device is recording audio to recognize speech from the user.

typedef struct
{
    CarplayEntity			entity;
    CarplaySpeechMode		mode;

}CarPlaySpeechState;

typedef struct
{
    CarplayEntity			screen;		// Owner of the screen.
    CarplayEntity			permScreen;		// Permanent owner of screen.
    CarplayEntity			mainAudio;	// Owner of main audio.
    CarplayEntity			permMainAudio;	// Permanent owner of main audio.
    CarplayEntity			phoneCall;	// Owner of phone call.
    CarPlaySpeechState		speech;		// Owner of speech and its mode.
    CarplayEntity			turnByTurn;	// Owner of navigation.

}	CarPlayModeState;
#endif



typedef void ( *CarplaySessionStarted_f )(void *ctx);
typedef void ( *CarplaySessionStop_f )(void *ctx);
typedef void ( *CarplaySessionModesChanged_f )(void *ctx, const CarPlayModeState *	inState);
typedef void ( *CarplaySessionRequestUI_f )(void *ctx);
typedef void ( *CarplaySessionDuckAudio_f)(void *ctx, double inDurationSecs, double inVolume);
typedef void ( *CarplaySessionUnduckAudio_f)(void *ctx, double inDurationSecs);
typedef void ( *CarplayDisableBtSession_f)(void *ctx);
typedef void ( *CarplayNotifyDeviceName_f)(void *ctx, const char *name, int len);
typedef void ( *CarplayBonjourServiceFound_f)(void *ctx, char phone_bt_mac[6]);
typedef void ( *CarplayViewAreaUpdateNotify_f)(void *ctx, int index);
//typedef void ( *CarplayMsgNotify_f)(void *ctx, int id, int para1, int para2, const char* msg, int msgLen);
typedef void ( *CarplayMsgNotify_f)(void *ctx, const char* msg, int msgLen);

typedef struct carplay_session_callbacks
{
    CarplaySessionStarted_f SessionStarted;
    CarplaySessionStop_f SessionStop;
    CarplaySessionModesChanged_f SessionModesChanged;
    CarplaySessionRequestUI_f SessionRequestUI;
    CarplaySessionDuckAudio_f	SessionDuckAudio;
    CarplaySessionUnduckAudio_f SessionUnduckAudio;
    CarplayDisableBtSession_f	DisableBtSession;
    CarplayNotifyDeviceName_f	NotifyDeviceName;
    CarplayBonjourServiceFound_f BonjourServiceFound;
    CarplayViewAreaUpdateNotify_f ViewAreaUpdateNotify;

    CarplayMsgNotify_f           MsgNotify;
    void *ctx;
}carplay_session_callbacks_t;

typedef carplay_session_callbacks_t carplaySessionCbs;

/*
* @brief 初始化carplay的基本信息,整个程序的生命周期里面调用一次;
*/
int carplay_init();
void carplay_uninit();
/*
* @brief 注册carplay plugin的回调函数;
* @param  cbs指向carplay_session_callbacks_t类型的变量;
*/
void carplay_register_callbacks(void *cbs);
/*
* @brief 启动carplay;
*/
int carplay_start();

void start_carplay_client_connect();
/*
* @brief 停止carplay;
*/
void carplay_stop();
/*
* @brief 设置要连接手机的蓝牙mac地址,无线连接的时候使用;
* @param  bt_addr 蓝牙mac地址;
*/
void carplay_wl_set_iphone_mac_addr(char bt_addr[6]);

/*
* @brief 发送电话按键给苹果;
* @param  button 类型见enum CALL;
*/
void Telephone_button_Update(unsigned button);
/*
* @brief 发送多媒体按键给苹果;
* @param  button 类型见enum MEDIA;
*/
void Media_button_Update(unsigned button);
/*
* @brief 发送siri按键给苹果;
* @param  button 1表示按下启动siri,0表示释放来结束siri;
*/
void send_siri_cmd(int button);
void send_single_touchscreen_x_y_2_carplay(unsigned short x, unsigned short y, unsigned char pressed);
void send_mul_touchscreen_x_y_2_carplay(
                                        unsigned char finger_idx1, unsigned char pressed1,
                                        unsigned short x1, unsigned short y1,
                                        unsigned char finger_idx2, unsigned char pressed2,
                                        unsigned short x2, unsigned short y2
                                        );
/*
* @brief 发送旋钮信息给苹果手机,不建议用这个;
*/
void KnobUpdate(char 	gSelectButtonPressed,
				char 	gHomeButtonPressed,
				char 	gBackButtonPressed,
				double	gXPosition,
				double	gYPosition,
				char	gWheelPositionRelative
				);
/*
* @brief 发送旋钮信息给苹果手机;
*/
void sendKnobInfo( char	gSelectButtonPressed,
						char	gHomeButtonPressed,
						char	gBackButtonPressed,
						char	gXPosition,
						char	gYPosition,
						char	gWheelPositionRelative);

/*
* @brief 请求iphone启动一个应用;
* @param  bundleId app的包名;
* @param  alert app启动的时候时候弹出警告对话框;
*/
int apple_app_launcher(char * bundleId, char alert);

/*
* @brief 要求iphone发送关键帧;
*/
void force_key_frame();
/*
* @brief 设置夜间模式;
* @param  inNightMode 1表示进入夜间模式,0表示退出夜间模式
*/
void set_night_mode(char inNightMode);

/*
* @brief 请求苹果手机输出视频数据;
*/
void request_UI(char *url);

/*
*
*/
void borrow_screen(int priority, int unborrow_constraint);
void unborrow_screen(void);
void take_screen(int priority, int take_constraint, int borrow_constraint);
void untake_screen(void);
void borrow_audio(int priority, int unborrow_constraint);
void unborrow_audio(void);
void take_audio(int priority, int take_constraint, int borrow_constraint);
void untake_audio(void);
void carplay_send_change_modes(CarplayTransferType 		inScreenType,
								CarplayTransferPriority		inScreenPriority,
								CarplayConstraint			inScreenTake,
								CarplayConstraint			inScreenBorrow,
								CarplayTransferType 		inAudioType,
								CarplayTransferPriority		inAudioPriority,
								CarplayConstraint			inAudioTake,
								CarplayConstraint			inAudioBorrow,
								CarplayTriState				inPhone,
								CarplaySpeechMode			inSpeech,
								CarplayTriState				inTurnByTurn);

int carplay_ipc_start();
void carplay_ipc_stop();
void process_play_stream(int handle, void *buffer, int len, int frames, unsigned long long timestamp);
void process_record_stream(int handle, void *buffer, int len, int frames, unsigned long long timestamp);

int carplay_get_iphone_ip_addr(char *ipaddr);
void carplay_wl_set_iphone_mac_addr(char bt_addr[6]);
int carplay_get_connected_iphone_wifi_mac(char mac[18]);

#ifdef __cplusplus
}
#endif
#endif
