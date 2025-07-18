/*****************************************************************
* Project ECSDK.
* (c) copyright 2017-2020.
* Company Carbit.
* All rights reserved. Copy without permission
****************************************************************/
///\file ECTypes.h
#ifndef CARBIT_EC_SDK_TYPES_H
#define CARBIT_EC_SDK_TYPES_H

#if  defined(PLATFORM_H1B)
#include "api/libfs2/types.h"
#else
#include <stdint.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/**
* @brief return code of ECSDK APIs.
*/
#define EC_OK                     0              ///< success
#define EC_ERR_INVAL_OP          -1              ///< invalid operation
#define EC_ERR_INVAL_PARAM       -2              ///< invalid parameter(s)
#define EC_ERR_OP_FAIL           -3              ///< operation failed
#define EC_ERR_LICENSE_AUTH_FAIL -4              ///< license authorize failed
#define EC_ERR_APP_NOT_STARTED   -5              ///< phone app not started
#define EC_ERR_APP_AUTH_FAIL     -6              ///< phone app authorization failed.
#define EC_ERR_APP_AUTH_PENDING  -7              ///< phone app authorization pending.
#define EC_ERR_PERMISSION_DENIED -8              ///< without permission.


enum ECConnectedStatus
{
    EC_CONNECT_STATUS_DEVICE_ATTACHED = 1,
    EC_CONNECT_STATUS_DEVICE_DEATTACHED = 2,
    EC_CONNECT_STATUS_CONNECTING = 3,
    EC_CONNECT_STATUS_CONNECT_FAILED = 4,
    EC_CONNECT_STATUS_CONNECT_SUCCEED = 5,
    EC_CONNECT_STATUS_DISCONNECTED = 6,
    EC_CONNECT_STATUS_APP_EXIT = 7,
    EC_CONNECT_STATUS_INTERRUPTED_BY_APP = 8,
};
typedef enum ECConnectedStatus ECConnectedStatus;

enum ECConnectedType
{
    EC_CONNECT_TYPE_ANDROID_USB= 0,                 ///< android usb
    EC_CONNECT_TYPE_ANDROID_WIFI = 1,                   ///< android wifi
    EC_CONNECT_TYPE_IOS_USB_EAP = 2,                    ///< iphone usb eap, app screen
    EC_CONNECT_TYPE_IOS_USB_MUX = 3,                    ///< iphone usb mux, app screen
    EC_CONNECT_TYPE_IOS_USB_AIRPLAY = 4,                ///< iphone usb airplay, system screen
    EC_CONNECT_TYPE_IOS_WIFI_APP = 5,                   ///< iphone wifi app, app screen
    EC_CONNECT_TYPE_IOS_WIFI_AIRPLAY = 6,               ///< iphone wifi airplay, system screen
    EC_CONNECT_TYPE_IOS_USB_LIGHTNING = 7,              ///< iphone usb lightning connect(闪连)
    EC_CONNECT_TYPE_MAX,                            ///< reserve
};
typedef enum ECConnectedType ECConnectedType;

enum ECMirrorStatus
{
    EC_MIRROR_STATUS_MIRROR_AUTH_PENDING = 0x1000,
    EC_MIRROR_STATUS_MIRROR_STARTED = 0x1001,
    EC_MIRROR_STATUS_MIRROR_FAILED = 0x1002,
	EC_MIRROR_STATUS_MIRROR_STOPPED = 0x1003,
};
typedef enum ECMirrorStatus ECMirrorStatus;

/**
* @enum ECStatusMessage
*
* @brief status message code
*
* @see IECCallback::onECStatusMessage
*/
enum ECStatusMessage
{
    EC_STATUS_MESSAGE_USER_NOT_AUTH_DEBUG = 0,       ///< user did not authorize car to use phone via usb debug mode.
    EC_STATUS_MESSAGE_APP_NOT_RUNNING = 1,           ///< phone app is not running
    EC_STATUS_MESSAGE_APP_FOREGROUND = 2,            ///< phone app runs in foreground,deprecated ,instead of EC_STATUS_MESSAGE_APP_FOREGROUND_SAME or EC_STATUS_MESSAGE_APP_FOREGROUND_SPLIT
	EC_STATUS_MESSAGE_APP_FOREGROUND_SAME = 3,       ///< phone app runs in foreground with same screen mode
	EC_STATUS_MESSAGE_APP_FOREGROUND_SPLIT = 4,      ///< phone app runs in foreground with split screen mode
    EC_STATUS_MESSAGE_APP_BACKGROUND_SAME = 5,       ///< phone app runs in background with same screen mode
	EC_STATUS_MESSAGE_APP_BACKGROUND_SPLIT = 6,      ///< phone app runs in background with split screen mode
    EC_STATUS_MESSAGE_APP_SCREENLOCKED = 7,          ///< screen locked
    EC_STATUS_MESSAGE_APP_UNSCREENLOCKED = 8,        ///< screen unlocked
    EC_STATUS_MESSAGE_APP_NAVI_STARTED = 9,          ///< phone app started to navigate
    EC_STATUS_MESSAGE_APP_NAVI_STOPPED = 10,           ///< phone app stopped to navigate
    EC_STATUS_MESSAGE_SWITCH_AOA_FAIL = 11,           ///< failed to switch AOA mode for current android phone
    EC_STATUS_MESSAGE_3RD_APP_HORIZONTAL_NOT_SUPPORT = 12,    ///< 3rd app does not support horizontal screen
    EC_STATUS_MESSAGE_3RD_APP_HORIZONTAL_SUPPORT = 13,        ///< 3rd app supports horizontal screen
    EC_STATUS_MESSAGE_APP_VR_STARTED = 14,            ///< vr of phone app started.
    EC_STATUS_MESSAGE_APP_VR_STOPPED = 15,            ///< vr of phone app stopped.
	EC_STATUS_MESSAGE_ACQUIRE_BLUETOOTH_A2DP = 16,    ///< HU needed to acquire bluetooth for music play and send play event to phone via bluetooth.
	EC_STATUS_MESSAGE_ACQUIRE_BLUETOOTH_A2DP_WITHOUT_SEND_PLAY = 17,    ///< HU needed to acquire bluetooth for music play but not send play event to phone.
	EC_STATUS_MESSAGE_SWITCH_TO_SYSTEM_MAIN_PAGE = 18,///< HU switch to system main page.
	EC_STATUS_MESSAGE_LAUNCH_PHONE_APP = 19,          ///< phone request HU to launch app.
	EC_STATUS_MESSAGE_APP_TALKIE_STARTED = 20,        ///< talkie of phone app started.
	EC_STATUS_MESSAGE_APP_TALKIE_STOPPED = 21,        ///< talkie of phone app stopped.
	EC_STATUS_MESSAGE_APP_MUSIC_PENDING = 22,         ///< music of phone app is pending, it will be deprecated.
	EC_STATUS_MESSAGE_APP_MUSIC_PLAYING = 23,         ///< music of phone app is playing, it will be deprecated.
	EC_STATUS_MESSAGE_APP_MUSIC_PAUSED = 24,          ///< music of phone app paused, it will be deprecated.
	EC_STATUS_MESSAGE_APP_MUSIC_STOPPED = 25,	     ///< music of phone app stopped, it will be deprecated.
	EC_STATUS_MESSAGE_SWITCH_TO_FRONT = 26,           ///< make easyconn switch to front.
	EC_STATUS_MESSAGE_OPEN_BLUETOOTH_PHONE = 27,      ///< make HU open bluetooth phone.
	EC_STATUS_MESSAGE_OPEN_BLUETOOTH_SETTING = 28,    ///< make HU open bluetooth setting.
    EC_STATUS_MESSAGE_TALKIE_MUTE = 29,               ///< talkie of phone app is mute.
    EC_STATUS_MESSAGE_TALKIE_NON_MUTE = 30,           ///< talkie of phone app is not mute.
	EC_STATUS_MESSAGE_TALKIE_NON_LOGIN = 31,          ///< user of phone app is not logged in.
    EC_STATUS_MESSAGE_SWITCH_EASYCONN_TO_BACKGROUND = 32,///< make easyconn switch to background.
    EC_STATUS_MESSAGE_HU_BT_NOT_MATCH = 33,           ///< the HU's BT is not connected with the phone.
    EC_STATUS_MESSAGE_ENABLE_UPLOAD_GPS_INFO = 34,    ///< phone app enable HU to upload gps info.
    EC_STATUS_MESSAGE_DISABLE_UPLOAD_GPS_INFO = 35,   ///< phone app disable HU to upload gps info.
    EC_STATUS_MESSAGE_ENABLE_ACCESSIBILITY = 36,      ///< android phone enable accessibility.
    EC_STATUS_MESSAGE_DISABLE_ACCESSIBILITY = 37,     ///< android phone disable accessibility.
	EC_STATUS_MESSAGE_START_DLNA_SERVICE = 38,        ///< HU needed to start DLNA service.
	EC_STATUS_MESSAGE_STOP_DLNA_SERVICE = 39,         ///< HU needed to stop DLNA service.
	EC_STATUS_MESSAGE_MAX                        ///< reserve
};
typedef enum ECStatusMessage ECStatusMessage;

/**
* @enum ECCallType
*
* @brief call type
*
* @see ECSDK::onCallAction
*/
enum ECCallType
{
    EC_CALL_TYPE_DAIL = 0,                       ///< ring up
    EC_CALL_TYPE_HANG_UP = 1,                        ///< ring off
    EC_CALL_TYPE_MAX = 2,                            ///< reserve
};
typedef enum ECCallType ECCallType;

/**
* @enum ECMicType
*
* @brief microphone type
*/
enum ECMicType
{
    EC_MIC_TYPE_NATIVE = 0,                      ///< microphone of car
    EC_MIC_TYPE_PHONE = 1,                           ///< microphone of phone
    EC_MIC_TYPE_UNSUPPORT = 2,                       ///< no microphone
};
typedef enum ECMicType ECMicType;

/**
* @enum ECMediaLevel
*
* @brief car multimedia level
*/
enum ECMediaLevel
{
    EC_DVD_LEVEL_LOW = 0,                        ///< low-end
    EC_DVD_LEVEL_MIDDLE = 1,                         ///< middle-end
    EC_DVD_LEVEL_HIGH = 2,                           ///< high-end
};
typedef enum ECMediaLevel ECMediaLevel;

/**
*
* @enum ECScreenType
*
* @brief HU screen type
*/
enum ECScreenType
{	
	EC_CAR_SCREEN_HORIZONTAL = 0,                    ///< horizontal screen type
	EC_CAR_SCREEN_VERTICAL = 1,                      ///< vertical screen type   
	EC_CAR_SCREEN_UNKNOWN = 2,						 ///< unknown screen type
};
typedef enum ECScreenType ECScreenType;

/**
*
* @enum ECProjectFlavor
*
* @brief HU Project market for sale,This field will affect the function of SDK,
 * SDK will carry a flavor by default,EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_CN or
 * EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_OVERSEA.
*/
enum ECProjectFlavor
{
    EC_PROJECT_FLAVOR_DEFAULT = 0,                                   ///< default value
	EC_PROJECT_FLAVOR_AFTER_MARKET_INSTALLED_PRODUCTS_CN = 1,            ///< aftermarket installed products in China
	EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_CN = 2,                 ///< factory-installed products in China
	EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_OVERSEA = 3,            ///< factory-installed products oversea
	EC_PROJECT_FLAVOR_AFTER_MARKET_INSTALLED_PRODUCTS_OVERSEA = 4,        ///< aftermarket installed products oversea
};
typedef enum ECProjectFlavor ECProjectFlavor;

/**
* @struct ECAuthentication
*
* @brief car authorization info
*/
struct ECAuthentication
{
    char         uuid[1024];                     ///< the universally unique identifier of the car
    char         pwd[1024];                      ///< the specific password for authentication powered by Carbit.
    char         versionName[1024];              ///< the version name of EasyConn.
    uint32_t     versionCode;                    ///< the version code of EasyConn.
	uint32_t         autoAuthViaCar;                 ///< specify whether make automatic authentication via car's network.
    ECProjectFlavor     flavor;                  ///< specify the HU Project market for sale,SDK will carry a flavor by default.see enum ECProjectFlavor in ECTypes.h
    char         reserve[256];                   ///< reserve
};
typedef struct ECAuthentication ECAuthentication;

enum ECSupportConnect
{
	EC_SUPPORT_CONNECT_ADB = 0x01,
	EC_SUPPORT_CONNECT_AOA = 0x02,
	EC_SUPPORT_CONNECT_WIFIDIRECT = 0x08,
	EC_SUPPORT_CONNECT_EAP = 0x20,
	EC_SUPPORT_CONNECT_MUX = 0x40,
	EC_SUPPORT_CONNECT_LIGHTNING = 0x80,
    EC_SUPPORT_CONNECT_IPHONE_WIFI = 0x100,
    EC_SUPPORT_CONNECT_ANDROID_WIFI = 0x200,
    EC_SUPPORT_CONNECT_USB_AIRPLAY = 0x400,
    EC_SUPPORT_CONNECT_WIFI_AIRPLAY = 0x800
};
typedef enum ECSupportConnect ECSupportConnect;

enum ECSupportFunction
{
	EC_SUPPORT_FUNCTION_INPUT = 0x01,
	EC_SUPPORT_FUNCTION_SPEECH_WAKE = 0x02,
	EC_SUPPORT_FUNCTION_VR_TEXT = 0x04,
    EC_SUPPORT_FUNCTION_BT_MUSIC = 0x08,
    EC_SUPPORT_FUNCTION_BT_AUTO_PAIR = 0x10,
    EC_SUPPORT_FUNCTION_HU_HOTSPOT = 0x20,
    EC_SUPPORT_FUNCTION_GPS = 0x40,
    EC_SUPPORT_FUNCTION_WIFI_QR_CONNECT = 0x80,
    EC_SUPPORT_FUNCTION_BT_CALL_BY_HU = 0x100,
    EC_SUPPORT_FUNCTION_DLNA = 0x200,
    EC_SUPPORT_FUNCTION_PHONE_CONTROL_CAR = 0x400,
    EC_SUPPORT_FUNCTION_MEDIA_TRANSPORT_VIA_EC = 0x800,
    EC_SUPPORT_FUNCTION_ANDROID_PARALLEL_WORLD = 0x1000,
    EC_SUPPORT_FUNCTION_BLE_CONNECT = 0x2000
};
typedef enum ECSupportFunction ECSupportFunction;

enum ECMicFeature
{
	EC_MIC_SUPPORT_ECHO_CANCELLATION = 0x0001,
	EC_MIC_SUPPORT_ECHO_CANCELLATION_VIA_PHONE = 0x0002,
	EC_MIC_SUPPORT_LEFT_CHANNEL_RECORD = 0x0004,
};
typedef enum ECMicFeature ECMicFeature;

enum ECProductType
{
    EC_PRODUCT_TYPE_DEFAULT = 0,                 ///< default product type
    EC_PRODUCT_TYPE_DA = 1,                          ///< product type of direct account 
    EC_PRODUCT_TYPE_MU = 2,                          ///< product type mobile head unit 
    EC_PRODUCT_TYPE_NI = 3,                           ///< product type of navigation instrument 
};
typedef enum ECProductType ECProductType;

/**
* @struct ECCarDescription
*
* @brief  car description info
*/
struct  ECCarDescription
{
    ECMediaLevel mediaLevel;                     ///< level of multimedia，reserved field
    char         mediaOS[64];                    ///< os of multimedia，reserved field
    char         mediaModel[64];                 ///< multimedia system model.
	ECScreenType screenType;					 ///< HU screen type
    char         carBrand[64];                   ///< car brand
    char         carModel[64];                   ///< car model
    char         carConfig[64];                  ///< car config information
    ECMicType    micType;                        ///< microphone type
	uint32_t		 supportBTCall;					 ///< whether HU support bluetooth call.
	uint32_t         supportBTSetting;               ///< whether HU support bluetooth setting.
    char         btName[64];                     ///< bluetooth name
    char         btAddress[64];                  ///< bluetooth mac addrss
    char         btPin[32];                      ///< bluetooth pin code
	uint32_t     dpi;                            ///< dpi of screen of HU.
	uint32_t         enableDpi;                      ///< a switch which decides whether dpi can be used in new layout algorithm.
	uint32_t     supportConnect;                 ///< the supported connects of HU.
												 ///< the value of supportConnect can be the combination of ECSupportConnect
												 ///< such as (EC_SUPPORT_CONNECT_ADB | EC_SUPPORT_CONNECT_WIFIDIRECT).
    uint32_t     supportFunction;                ///< such as (EC_SUPPORT_FUNCTION_INPUT)
	uint32_t     micSupportFeature;              ///< the feature of car's microphone.
	                                             ///< the value of micSupportFeature can be the combination of ECMicFeature
	                                             ///< such as (EC_MIC_SUPPORT_ECHO_CANCELLATION | EC_MIC_SUPPORT_LEFT_CHANNEL_RECORD)
	uint32_t     productType;                    ///< the value of productType refer to ECProductType.
    char         wakeupWord[256];                ///< the value of wakeupWord will be the wakeup word of phone's app.
    char         reserve[256];                   ///< reserve
};
typedef struct ECCarDescription ECCarDescription;

enum ECMirrorMode
{
	EC_MIRROR_MODE_DEFAULT = 0,                  ///< default mirror mode.
	EC_MIRROR_MODE_FIXED_NAVIGATION              ///< fixed navigation mirror mode.
};
typedef enum ECMirrorMode ECMirrorMode;

/**
* @brief log output destination
*
* @see ECOptions::logOutputType
*/
enum ECLogOutputType
{
    EC_LOG_OUT_STD = 0,                              ///< log to std out
    EC_LOG_OUT_FILE,                                 ///< log to file
    EC_LOG_OUT_LOGCAT,                               ///< log to file only for android
    EC_LOG_OUT_SLOGINFO,                             ///< loginfo  only for qnx
};
typedef enum ECLogOutputType ECLogOutputType;

/**
* @brief log module
*
* @see ECSDK::setLogLevel
*/
enum ECLogModule
{
    EC_LOG_MODULE_SDK = 0x01,                       ///< output sdk module log
    EC_LOG_MODULE_ADB = 0x02,                       ///< output adb module log
    EC_LOG_MODULE_MUX = 0x04,                       ///< output mux module log
    EC_LOG_MODULE_USB = 0x08,                       ///< output usb module log
    EC_LOG_MODULE_APP = 0x10                        ///< output ec app module log
};
typedef enum ECLogModule ECLogModule;

/**
* @brief log level
*
* @see ECSDK::setLogLevel
*/
enum ECLogLevel
{
    EC_LOG_LEVEL_ALL = 0,                              ///< all log
    EC_LOG_LEVEL_DEBUG,                                ///< debug log
    EC_LOG_LEVEL_INFO,                                 ///< info log
    EC_LOG_LEVEL_WARN,                                 ///< warn log
    EC_LOG_LEVEL_ERROR,                                ///< error log
    EC_LOG_LEVEL_FATAL,                                ///< fatal log
    EC_LOG_LEVEL_OFF                                   ///< no log
};
typedef enum ECLogLevel ECLogLevel;

/**
* @struct ECOptions
*
* @brief  options info
*/
struct ECOptions 
{           
    char         workspace[4096];                ///< absolute path of readable and writable directory 
    uint32_t         scanADB;                        ///< whether ECSDK scan adb devices.  
    uint32_t         scanAOA;                        ///< whether ECSDK scan aoa devices.  
    uint32_t         scanIOSUsb;                     ///< whether ECSDK scan ios devices, RESERVED.
	uint32_t         supportRVForAdb;                ///< whether ECSDK support rv for adb.
	uint32_t         isAppMirrorForAdb;              ///< whether ECSDK use app mirror for adb, 
	                                             ///< if supportRVForAdb is false, isAppMirrorForAdb would be used as false.  	
	uint32_t         supportScreenMirroring;         ///< whether the app of connected phone support screen mirroring.
	uint32_t         supportThirdPartyApp;           ///< whether the app of connected phone support the third-party app.
	uint32_t         supportLandscapeAdaptive;       ///< whether the app of connected phone support landscape adaptive.
	uint32_t         supportScreenTouch;             ///< whether the HU support screen touch, it determines whether the app of the phone displays a mask.          
	uint32_t         supportOTAUpdate;               ///< make the app of connected phone decide whether support OTA Update.
	uint32_t         supportBackDesktop;             ///< whether the app of connected phone support back desktop of HU.
	ECMirrorMode mirrorMode;                     ///< tell the app of connected phone which mirror mode would be used.
    uint32_t     bluetoothPolicy;                ///< the policy of A2DP message phone sent to the car.
    uint32_t     disableShowCallInfo;            ///< true:Don't show call info
    uint32_t     socketTimeoutPeriod;            ///< socket timeout period in seconds
	char         reserve[256];                   ///< reserve
};
typedef struct ECOptions ECOptions;

/**
* @enum ECTransportType
*
* @brief transport type
*
* @see ECSDK::onPhoneConnected,  openTransport
*/
enum ECTransportType
{
    EC_TRANSPORT_ANDROID_USB_ADB = 0,            ///< android usb adb, system screen
    EC_TRANSPORT_ANDROID_USB_AOA = 1,                ///< android usb aoa, app screen
    EC_TRANSPORT_ANDROID_WIFI = 2,                   ///< android wifi, app screen
    EC_TRANSPORT_IOS_USB_EAP = 3,                    ///< iphone usb eap, app screen
    EC_TRANSPORT_IOS_USB_MUX = 4,                    ///< iphone usb mux, app screen
    EC_TRANSPORT_IOS_USB_AIRPLAY = 5,                ///< iphone usb airplay, system screen   
    EC_TRANSPORT_IOS_WIFI_APP = 6,                   ///< iphone wifi app, app screen
    EC_TRANSPORT_IOS_WIFI_AIRPLAY = 7,               ///< iphone wifi airplay, system screen
	EC_TRANSPORT_IOS_USB_LIGHTNING = 8,              ///< iphone usb lightning connect(闪连)
    EC_TRANSPORT_MAX,                            ///< reserve
};
typedef enum ECTransportType ECTransportType;

/**
* @enum ECVideoType
*
* @brief video type
*
* @see ECSDK::ECMirrorConfig
*/
enum ECVideoType
{
    EC_VIDEO_TYPE_H264 = 0,                      ///< H264
    EC_VIDEO_TYPE_MPEG4,                         ///< MPEG4
    EC_VIDEO_TYPE_JPEG,                          ///< JPEG
    EC_VIDEO_TYPE_MAX,                           ///< reserve
};
typedef enum ECVideoType ECVideoType;

/**
*
* @enum ECScreenDirection
*
* @brief HU screen direction
*/
enum ECScreenDirection
{
    EC_SCREEN_DIRECTION_UNKNOWN,						 ///< unknown screen direction
    EC_SCREEN_DIRECTION_HORIZONTAL,                    ///< horizontal screen direction
    EC_SCREEN_DIRECTION_VERTICAL,                      ///< vertical screen direction
    EC_SCREEN_DIRECTION_HORIZONTAL_ROUND,              ///< horizontal screen direction and round mirror
    EC_SCREEN_DIRECTION_VERTICAL_ROUND,                ///< vertical screen direction and round mirror
};
typedef enum ECScreenDirection ECScreenDirection;

/**
*
* @struct ECVideoArea
*
* @brief Display the area of the video screen
*/
struct ECVideoArea {
    uint8_t    present;
    uint16_t   originXPixels;
    uint16_t   originYPixels;
    uint16_t   widthPixels;
    uint16_t   heightPixels;
};
typedef struct ECVideoArea ECVideoArea;

/**
*
* @struct ECVideoView
*
* @brief Define the area that the window can display
*/
struct ECVideoView {
    uint16_t index;
    ECVideoArea viewArea;
    ECVideoArea safeArea;
};
typedef struct ECVideoView ECVideoView;

/*!
* @struct ECVideoViewConfig
*
* @brief Define viewport parameters
*/
struct ECVideoViewConfig{
    uint16_t initArea;

    int16_t viewCount;

    ECVideoView* viewGroup;    // array
};
typedef struct ECVideoViewConfig ECVideoViewConfig;

/**
 * @enum ECMirrorFeature
 *
 * @brief Define mirror feature
 */
enum ECMirrorFeature
{
    EC_MIRROR_FEATURE_DEFAULT             = 0x00,      ///< no feature
    EC_MIRROR_FEATURE_HUD_ELEMENT_NORMAL  = 0x01,      ///< HUD default layout
    EC_MIRROR_FEATURE_HUD_ELEMENT_SIMPLE  = 0x02       ///< HUD simple layout
};
typedef enum ECMirrorFeature ECMirrorFeature;

/**
* @struct ECMirrorConfig
*
* @brief mirror config type
*
* @see ECSDK::openMirrorConnection
*/
struct ECMirrorConfig
{
    ECVideoType type;                            ///< video type
    
    int         width;                           ///< video width in pixels
    
    int         height;                          ///< video height in pixels
    
    int         quality;                         ///< video quality, 
                                                 ///< bitrate for H264 or MPEG4(typically, 1024*1024*4)
                                                 ///< picture quality factor[0~100] for JPEG, 0 is worst, 100 is best. Typically, 50.
    int         touchMode;                       ///< touch mode : 0x0 Single-Touch ;0x01 Multi-touch;0x02 not support touch; Single-Touch is default mode.

    int 		capScreenMode;					 ///< capture screen mode,0x00,default;0x01,not used;0x02,disable multi thread codec;0x04, use soft codec;0x08,use quality option; developers should not fix the default value while you did not know what it means.

    ECScreenDirection screenDirection;			 ///< HU screen direction.

    ECVideoViewConfig viewConfig;

    double      screenPhysicsWidth;              ///< Physical screen width in inches

    double      screenPhysicsHeight;             ///< Physical screen height in inches

    ECMirrorFeature mirrorFeature;               ///< mirror feature

    char        reserve[248];                    ///< reserve(must be zero clearing)
};
typedef struct ECMirrorConfig ECMirrorConfig;

/**
* @enum ECSystemKeyCode
*
* @brief phone key event code
*
* @see ECSDK::sendSystemKeyEvent
*/
enum ECSystemKeyCode
{
    //The Android phone's HOME/MENU/BACK key.
    EC_SYSTEM_KEYCODE_HOME = 0,                         ///< home
    EC_SYSTEM_KEYCODE_MENU,                             ///< menu
    EC_SYSTEM_KEYCODE_BACK,                             ///< back

    //Media
    EC_SYSTEM_KEYCODE_VOLUME_UP,                        ///< volume up
    EC_SYSTEM_KEYCODE_VOLUME_DOWN,                      ///< volume down
    EC_SYSTEM_KEYCODE_MEDIA_PALY,                       ///< media play
    EC_SYSTEM_KEYCODE_MEDIA_PAUSE,                      ///< media pause
    EC_SYSTEM_KEYCODE_MEDIA_STOP,                       ///< media stop
    EC_SYSTEM_KEYCODE_MEDIA_NEXT,                       ///< media next
    EC_SYSTEM_KEYCODE_MEDIA_PREVIOUS,                   ///< media previous
    EC_SYSTEM_KEYCODE_MEDIA_REWIND,                     ///< media backward
    EC_SYSTEM_KEYCODE_MEDIA_FAST_FORWARD,               ///< media forward
    EC_SYSTEM_KEYCODE_MUTE,                             ///< media mute
    EC_SYSTEM_KEYCODE_POWER,                            ///< power

    EC_SYSTEM_KEYCODE_MAX,                              ///< reserve
};
typedef enum ECSystemKeyCode ECSystemKeyCode;

/**
* @enum ECBtnCode
*
* @brief phone app button code
*
* @see ECSDK::sendBtnEvent
*/
enum  ECBtnCode
{
    // for driving mode
    EC_BTN_TALKIE = 0x1010,                         ///< start talkie
    EC_BTN_TALKIE_GROUP_MUTE = 0x1011,              ///< talkie group mute
    EC_BTN_TALKIE_GROUP_CANCEL_MUTE = 0x1012,       ///< talkie group cancel mute
    EC_BTN_TALKIE_GROUP_SWITCH_MUTE = 0x1013,       ///< talkie group switch mute
    EC_BTN_NAVIGATION = 0x1020,                     ///< start navigation
    EC_BTN_VOICE_ASSISTANT = 0x1030,                ///< start voice assistant
    EC_BTN_MUSIC_PLAY = 0x1040,                     ///< play music
    EC_BTN_MUSIC_NEXT = 0x1041,                     ///< play next music
    EC_BTN_MUSIC_PREVIOUS = 0x1042,                 ///< play previous music
    EC_BTN_MUSIC_PAUSE = 0x1043,                    ///< music pause
    EC_BTN_MUSIC_STOP = 0x1044,			            ///< music stop        
    EC_BTN_MUSIC_PLAY_PAUSE = 0x1047,               ///< music switch between pause and stop

    EC_BTN_VOLUME_UP = 0x1050,                      ///< increase the volume of phone
    EC_BTN_VOLUME_DOWN = 0x1051,                    ///< decrease the volume of phone

    EC_BTN_TOPLEFT = 0x1060,                        ///< click top left button
    EC_BTN_TOPRIGHT = 0x1061,                       ///< click top right button
    EC_BTN_BOTTOMLEFT = 0x1062,                     ///< click bottom left button
    EC_BTN_BOTTOMRIGHT = 0x1063,                    ///< click bottom right button   

    EC_BTN_MODE = 0x1070,                           ///< click mode button    

	EC_BTN_APP_FRONT = 0x1080,						///< make the app of android phone switch to the foreground

    EC_BTN_APP_BACK = 0x1090,                       ///< it works like the function of the back button to the app of the connected phone.

    EC_BTN_ENFORCE_LANDSCAPE = 0x10A0,              ///< make the android phone enforce landscape.
	EC_BTN_CANCEL_LANDSCAPE = 0x10A1,               ///< make the android phone cancel landscape.
	EC_BTN_ENFORCE_OR_CANCEL_LANDSCAPE = 0x10A2,    ///< make the android phone enforce or cancel landscape.

    EC_BTN_SYSTEM_HOME = 0x2010,                         ///< The Android phone's HOME key
	EC_BTN_SYSTEM_BACK = 0x2012,                             ///< The Android phone's BACK key

    EC_BTN_MAX,                                     ///< reserve
};
typedef enum ECBtnCode ECBtnCode;

/**
* @enum ECBtnEventType
*
* @brief physical button action code
*
* @see ECSDK::sendBtnEvent
*/
enum  ECBtnEventType
{
	EC_BTN_TYPE_UP = 0,                          ///< key up
    EC_BTN_TYPE_DOWN = 1,                            ///< key down
    EC_BTN_TYPE_CLICK = 2,                           ///< click
    EC_BTN_TYPE_DOUBLE_CLICK = 3,                    ///< double click
    EC_BTN_TYPE_LONG_PRESS = 4,                      ///< long press

    EC_BTN_TYPE_MAX,                             ///< reserve
};
typedef enum ECBtnEventType ECBtnEventType;

/**
* @enum ECTouchEventType
*
* @brief touch event type
*
* @see ECSDK::sendTouchEvent
*/
enum ECTouchEventType
{
    EC_TOUCH_UP = 0,                             ///< touch up
    EC_TOUCH_DOWN = 1,                               ///< touch down
    EC_TOUCH_MOVE = 2,                               ///< touch move
};
typedef enum ECTouchEventType ECTouchEventType;

/**
* @struct ECTouchEventData
*
* @brief touch data struct
*
* @see ECSDK::sendTouchEvent
*/
struct ECTouchEventData
{
    unsigned short    pointX;                    ///< touch point x
    unsigned short    pointY;                    ///< touch point y
    unsigned short    slot;                      ///< multi touch slot(default is 0)
    char              reserve[32];               ///< reserve
};
typedef struct ECTouchEventData ECTouchEventData;

/**
* @enum ECGearType
*
* @brief car gear type
*
* @see ECSDK::uploadGearStatus
*/
enum ECGearType
{
    EC_GEAR_NEUTRUAL = 0,                        ///< neutral
    EC_GEAR_MANUAL_1st,                          ///< manual first
    EC_GEAR_MANUAL_2nd,                          ///< manual second
    EC_GEAR_MANUAL_3rd,                          ///< manual third
    EC_GEAR_MANUAL_4th,                          ///< manual forth
    EC_GEAR_MANUAL_5th,                          ///< manual fifth
    EC_GEAR_MANUAL_6th,                          ///< manual sixth
    EC_GEAR_MANUAL_7th,                          ///< manual seventh
    EC_GEAR_MANUAL_8th,                          ///< manual eighth
    EC_GEAR_MANUAL_9th,                          ///< manual ninth
    EC_GEAR_MANUAL_10th,                         ///< manual tenth

    EC_GEAR_AUTO_DRIVE = 100,                    ///< automatic drive
    EC_GEAR_AUTO_PARK,                           ///< automatic park
    EC_GEAR_AUTO_REVERSE,                        ///< automatic reverse
};
typedef enum ECGearType ECGearType;

/**
* @enum ECDrivingStatus
*
* @brief driving status
*
* @see ECSDK::uploadDrivingStatus
*/
enum ECDrivingStatus
{
    EC_DRIVING_FREE = 0x0,                       ///< no limited
    EC_DRIVING_NO_VIDEO = 0x01,                  ///< no video
    EC_DRIVING_NO_KEYBOARD_INPUT = 0x02,         ///< no keyboard input
    EC_DRIVING_NO_VOICE_INPUT = 0x04,            ///< no voice
    EC_DRIVING_NO_CONFIG = 0x08,                 ///< no config
};
typedef enum ECDrivingStatus ECDrivingStatus;

/**
* @enum ECHeadLightStatus
*
* @brief car headlight status
*
* @see ECSDK::uploadLightStatus
*/
enum ECHeadLightStatus
{
    EC_HEADLIGHT_OFF = 0,                        ///< headlight off
    EC_HEADLIGHT_ON,                             ///< headlight on
    EC_HEADLIGHT_HIGH,                           ///< high-beam on
};
typedef enum ECHeadLightStatus ECHeadLightStatus;

/**
* @enum ECTurnIndicatorStatus
*
* @brief car's turning indicator status
*
* @see ECSDK::uploadLightStatus
*/
enum ECTurnIndicatorStatus
{
    EC_TURNINDICATOR_NONE = 0,                   ///< indicator off
    EC_TURNINDICATOR_LEFT,                       ///< left-hand indicator on
    EC_TURNINDICATOR_RIGHT,                      ///< right-hand indicator on
};
typedef enum ECTurnIndicatorStatus ECTurnIndicatorStatus;

/**
* @enum ECSensorType
*
* @brief car sensor type
*
* @see ECSDK::uploadSensorError
*/
enum ECSensorType
{
    EC_SENSOR_LOCATION = 1,                      ///< sensor location
    EC_SENSOR_COMPASS,                           ///< sensor compass
    EC_SENSOR_SPEED,                             ///< sensor speed
    EC_SENSOR_RPM,                               ///< sensor rpm
    EC_SENSOR_ODOMETER,                          ///< sensor odometer
    EC_SENSOR_FUEL,                              ///< sensor fuel
    EC_SENSOR_PARKING_BRAKE,                     ///< sensor handbrake
    EC_SENSOR_GEAR,                              ///< sensor gear
    EC_SENSOR_NIGHT_MODE,                        ///< sensor night-mode
    EC_SENSOR_ENV_STATUS,                        ///< sensor car environment
    EC_SENSOR_DRIVING_STATUS,                    ///< sensor driving status
    EC_SENSOR_PASSENGER_STATUS,                  ///< sensor passenger status
    EC_SENSOR_DOOR_STATUS,                       ///< sensor door status
    EC_SENSOR_LIGHT_STATUS,                      ///< sensor light status
    EC_SENSOR_TIRE_PRESSURE_STATUS,              ///< sensor tire pressure status
    EC_SENSOR_ACCLEROMETER_STATUS,               ///< sensor accelerated status
    EC_SENSOR_GYROSCOPE_STATUS,                  ///< sensor gyroscopes status
    EC_SENSOR_GPS_SATELLITE_STATUS,              ///< sensor GPS
    EC_SENSOR_MAX                                ///< reserve
};
typedef enum ECSensorType ECSensorType;

/**
* @enum ECSensorErrorType
*
* @brief cat sensor error type
*
* @see ECSDK::uploadSensorError
*/
enum ECSensorErrorType
{
    EC_SENSORERROR_OK = 0,                       ///< sensor ok
    EC_SENSORERROR_TRANSIENT,                    ///< sensor transient error
    EC_SENSORERROR_PERMANENT,                    ///< sensor permanent error
};
typedef enum ECSensorErrorType ECSensorErrorType;

enum ECNaviStatus
{
	EC_NAVI_STATUS_ACTIVE = 0,
	EC_NAVI_STATUS_INACTIVE,
	EC_NAVI_STATUS_MAX
};
typedef enum ECNaviStatus ECNaviStatus;

enum ECNaviCameraType
{
	EC_NAVI_CAMERA_SPEED = 0,
	EC_NAVI_CAMERA_SPY,
	EC_NAVI_CAMERA_REDLIGHT,
	EC_NAVI_CAMERA_VIOLATION,
	EC_NAVI_CAMERA_BUS,
	EC_NAVI_CAMERA_EMERGENCY,
	EC_NAVI_CAMERA_MAX
};
typedef enum ECNaviCameraType ECNaviCameraType;

enum ECNaviIcon
{
    EC_NAVI_ICON_NONE                          = 0,               ///< 收到此值，不显示导航图标
    EC_NAVI_ICON_DEFAULT                       = 1,               ///< 自车.请忽略这个元素，从左转图标开始
    EC_NAVI_ICON_LEFT                          = 2,               ///< 左转
    EC_NAVI_ICON_RIGHT                         = 3,               ///< 右转
    EC_NAVI_ICON_LEFT_FRONT                    = 4,               ///< 左前方
    EC_NAVI_ICON_RIGHT_FRONT                   = 5,               ///< 右前方
    EC_NAVI_ICON_LEFT_BACK                     = 6,               ///< 左后方
    EC_NAVI_ICON_RIGHT_BACK                    = 7,               ///< 右后方
    EC_NAVI_ICON_LEFT_TURN_AROUND              = 8,               ///< 左转掉头
    EC_NAVI_ICON_STRAIGHT                      = 9,               ///< 直行
    EC_NAVI_ICON_ARRIVED_WAYPOINT              = 10,              ///< 到达途经点
    EC_NAVI_ICON_ENTER_ROUNDABOUT              = 11,              ///< 进入环岛
    EC_NAVI_ICON_OUT_ROUNDABOUT                = 12,              ///< 驶出环岛
    EC_NAVI_ICON_ARRIVED_SERVICE_AREA          = 13,              ///< 到达服务区
    EC_NAVI_ICON_ARRIVED_TOLLGATE              = 14,              ///< 到达收费站
    EC_NAVI_ICON_ARRIVED_DESTINATION           = 15,              ///< 到达目的地
    EC_NAVI_ICON_ARRIVED_TUNNEL                = 16,              ///< 到达隧道
    EC_NAVI_ICON_CROSSWALK                     = 17,              ///< 通过人行横道
    EC_NAVI_ICON_OVERPASS                      = 18,              ///< 通过过街天桥
    EC_NAVI_ICON_UNDERPASS                     = 19,              ///< 通过地下通道
    EC_NAVI_ICON_SQUARE                        = 20,              ///< 通过广场
    EC_NAVI_ICON_PARK                          = 21,              ///< 通过公园
    EC_NAVI_ICON_STAIRCASE                     = 22,              ///< 通过扶梯
    EC_NAVI_ICON_LIFT                          = 23,              ///< 通过直梯
    EC_NAVI_ICON_CABLEWAY                      = 24,              ///< 通过索道
    EC_NAVI_ICON_SKY_CHANNEL                   = 25,              ///< 通过空中通道
    EC_NAVI_ICON_CHANNEL                       = 26,              ///< 通过通道、建筑物穿越通道
    EC_NAVI_ICON_WALK_ROAD                     = 27,              ///< 通过行人道路
    EC_NAVI_ICON_CRUISE_ROUTE                  = 28,              ///< 通过游船路线
    EC_NAVI_ICON_SIGHTSEEING_BUSLINE           = 29,              ///< 通过观光车路线
    EC_NAVI_ICON_SLIDEWAY                      = 30,              ///< 通过滑道
    EC_NAVI_ICON_LADDER                        = 31,              ///< 通过阶梯
    EC_NAVI_ICON_MERGE_LEFT                    = 51,              ///< 靠左行驶
    EC_NAVI_ICON_MERGE_RIGHT                   = 52,              ///< 靠右行驶
    EC_NAVI_ICON_SLOW                          = 53,              ///< 减速慢行
    EC_NAVI_ICON_ENTRY_RING_LEFT               = 54,              ///< 标准小环岛 绕环岛左转，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ENTRY_RING_RIGHT              = 55,              ///< 标准小环岛 绕环岛右转，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ENTRY_RING_CONTINUE           = 56,              ///< 标准小环岛 绕环岛直行，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ENTRY_RING_UTURN              = 57,              ///< 标准小环岛 绕环岛调头，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ENTRY_LEFT_RING               = 58,              ///< 进入环岛图标，左侧通行地区的顺时针环岛
    EC_NAVI_ICON_LEAVE_LEFT_RING               = 59,              ///< 驶出环岛图标，左侧通行地区的顺时针环岛
    EC_NAVI_ICON_UTURN_RIGHT                   = 60,              ///< 右转掉头图标，左侧通行地区的掉头
    EC_NAVI_ICON_SPECIAL_CONTINUE              = 61,              ///< 顺行图标(和直行有区别，顺行图标带有虚线)
    EC_NAVI_ICON_ENTRY_LEFT_RING_LEFT          = 62,              ///< 标准小环岛 绕环岛左转，左侧通行地区的顺时针环岛
    EC_NAVI_ICON_ENTRY_LEFT_RING_RIGHT         = 63,              ///< 标准小环岛 绕环岛右转，左侧通行地区的顺时针环岛
    EC_NAVI_ICON_ENTRY_LEFT_RING_CONTINUE      = 64,              ///< 标准小环岛 绕环岛直行，左侧通行地区的顺时针环岛
    EC_NAVI_ICON_ENTRY_LEFT_RING_UTURN         = 65,              ///< 标准小环岛 绕环岛调头，左侧通行地区的顺时针环岛
    EC_NAVI_ICON_SLOPE                         = 66,              ///< 通过斜坡图标
    EC_NAVI_ICON_BRIDGE                        = 67,              ///< 通过桥图标
    EC_NAVI_ICON_FERRYBOAT                     = 68,              ///< 通过渡轮图标
    EC_NAVI_ICON_SUBWAY                        = 69,              ///< 通过地铁图标
    EC_NAVI_ICON_ENTER_BUILDING                = 70,              ///< 进入建筑物图标
    EC_NAVI_ICON_LEAVE_BUILDING                = 71,              ///< 离开建筑物图标
    EC_NAVI_ICON_BY_ELEVATOR                   = 72,              ///< 电梯换层图标
    EC_NAVI_ICON_BY_STAIR                      = 73,              ///< 楼梯换层图标
    EC_NAVI_ICON_ESCALATOR                     = 74,              ///< 扶梯换层图标
    EC_NAVI_ICON_LOW_TRAFFIC_CROSS             = 75,              ///< 非导航段通过红绿灯路口图标
    EC_NAVI_ICON_LOW_CROSS                     = 76,              ///< 非导航段通过普通路口图标
    EC_NAVI_ICON_ROTARY_SHARP_LEFT             = 77,              ///< 环岛左后转，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ROTARY_SHARP_RIGHT            = 78,              ///< 环岛后右转，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ROTARY_SLIGHT_LEFT            = 79,              ///< 环岛左前转，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_ROTARY_SLIGHT_RIGHT           = 80,              ///< 环岛右前转，右侧通行地区的逆时针环岛
    EC_NAVI_ICON_MAX
};
typedef enum ECNaviIcon ECNaviIcon;

enum ECGpsSignalIntensity {
    EC_GPS_SIGNAL_DEFAULT                     = 0,               ///< GPS信号初始值
    EC_GPS_SIGNAL_WEAK                        = 1,               ///< GPS信号弱
    EC_GPS_SIGNAL_STRONG                      = 2               ///< GPS信号强
};
typedef enum ECGpsSignalIntensity ECGpsSignalIntensity;


/*
* @struct ECNavigationHudInfo
*
* @brief navigation HUD info
*
* @see   IECCallback::onPhoneAppHUD
*/
struct ECNavigationHudInfo
{
	ECNaviStatus status;
	char* currentRoad;
	int32_t	carDirection;
	ECNaviCameraType cameraType;
	int32_t	cameraSpeed;
	int32_t	cameraDistance;
	ECNaviIcon naviIcon;
	char* nextRoad;
	int32_t roadRemainingDistance;
	int32_t roadRemainingTime;
	int32_t destinationRemainingDistance;
	int32_t destinationRemainingTime;
	uint64_t arriveTime;
	char*   arriveTimeZone;
    int32_t signalIntensity;
};
typedef struct ECNavigationHudInfo ECNavigationHudInfo;

/*
* @enum ECAudioChannelType
*
* @brief audio channel type
*
*/
enum ECAudioChannelType
{
    EC_AUDIO_CHANNEL_MONO = 1,                   ///< mono channel
    EC_ADUIO_CHANNEL_STEREO                      ///< stereo channel
};
typedef enum ECAudioChannelType ECAudioChannelType;

/*
* @enum ECAudioFormatType
*
* @brief audio format type
*
*/
enum ECAudioFormatType
{
    EC_AUDIO_FORMAT_U8 = 0,                      ///< PCM unsigned 8 bits
    EC_AUDIO_FORMAT_S8 = 1,                          ///< PCM signed 8 bits
    EC_AUDIO_FORMAT_U16_LE = 2,                      ///< PCM unsigned little endian 16 bits
    EC_AUDIO_FORMAT_S16_LE = 3,                      ///< PCM signed little endian 16 bits
    EC_AUDIO_FORMAT_U16_BE = 4,                      ///< PCM unsigned big endian 16 bits
    EC_AUDIO_FORMAT_S16_BE = 5,                      ///< PCM signed big endian 16 bits
    EC_AUDIO_FORMAT_U24_LE = 6,                      ///< PCM unsigned little endian 24 bits
    EC_AUDIO_FORMAT_S24_LE = 7,                      ///< PCM signed little endian 24 bits
    EC_AUDIO_FORMAT_U24_BE = 8,                      ///< PCM unsigned big endian 24 bits
    EC_AUDIO_FORMAT_S24_BE = 9,                      ///< PCM signed big endian 24 bits
    EC_AUDIO_FORMAT_U32_LE = 10,                      ///< PCM unsigned little endian 32 bits
    EC_AUDIO_FORMAT_S32_LE = 11,                      ///< PCM signed little endian 32 bits
    EC_AUDIO_FORMAT_U32_BE = 12,                      ///< PCM unsigned big endian 32 bits
    EC_AUDIO_FORMAT_S32_BE = 13,                      ///< PCM signed big endian 32 bits
    EC_AUDIO_FROMAT_F32 = 14,                          ///< PCM float 32 bits
};
typedef enum ECAudioFormatType ECAudioFormatType;

enum ECAudioType
{
    EC_AUDIO_TYPE_TTS     = 0x01,                   ///< TTS audio
    EC_AUDIO_TYPE_VR      = 0x02,                   ///< VR audio
    EC_AUDIO_TYPE_TALKIE  = 0x04,                   ///< IM audio
    EC_AUDIO_TYPE_MUSIC   = 0x08                    ///< Music audio
};
typedef enum ECAudioType ECAudioType;

/*
* @struct ECAudioInfo
*
* @brief audio data information
*
* @see   IECCallback::onPhoneAppTTSStart, IECCallback::onPhoneAppVRStart
*/
struct ECAudioInfo
{
    uint32_t               sampleRate;            ///< sample rate
    ECAudioChannelType     channel;               ///< channel type
    ECAudioFormatType      format;                ///< audio format type
};
typedef struct ECAudioInfo ECAudioInfo;

/*
* @enum ECAppPage
*
* @brief app page
*
*/
enum ECAppPage
{
    EC_APP_PAGE_NAVIGATION = 1,                   ///< navigation page
    EC_APP_PAGE_MUSIC = 2,                        ///< music page
    EC_APP_PAGE_VR = 3,                           ///< voice assistance page
    EC_APP_PAGE_TALKIE = 4,                       ///< talkie page
    EP_APP_PAGE_NAVI_HOME = 5,                    ///< navigation to go home
    EC_APP_PAGE_NAVI_WORK = 6,                    ///< navigation to go to work
    EC_APP_PAGE_MAIN = 7,                         ///< main page
    EC_APP_PAGE_NAVI_GAS_STATION = 8,             ///< navigation to go to gas station
    EC_APP_PAGE_CAR_PARK = 9,                     ///< navigation to go to car park
    EC_APP_PAGE_4S_SHOP = 10,                     ///< navigation to 4s shop
    EC_APP_PAGE_MUSIC_XMLY = 11,                  ///< music of XMLY
    EC_APP_PAGE_MUSIC_QQ = 12,                    ///< music of QQ
    EC_APP_PAGE_MUSIC_RADIO = 13,                 ///< web radio
    EC_APP_PAGE_MUSIC_NATIVE = 14,                ///< music of local
    EC_APP_PAGE_WECHAT = 15,                      ///< wechat
    EC_APP_PAGE_PERSONAL_CENTER = 16,             ///< personal center
    EC_APP_PAGE_APP_MANAGER = 17,                 ///< the manager of the thirdparty app
    EC_APP_PAGE_ADD_OTA = 18                      ///< the OTA download
};
typedef enum ECAppPage ECAppPage;

enum ECCarStatusType
{
	EC_CAR_REVERSING = 1,                         ///< car reverse    
	EC_CAR_BLUETOOTH = 2,                             ///< car bluetooth
	EC_CAR_DRIVINGMODE = 3,                           ///< car driving mode
	EC_CAR_AUDIO_FOCUS_CHANGE = 4,                    ///< car audio focus change
	EC_CAR_IS_AUTO_START_EASYCONN = 5,                 ///< whether car auto start easyconn
};
typedef enum ECCarStatusType ECCarStatusType;

enum ECCarStatusValue
{
	EC_CAR_STATUS_FALSE = 0,
	EC_CAR_STATUS_TRUE = 1,

	EC_CAR_STATUS_STARTED = 2,
	EC_CAR_STATUS_STOPPED = 3,

	EC_CAR_STATUS_CLOSED = 4,
	EC_CAR_STATUS_UNCONNECTED = 5,
	EC_CAR_STATUS_CONNECTED = 6,
	EC_CAR_STATUS_CONNECTED_SAME = 7,

	EC_CAR_STATUS_LONG_FUCUS_GAIN = 8,
	EC_CAR_STATUS_LONG_FUCUS_LOSS = 9,
	EC_CAR_STATUS_SHORT_FUCUS_GAIN = 10,
	EC_CAR_STATUS_SHORT_FUCUS_LOSS = 11,
	EC_CAR_STATUS_FADEDOWN_FUCUS_GAIN = 12,
	EC_CAR_STATUS_FADEDOWN_FUCUS_LOSS = 13,
};
typedef enum ECCarStatusValue ECCarStatusValue;

/*
* @struct ECCarCmd
*
* @brief the struct of voice command, the attribute of "cmd" in ECCarCmd can be regular expression,
*        such as "(打开|开启)空调", it equals "打开空调" or "开启空调".
* 
* @see   ECSDK::registerCarCmds, IECCallback::onCarCmdNotified
*/
struct ECCarCmd
{
	char* id;                                     ///< same function command can have same id. 
	char* cmd;                                    ///< the command content matched, the content can be regular expression.
	char* vrText;                                 ///< vrText is just used in IECCallback::onCarCmdNotified, the mathched command content.
	uint32_t  pauseMusic;                             ///< pauseMusic is just used in ECSDK::registerCarCmds, it tell the connected phone whether pause music when the command was triggered.
};
typedef struct ECCarCmd ECCarCmd;

enum ECOTAUpdateErrorCode
{
	EC_OTA_ERROR_SOFTWARE_DOWNLOAD_FAILED_VIA_NETWORK = -1,
	EC_OTA_ERROR_SOFTWARE_SPACE_NOT_ENOUGH = -2,
	EC_OTA_ERROR_SOFTWARE_NETWORK_UNAVAILABLE = -3,
	EC_OTA_ERROR_SOFTWARE_DOWNLOAD_FAILED_VIA_PHONE = -4
};
typedef enum ECOTAUpdateErrorCode ECOTAUpdateErrorCode;

struct ECOTAUpdateSoftware
{
	const char* softwareId;                       ///< id of the software.
	const char* softwareName;                     ///< name of the software.	
    uint32_t    softwareSize;                     ///< size of the software.
	uint32_t    versionCode;                      ///< current version code of the software.
	const char* vcDesc;                           ///< the description of the version code.
	const char* vcTitle;                          ///< the title of current version software.
	const char* vcDetail;                         ///< the detail of current version software.
	const char* createTime;                       ///< the create time of current version software.
	const char* modifyTime;                       ///< the modify time of current version software.
	uint8_t     isFullDist;                       ///< 1 means full package, 0 means incremental package.
	const char* packagePath;                      ///< the path of the software in the HU.
	const char* md5Path;                          ///< the path of the md5 file in the HU.
	const char* iconPath;                         ///< the path of the icon in the HU. 	   
    uint32_t        isRemote;                         ///< true means software need to be downloaded from remote to phone.
};
typedef struct ECOTAUpdateSoftware ECOTAUpdateSoftware;

enum ECOTAUpdateCheckMode
{
    EC_OTA_CHECK_VIA_DEFAULT = 0,                 ///< let sdk decide which way to check ota update.
    EC_OTA_CHECK_VIA_NETWORK,                     ///< use HU's network to check ota update.
    EC_OTA_CHECK_VIA_PHONE,                       ///< use the connected phone to check ota update.
	EC_OTA_CHECK_ONLY_LOCAL                       ///< check local only to gain specified downloaded software.
};
typedef enum ECOTAUpdateCheckMode ECOTAUpdateCheckMode;

/*!
 * \brief Message argument used to change EasyCon language.
 *
 * This is a message argument related to the message value \ref EC_SYS_CMD_COMM_CHANGE_LANG
 */
enum ECLanguage
{
    EC_LANG_ZH_CN = 0, /*! Simplified Chinese.*/    // 简体中文
    EC_LANG_EN_EN,     /*! English. */              // 英文
    EC_LANG_ZH_TW,     /*! Traditional Chinese. */  // 繁体中文
    EC_LANG_AR_AE,     /*! Arabic. */               // 阿拉伯文
    EC_LANG_RU_RU,     /*! Russian. */              // 俄语
    EC_LANG_ES_ES,     /*! Spanish. */              // 西班牙语
    EC_LANG_FA_FA,     /*! Persian. */              // 波斯语
    EC_LANG_DE_GE,     /*! German. */               // 德语
    EC_LANG_FR_FR,     /*! French. */               // 法语
    EC_LANG_IT_IT,     /*! Italian. */              // 意大利语
    EC_LANG_PT_BR,     /*! Portuguese. */           // 葡萄牙语
    EC_LANG_IW_IL,     /*! Hebrew. */               // 希伯来语
};
typedef enum ECLanguage ECLanguage;

/*!
 * \brief The ECOTAResourceVersion struct
 *
 * \see ECSDKAPP::ECOTAConfig
 */
typedef struct {
    const char* softWareId;                             ///< Unique identification of resources
    uint32_t    softVersion;                            ///< Version of the local package
}ECOTAResourceVersion;

/*!
 * \brief The ECOTAConfig struct \ref   ECSDKAPP::ECSDKOTAUpdate::initialize
 *
 */
struct ECOTAConfig{
    ECOTAResourceVersion*  otaResourceVersionList;        ///< Upgrade the resource version list
    uint32_t listNum;                                  ///< The amount of otaResourceVersionList
};
typedef struct ECOTAConfig ECOTAConfig;


enum ECAuthSuccessCode
{
	EC_CAR_NETWORK_AUTH_CHECK_UUID_LICENSE = 0x1010,
	EC_CAR_NETWORK_AUTH_REGISTER_UUID_LICENSE = 0x1020,
	EC_CAR_NETWORK_AUTH_DOWNLOAD_UUID_LICENSE = 0x1030,
	EC_PHONE_NETWORK_AUTH_CHECK_UUID_LICENSE = 0x2010,
	EC_PHONE_NETWORK_AUTH_REGISTER_UUID_LICENSE = 0x2020,
	EC_PHONE_NETWORK_AUTH_DOWNLOAD_UUID_LICENSE = 0x2030
};
typedef enum ECAuthSuccessCode ECAuthSuccessCode;

enum ECMusicStatus
{
	EC_MUSIC_STATUS_PLAYING = 1,                  ///< music is playing.
	EC_MUSIC_STATUS_PAUSED,                       ///< music was paused.
	EC_MUSIC_STATUS_STOPPED,                      ///< music was stopped it is not used yet.
	EC_MUSIC_STATUS_PENDING                       ///< music is pending, it is not used yet.
};
typedef enum ECMusicStatus ECMusicStatus;

struct ECAppMusicInfo
{
	ECMusicStatus status;                         ///< the status of the song.
	const char*   title;                          ///< the name of the song.
	const char*   artist;                         ///< the artist of the song.
	const char*   album;                          ///< the album of the song.
	const char*   albumArtist;                    ///< the artist of the album.
	uint64_t      length;                         ///< the total time of the song, in ms.
};
typedef struct ECAppMusicInfo ECAppMusicInfo;

enum ECDisplayRotation
{
    EC_DISPLAY_ROTATION_0 = 0,
    EC_DISPLAY_ROTATION_90,
    EC_DISPLAY_ROTATION_180,
    EC_DISPLAY_ROTATION_270
};
typedef enum ECDisplayRotation ECDisplayRotation;

enum ECFlavor
{
    EC_FLAVOR_AE_AFTER_MARKET_INSTALLED = 0x00,
    EC_FLAVOR_AE_FACTORY_INSTALLED = 0x01,
    EC_FLAVOR_AE_OVERSEA_FACTORY_INSTALLED = 0x02,
    EC_FLAVOR_AE_FREE = 0x03,
    EC_FLAVOR_AE_OVERSEA_AFTER_MARKET_INSTALLED = 0x04,
    EC_FLAVOR_GWM_EC = 0x10,
    EC_FLAVOR_XIANDOU_ORA = 0x11,
    EC_FLAVOR_XIANDOU_HAVAL = 0x12,
    EC_FLAVOR_FORD = 0x13,
    EC_FLAVOR_YLTP = 0x15,
	EC_FLAVOR_MOTOFUN_CHINA = 0x30,
	EC_FLAVOR_MOTOFUN_OVERSEA = 0x31,
	EC_FLAVOR_MOTOFUN_MINI_CHINA = 0x32,
	EC_FLAVOR_MOTOFUN_MINI_OVERSEA = 0x33,
    EC_FLAVOR_GUANGYANG_EC = 0x34,
};
typedef enum ECFlavor ECFlavor;

enum ECBluetoothPolicy
{
    EC_BLUETOOTH_POLICY_SEND_DEFAULT_WITH_A2DP_CONNECTED_ONLY = 0x00,       			///<The app will send A2DP message to the HU while app come out to the system Mirroring ,and phone's A2DP connected to the HU.
    EC_BLUETOOTH_POLICY_SEND_DEFAULT_WITH_HFP_OR_A2DP_CONNECTED  = 0x01, 				///<The app will send A2DP message to the HU while app come out to the system Mirroring, whether phone's A2DP connected or HFP connected to HU.
    EC_BLUETOOTH_POLICY_ALWAYS_ACCEPT_BTN = 0x02                                        ///<The app will response to btn events no matter phone's bluetooth is connected.
};
typedef enum ECBluetoothPolicy ECBluetoothPolicy;

struct ECVideoInfo
{
    uint32_t realWidth;                          ///< the real width of video frame in pixels.
    uint32_t realHeight;                         ///< the real height of video frame in pixels.
    uint32_t mirrorWidth;                        ///< the mirror width of video frame in pixels.
    uint32_t mirrorHeight;                       ///< the mirror height of video frame in pixels.
    int32_t  direction;                          ///< the direction of video frame. -1: video file stream, 0: up vertical screen, 3: down vertical screen,4: left horizontal screen,7:right horizontal screen.
};
typedef struct ECVideoInfo ECVideoInfo;

/**
 * The type of the editable content.
 */
enum ECInputType
{
    EC_INPUT_TYPE_NONE = 0x00000000,
    EC_INPUT_TYPE_TEXT = 0x00000001,
	EC_INPUT_TYPE_TEXT_CAP_CHARACTERS = 0x00001001,
	EC_INPUT_TYPE_TEXT_CAP_WORDS = 0x00002001,
	EC_INPUT_TYPE_TEXT_CAP_SEQUENCE = 0x00004001,
	EC_INPUT_TYPE_TEXT_AUTO_CORRECT = 0x00008001,
	EC_INPUT_TYPE_TEXT_AUTO_COMPLETE = 0x00010001,
	EC_INPUT_TYPE_TEXT_MULTI_LINES = 0x00020001,
	EC_INPUT_TYPE_TEXT_IME_MULTI_LINES = 0x00040001,
	EC_INPUT_TYPE_TEXT_NO_SUGGESTIONS = 0x00080001,
	EC_INPUT_TYPE_TEXT_URI = 0x00000011,
	EC_INPUT_TYPE_TEXT_EMAIL_ADDRESS = 0x00000021,
	EC_INPUT_TYPE_TEXT_EMAIL_SUBJECT = 0x00000031,
	EC_INPUT_TYPE_TEXT_SHORT_MESSAGE = 0x00000041,
	EC_INPUT_TYPE_TEXT_LONG_MESSAGE = 0x00000051,
	EC_INPUT_TYPE_TEXT_PERSON_NAME = 0x00000061,
	EC_INPUT_TYPE_TEXT_POSTAL_ADDRESS = 0x00000071,
	EC_INPUT_TYPE_TEXT_PASSWORD = 0x00000081,
	EC_INPUT_TYPE_TEXT_VISIBLE_PASSWORD = 0x00000091,
	EC_INPUT_TYPE_TEXT_WEB_EDIT_TEXT = 0x000000a1,
	EC_INPUT_TYPE_TEXT_FILTER = 0x000000b1,
	EC_INPUT_TYPE_TEXT_PHONETIC = 0x000000c1,
	EC_INPUT_TYPE_TEXT_WEB_EMAIL_ADDRESS = 0x000000d1,
	EC_INPUT_TYPE_TEXT_WEB_PASSWORD = 0x000000e1,
	EC_INPUT_TYPE_NUMBER = 0x00000002,
	EC_INPUT_TYPE_NUMBER_SIGNED = 0x00001002,
	EC_INPUT_TYPE_NUMBER_DECIMAL = 0x00002002,
	EC_INPUT_TYPE_NUMBER_PASSWORD = 0x00000012,
	EC_INPUT_TYPE_PHONE = 0x00000003,
	EC_INPUT_TYPE_DATETIME = 0x00000004,
	EC_INPUT_TYPE_DATE = 0x00000014,
	EC_INPUT_TYPE_TIME = 0x00000024
};
typedef enum ECInputType ECInputType;

/**
 * The type of the Input Method Editor (IME).
 * imeOptions may be combined with variations and flags to indicate desired behaviors.
 */
enum ECInputImeOptions
{
    EC_INPUT_IME_ACTION_UNSPECIFIED = 0x00000000,
	EC_INPUT_IME_ACTION_NONE = 0x00000001,
	EC_INPUT_IME_ACTION_GO = 0x00000002,
	EC_INPUT_IME_ACTION_SEARCH = 0x00000003,
	EC_INPUT_IME_ACTION_SEND = 0x00000004,
	EC_INPUT_IME_ACTION_NEXT = 0x00000005,
	EC_INPUT_IME_ACTION_DONE = 0x00000006,
	EC_INPUT_IME_ACTION_PREVIOUS = 0x00000007,
	EC_INPUT_IME_FLAG_NO_PERSONALIZED_LEARNING = 0x01000000,
	EC_INPUT_IME_FLAG_NO_FULL_SCREEN = 0x02000000,
	EC_INPUT_IME_FLAG_NAVIGATE_PREVIOUS = 0x04000000,
	EC_INPUT_IME_FLAG_NAVIGATE_NEXT = 0x08000000,
	EC_INPUT_IME_FLAG_NO_EXTRACT_UI = 0x10000000,
	EC_INPUT_IME_FLAG_NO_ACCESSORY_ACTION = 0x20000000,
	EC_INPUT_IME_FLAG_NO_ENTER_ACTION = 0x40000000,
	EC_INPUT_IME_FLAG_FORCE_ASCII = 0x80000000
};
typedef enum ECInputImeOptions ECInputImeOptions;

/**
 * inputType see ECInputType.
 * imeOptions see ECInputImeOptions.
 * imeOptions can be combined with action and flag.
 */
struct ECInputInfo
{
    int32_t  inputType;
    int32_t  imeOptions;
	const char* rawText;
	uint32_t minLines;
	uint32_t maxLines;
	uint32_t maxLength;
};
typedef struct ECInputInfo ECInputInfo;

struct ECForwardInfo
{
    char     ip[64];
    uint32_t localPort;
    uint32_t remotePort;  
};
typedef struct ECForwardInfo ECForwardInfo;

enum ECVRTextType
{
    EC_VR_TEXT_FROM_UNKNOWN =0,
	EC_VR_TEXT_FROM_VR,
	EC_VR_TEXT_FROM_SPEAK    
};
typedef enum ECVRTextType ECVRTextType;

struct ECVRTextInfo
{
    ECVRTextType type;
    int32_t      sequence;
    char         plainText[4096];  
    char         htmlText[4096];
};
typedef struct ECVRTextInfo ECVRTextInfo;

struct ECPageInfo
{
    int32_t  page;
    char     name[128];
    char     iconMd5[65];
};
typedef struct ECPageInfo ECPageInfo;

enum ECIconFormat
{
    EC_ICON_PNG = 1,
    EC_ICON_JPG =2
};
typedef enum ECIconFormat ECIconFormat;

struct ECIconInfo
{
    int32_t page;
    int32_t iconFormat;
    char*   iconData;
    int32_t iconLength;
};
typedef struct ECIconInfo ECIconInfo;

enum ECQrAction
{
    EC_QR_ACTION_DEFAULT                       = 0x00, ///< we use it when we are not sure current connect type,
    EC_QR_ACTION_WIFI_AP_MODE_CUSTOMIZED       = 0x01, ///< connect via wifi AP mode which customized and it can not access internet Normally.
    EC_QR_ACTION_WIFI_AP_MODE_ACCESS_INTERNET  = 0x02, ///< connect via wifi AP mode which can access internet.
    EC_QR_ACTION_WIFI_STATION_MODE             = 0x04, ///< connect via wifi Station mode
    EC_QR_ACTION_WIFI_P2P_MODE                 = 0x08, ///< connect via wifi P2P mode
    EC_QR_ACTION_USB_ANDROID                   = 0x10, ///< connect via Android USB
    EC_QR_ACTION_USB_IPHONE                    = 0x20, ///< connect via iPhone USB
    EC_QR_ACTION_BT                            = 0x40,  ///< connect via BT
    EC_QR_ACTION_BLE                           = 0x80  ///<
};
typedef enum ECQrAction ECQrAction;

struct ECQRInfo
{
    char ssid[32];                              ///< 网络配置，热点无线信号名称
    char pwd[16];                                ///< 网络配置，wifi密码
    char auth[16];                               ///< 网络配置，wifi加密方式
    char mac[32];                                ///< 网络配置，网卡物理地址
    char name[32];                              ///< 网络配置，网络接口名称
    char bm[32];                                  ///< The mac address of car bt.
    int  action;                                   ///< 网络配置，取值参考 ECQrAction
};
typedef struct ECQRInfo ECQRInfo;

enum ECErrorStatusCode
{
    EC_ERROR_NO_ERROR = 0,
    EC_ERROR_TIME_OUT = -1
};
typedef enum ECErrorStatusCode ECErrorStatusCode;

struct ECDevice
{
    ECTransportType transportType;
    char serial[128];
    char phoneIp[64];
};
typedef struct ECDevice ECDevice;

enum ECTransportInfo
{
    EC_TRANSPORT_INFO_SPEECH_ENGINE = 1
};
typedef enum ECTransportInfo ECTransportInfo;

enum ECSpeechEngineType
{
    EC_SPEECH_ENGINE_TXZ = 1,						///<  TXZ speech engine
    EC_SPEECH_ENGINE_IFLYTEK = 2,					///<  IFLYTEK speech engine free version
    EC_SPEECH_ENGINE_IFLYTEK_ADVANCED = 2001,		///<  IFLYTEK speech engine advanced version
	EC_SPEECH_ENGINE_IFLYTEK_PRO = 2002,  			///<  IFLYTEK speech engine professional version
};
typedef enum ECSpeechEngineType ECSpeechEngineType;

enum ECWifiStateAction
{
	EC_WIFI_STATE_CHANGED_ACTION = 0,		 ///<AP, STATION
	EC_WIFI_P2P_STATE_CHANGED_ACTION = 1,    ///< P2P
};
typedef enum ECWifiStateAction ECWifiStateAction;

enum ECWifiState
{
	EC_WIFI_STATE_UNKNOWN = 0,
	EC_WIFI_STATE_ENABLE,              ///< The network is available
	EC_WIFI_STATE_DISABLE,			  ///< Network unavailable
	EC_WIFI_STATE_CONNECTED,		      ///< Network connected
	EC_WIFI_STATE_DISCONNECTED,        ///< Network disconnection
};
typedef enum ECWifiState ECWifiState;

struct ECNetWorkInfo
{
	int32_t		state;                 ///<see ECWifiState
	char		phoneIp[32];			   ///<phone ip
	char		carIp[32];				   ///<car ip
};
typedef struct ECNetWorkInfo ECNetWorkInfo;

enum ECBTConnectStatus{
    EC_BT_CONNECTSTATUS_UNKNOW,                 ///< Bluetooth connection status,Initialization status not available
    EC_BT_CONNECTSTATUS_DISCONNECT,             ///< Bluetooth connection status,Bluetooth disconnected
    EC_BT_CONNECTSTATUS_CONNECTED               ///< Bluetooth connection status,Bluetooth connected
};
typedef enum ECBTConnectStatus ECBTConnectStatus;

struct ECBTClientInfo{
    int version;
    int phoneType;
    char phoneID[64];
    char phoneName[64];
    char packageName[64];
};
typedef struct ECBTClientInfo ECBTClientInfo;

typedef struct ECQRInfo ECBTNetInfo;

enum ECBTBuildNetRlyStatus{
    EC_BT_REQUEST_BUILD_NET_USE_PHONE_AP  =     2,         ///< 允许连接，车机连接手机热点，手机app需要开启热点，并将ap信息告知车机端。
    EC_BT_REQUEST_BUILD_NET_NEED_PHONE_BUILD  = 1,         ///< 允许连接，手机需要根据返回的信息主动组网。
    EC_BT_REQUEST_BUILD_NET_SUCCEED         =   0,         ///< 允许连接，手机和车机已经组网成功，availableCarIp为车机ip地址。
    EC_BT_REQUEST_BUILD_NET_AUTH_PENDING   =   -1,         ///< 等待授权
    EC_BT_REQUEST_BUILD_NET_AUTH_FAIL      =   -2,         ///< 不允许连接
} ;
typedef enum ECBTBuildNetRlyStatus ECBTBuildNetRlyStatus;

struct ECBTRequestBuildNetRly{
    int status;                                            ///< 取值参考 ECBTBuildNetRlyStatus
    ECBTNetInfo netDeviceInfo;
};
typedef struct ECBTRequestBuildNetRly ECBTRequestBuildNetRly;

struct ECNetInterfaceInfo
{
    char name[32];                        ///< 网络接口名称:wlan0  p2p0
    char ip[32];                          ///< ipv4 地址
    char mask[32];                        ///< 子网掩码
};
typedef struct ECNetInterfaceInfo ECNetInterfaceInfo;

/**
*
* @enum ECGPSInfo
*
* @brief phone gps infomation
*/
struct ECGPSInfo {
    double altitude;
    double speed;
    double course;
    uint64_t time;
    double longitude;
    double latitude;
    char province[32];
    char city[64];
    char district[64];
    char road[64];
    char number[16];
    char poiname[256];
};
typedef struct ECGPSInfo ECGPSInfo;

/**
*
* @enum ECAPPHUDSupportFunction
*
* @brief APP HUD support function.
*
* @see enableDownloadPhoneAppHud
*/
enum ECAPPHUDSupportFunction {
    EC_APP_HUD_SUPPORT_FUNCTION_DEFAULT                = 0x0,              ///< no support function
    EC_APP_HUD_SUPPORT_FUNCTION_ROAD_JUNCTION_PICTURE  = 0x01,             ///< support show road junction picture.
    EC_APP_HUD_SUPPORT_FUNCTION_LANE_GUIDANCE_PICTURE  = 0x02              ///< support show lane guidance picture.
};
typedef enum ECAPPHUDSupportFunction ECAPPHUDSupportFunction;

/**
* @struct ECHudRoadJunctionPictureInfo
*
* @brief  road junction picture info
*
* @see   IECCallback::onPhoneAppHUDRoadJunctionPicture
*/
struct ECHudRoadJunctionPictureInfo {
    int8_t   status;         ///< picture status, 0 mean car hide the picture. 1 mean car show the picture.
    int32_t  format;        ///< picture format.
    char *   pictureData;    ///< picture data.
    uint32_t pictureLength;  ///< picture data length.
};
typedef struct ECHudRoadJunctionPictureInfo ECHudRoadJunctionPictureInfo;

struct ECPhoneNotification {
    int16_t appIconFormat;                      ///< 图标格式  @see enum ECIconFormat
    char    *appIconData;                       ///< 图标数据
    int32_t appIconLength;                      ///< 图标数据长度
    const char    *appName;                   ///< 发起通知程序名称
    const char    *title;         ///< 通知标题
    const char    *context;       ///< 通知内容
    const char    *dateTime;      ///< 通知发起的日期时间， format：dd.MM.yyyy HH:mm:ss.zzz
};
typedef struct ECPhoneNotification ECPhoneNotification;

/**
* @struct ECHudLaneGuidancePictureInfo
*
* @brief  lane guidance picture info
*
* @see   IECCallback::onPhoneAppHUDLaneGuidancePicture
*/
typedef ECHudRoadJunctionPictureInfo ECHudLaneGuidancePictureInfo;

#ifdef  __cplusplus
};
#endif

#endif
