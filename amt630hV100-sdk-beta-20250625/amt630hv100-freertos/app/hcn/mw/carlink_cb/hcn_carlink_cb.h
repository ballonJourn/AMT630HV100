/**
*
* @file hcn_carlink_cb.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/10 10:45
* @author och
*
*/

#ifndef __HCN_CARLINK_CB_H__
#define __HCN_CARLINK_CB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"

#define BT_PHONE_BOOK_MAX_NUM   (1000)  ///< 电话本最大下载数目
#define BT_SCAN_DEVICE_COUNT    (10)    ///< 蓝牙设备最大扫描数目
#define TEXT_PARAM_LEN      	(128)    ///< 字符串数据最大长度
#define BT_CONNECT_DEV_NAME_LEN (64)    ///< 蓝牙连接的设备名称长度

typedef enum {
    BT_BATTERY_CHANGE = 0,       ///< 手机电量
    BT_SIGNAL_CHANGE,            ///< 手机信号
    BT_CONNHCNTED_CHANGE,         ///< 手机连接状态
    BT_SWTICH_CHANGE,            ///< 手机蓝牙开关状态
    BT_BOOK_COUNT,               ///< 手机电话本下载个数
    BT_BOOK_STATE,               ///< 电话本下载状态
    BT_CALL_STATE,               ///< 手机通话状态
    BT_DEVICE_STATE,             ///< 设备状态
} bt_device_id_e;

typedef enum {
    BT_SCAN_DATA = 20,                ///< 耳机数据更新
} bt_scan_id_e;

typedef enum {
    BT_POWER_OFF = 0,
    BT_POWER_ON = 1,
    BT_BR_EDR_DISCOVER = 3,
    BT_BR_EDR_NOT_DISCOVER_BLE= 5,
    BT_BR_EDR_DISCOVER_BLE = 7,
    BT_SCANNING_BR_EDR = 13
} dev_state_e;

typedef enum {
    UNSUPPORTED = 0,
    STANDBY = 1,
    CONNECTING = 2,
    CONNECTED = 3,                      ///< 已连接
    OUTGOING_CALL = 4,                  ///< 去电
    INCOMING_CALL = 5,                  ///< 来电
    ACTIVE_CALL = 6,                    ///< 通话中
    ACTIVE_HELD = 7,                    ///< 3-way-calling
    FIRST_ACTIVE_SECOND_WAITING = 8,    ///< 3-way-calling
    FIRST_ACTIVE_SECOND_HELD = 9,       ///< 3-way-calling
    FIRST_OUTGOING_SECOND_HELD = 10,    ///< 3-way-calling
} hfp_state_e;

typedef enum {
    ///< 电话本状态
    PB_STATE_UNSUPPORTED = 0,
    PB_STATE_STANDBY,
    PB_STATE_CONNECTING,
    PB_STATE_CONNECTED,
    PB_STATE_DOWNDING
} pb_state_e;

typedef enum {
    STATE_UNSUPPORTED = 0,
    STATE_STANDBY,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_PAUSED,
    STATE_STREAMING
} bt_state_e;

/**
 * @brief 音乐播放状态枚举
 */
enum bt_music_play_state_e {
    BT_MUSIC_PLAY_STATE_STOPED,         ///< 停止状态
    BT_MUSIC_PLAY_STATE_PLAYING,        ///< 播放中
    BT_MUSIC_PLAY_STATE_PAUSED,         ///< 暂停状态
    BT_MUSIC_PLAY_STATE_FAST_FORWARDING, ///< 快进中
    BT_MUSIC_PLAY_STATE_FAST_REWINDING   ///< 快退中
};

typedef enum bt_music_play_state_e bt_music_play_state_e;

/**
 * @brief 音乐播放模式枚举
 */
typedef enum {
    BT_MUSIC_PLAY_MODE_OFF,          ///< 关闭重复
    BT_MUSIC_PLAY_MODE_SINGLE_TRACK,  ///< 单曲循环
    BT_MUSIC_PLAY_MODE_PAUSED,        ///< 暂停状态
    BT_MUSIC_PLAY_MODE_ALL_TRACKS,    ///< 全部循环
    BT_MUSIC_PLAY_MODE_GROUP          ///< 分组循环
} bt_music_play_mode_e;

/**
 * @brief 蓝牙音乐控制命令枚举
 */
typedef enum {
  BT_MUSIC_CMD_PLAYPAUSE,  ///< 播放/暂停
  BT_MUSIC_CMD_PLAY,       ///< 播放
  BT_MUSIC_CMD_PAUSE,      ///< 暂停
  BT_MUSIC_CMD_STOP,       ///< 停止
  BT_MUSIC_CMD_FORWARD,    ///< 下一曲
  BT_MUSIC_CMD_BACKWARD,   ///< 上一曲
  BT_MUSIC_CMD_REPEAT      ///< 重复模式
} bt_music_cmd_e;

typedef struct {
    bt_music_play_state_e cur_track_state; ///< 当前曲目状态
    uint16_t cur_time_music_play;          ///< 当前音乐播放的时间 （秒）
    uint16_t music_total_time;             ///< 当前音乐总时间   （秒）
} bt_music_play_info_t;

typedef struct {
    int img_index;          ///< 专辑封面图片序号，如 100086.jpg,为0代表无图片，每个专辑图片序号都不一样
    uint16_t img_height;     ///< 图片高度，默认是200像素
    uint16_t img_width;      ///< 图片宽度，默认是200像素
    int image_len;          ///< 图片长度
    char *image_buffer;     ///< 图片资源指针
} bt_music_song_art_cover_t;

/**
 * @brief 蓝牙音乐信息结构体
 */
typedef struct {
    bt_music_play_state_e play_state;            ///< 音乐播放状态
    bt_music_play_mode_e play_mode;              ///< 音乐播放模式
    bt_music_play_info_t music;                  ///< 音乐信息
    bt_music_song_art_cover_t song_art_cover;    ///< 专辑图片信息
    char lyrics[TEXT_PARAM_LEN];                ///< 歌曲歌词
    char artist[TEXT_PARAM_LEN];                ///< 艺术家/歌曲名
    char album[TEXT_PARAM_LEN];                 ///< 专辑名
} bt_music_info_t;

typedef struct {
    hfp_state_e btHfpState;    ///< 判断设备是否连接 >=3 代表已经连接
    char btCallNumber1[TEXT_PARAM_LEN];      ///< 通话号码
    char btCallPerson1[TEXT_PARAM_LEN];      ///< 通话联系人
    char btCallNumber2[TEXT_PARAM_LEN];      ///< 三方通话号码
    char btCallPerson2[TEXT_PARAM_LEN];      ///< 三方通话联系人
 } bt_call_t;

typedef struct {
    dev_state_e btDevState;    ///< 判断模块是否初始化完成 != 0
    uint8_t btPowerState;      ///< 电源状态 0：未上电 1:已上电
    uint8_t btA2dpState;       ///< A2DP状态
    uint8_t btPbState;           ///< 电话本状态
    uint8_t btSwitchState;       ///< 蓝牙的开关状态， 0为关闭，1为打开
    uint8_t btHfpAudio;
    uint8_t btConnected;            ///< 手机蓝牙连接状态 0为未连接  1为已连接
    uint8_t btHfpIBR;               ///< 手机是否支持来电铃声
    uint8_t btBatteryLevel;         ///< 手机电量 0-5级
    uint8_t btSignal;               ///< 手机信号 0-5级
    uint16_t btBookCount;           ///< 通讯录数量
    char btDevName[TEXT_PARAM_LEN]; ///< 蓝牙设备名称
    char btDevPin[TEXT_PARAM_LEN];  ///< 设备配对密码
    char btHfpAddr[TEXT_PARAM_LEN]; ///< 连接设备的蓝牙MAC地址，去除了:分割
    char btConnectDevName[BT_CONNECT_DEV_NAME_LEN]; ///< 蓝牙连接设备的名称,
} bt_data_t;

typedef struct {
    //dev_state_e btDevState;        ///< 判断模块是否初始化完成 != 0
    int btDevState;
    hfp_state_e btHfpState1;       ///< 耳机1的HFP状态 >=3 代表已经连接
    hfp_state_e btHfpState2;       ///< 耳机2的HFP状态 >=3 代表已经连接
    uint8_t btA2dpState1;   			///< 耳机1的A2DP状态 >=3 代表已经连接
    uint8_t btA2dpState2;   			///< 耳机2的A2DP状态 >=3 代表已经连接
    uint8_t btHfpAudio;
    uint8_t scanState;          		///< 0：未扫描  1：扫描中 2：扫描结束
    uint8_t devicHCNount;        		///< 扫描到的设备数量 
    char btScanName[BT_SCAN_DEVICE_COUNT][TEXT_PARAM_LEN];
    char btScanAddr[BT_SCAN_DEVICE_COUNT][TEXT_PARAM_LEN];
} bt_scan_data_t;

typedef struct {
    char name[TEXT_PARAM_LEN];  ///< 电话本名字最长字符为30
    char number[TEXT_PARAM_LEN];  ///< 电话本号码最长字符为30
} bt_phone_book_t;

typedef enum {
    WIFI_USER_NULL= 0x00,
    WIFI_USER_OTA,
    WIFI_USER_HCN,
    WIFI_USER_EY,
    WIFI_USER_TEST
} wifi_user_e;

typedef struct {
    char weather[32];
    int weatherIcon;
    int temperature;
} weather_data_t;

typedef struct {
    int16_t appIconFormat;                    ///< 图标格式  @see enum HCNIconFormat
    char    *appIconData;                     ///< 图标数据
    int32_t  appIconLength;                   ///< 图标数据长度
    const char    *appName;                   ///< 发起通知程序名称
    const char    *title;                     ///< 通知标题
    const char    *context;                   ///< 通知内容
    const char    *dateTime;                  ///< 通知发起的日期时间， format：dd.MM.yyyy HH:mm:ss.zzz
    uint8_t   phoneType;                      ///< 手机类型  0：Android 1:ios
} phone_notification_t;

typedef struct {
    int8_t   status;                         ///< picture status, 0 mean car hide the picture. 1 mean car show the picture.
    int32_t  format;                         ///< picture format.
    char *   pictureData;                    ///< picture data.
    uint32_t pictureLength;                  ///< picture data length.
} road_junction_pic_t;

typedef enum {
	HCN_NAVI_STATUS_ACTIVE = 0,
	HCN_NAVI_STATUS_INACTIVE,
	HCN_NAVI_STATUS_MAX
} hcn_navi_status;

typedef enum {
    HCN_NAVI_ICON_NONE                          = 0,               ///< 收到此值，不显示导航图标
    HCN_NAVI_ICON_DEFAULT                       = 1,               ///< 自车.请忽略这个元素，从左转图标开始
    HCN_NAVI_ICON_LEFT                          = 2,               ///< 左转
    HCN_NAVI_ICON_RIGHT                         = 3,               ///< 右转
    HCN_NAVI_ICON_LEFT_FRONT                    = 4,               ///< 左前方
    HCN_NAVI_ICON_RIGHT_FRONT                   = 5,               ///< 右前方
    HCN_NAVI_ICON_LEFT_BACK                     = 6,               ///< 左后方
    HCN_NAVI_ICON_RIGHT_BACK                    = 7,               ///< 右后方
    HCN_NAVI_ICON_LEFT_TURN_AROUND              = 8,               ///< 左转掉头
    HCN_NAVI_ICON_STRAIGHT                      = 9,               ///< 直行
    HCN_NAVI_ICON_ARRIVED_WAYPOINT              = 10,              ///< 到达途经点
    HCN_NAVI_ICON_ENTER_ROUNDABOUT              = 11,              ///< 进入环岛
    HCN_NAVI_ICON_OUT_ROUNDABOUT                = 12,              ///< 驶出环岛
    HCN_NAVI_ICON_ARRIVED_SERVICE_AREA          = 13,              ///< 到达服务区
    HCN_NAVI_ICON_ARRIVED_TOLLGATE              = 14,              ///< 到达收费站
    HCN_NAVI_ICON_ARRIVED_DESTINATION           = 15,              ///< 到达目的地
    HCN_NAVI_ICON_ARRIVED_TUNNEL                = 16,              ///< 到达隧道
    HCN_NAVI_ICON_CROSSWALK                     = 17,              ///< 通过人行横道
    HCN_NAVI_ICON_OVERPASS                      = 18,              ///< 通过过街天桥
    HCN_NAVI_ICON_UNDERPASS                     = 19,              ///< 通过地下通道
    HCN_NAVI_ICON_SQUARE                        = 20,              ///< 通过广场
    HCN_NAVI_ICON_PARK                          = 21,              ///< 通过公园
    HCN_NAVI_ICON_STAIRCASE                     = 22,              ///< 通过扶梯
    HCN_NAVI_ICON_LIFT                          = 23,              ///< 通过直梯
    HCN_NAVI_ICON_CABLEWAY                      = 24,              ///< 通过索道
    HCN_NAVI_ICON_SKY_CHANNEL                   = 25,              ///< 通过空中通道
    HCN_NAVI_ICON_CHANNEL                       = 26,              ///< 通过通道、建筑物穿越通道
    HCN_NAVI_ICON_WALK_ROAD                     = 27,              ///< 通过行人道路
    HCN_NAVI_ICON_CRUISE_ROUTE                  = 28,              ///< 通过游船路线
    HCN_NAVI_ICON_SIGHTSEEING_BUSLINE           = 29,              ///< 通过观光车路线
    HCN_NAVI_ICON_SLIDEWAY                      = 30,              ///< 通过滑道
    HCN_NAVI_ICON_LADDER                        = 31,              ///< 通过阶梯
    HCN_NAVI_ICON_MERGE_LEFT                    = 51,              ///< 靠左行驶
    HCN_NAVI_ICON_MERGE_RIGHT                   = 52,              ///< 靠右行驶
    HCN_NAVI_ICON_SLOW                          = 53,              ///< 减速慢行
    HCN_NAVI_ICON_ENTRY_RING_LEFT               = 54,              ///< 标准小环岛 绕环岛左转，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ENTRY_RING_RIGHT              = 55,              ///< 标准小环岛 绕环岛右转，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ENTRY_RING_CONTINUE           = 56,              ///< 标准小环岛 绕环岛直行，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ENTRY_RING_UTURN              = 57,              ///< 标准小环岛 绕环岛调头，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ENTRY_LEFT_RING               = 58,              ///< 进入环岛图标，左侧通行地区的顺时针环岛
    HCN_NAVI_ICON_LEAVE_LEFT_RING               = 59,              ///< 驶出环岛图标，左侧通行地区的顺时针环岛
    HCN_NAVI_ICON_UTURN_RIGHT                   = 60,              ///< 右转掉头图标，左侧通行地区的掉头
    HCN_NAVI_ICON_SPHCNIAL_CONTINUE              = 61,              ///< 顺行图标(和直行有区别，顺行图标带有虚线)
    HCN_NAVI_ICON_ENTRY_LEFT_RING_LEFT          = 62,              ///< 标准小环岛 绕环岛左转，左侧通行地区的顺时针环岛
    HCN_NAVI_ICON_ENTRY_LEFT_RING_RIGHT         = 63,              ///< 标准小环岛 绕环岛右转，左侧通行地区的顺时针环岛
    HCN_NAVI_ICON_ENTRY_LEFT_RING_CONTINUE      = 64,              ///< 标准小环岛 绕环岛直行，左侧通行地区的顺时针环岛
    HCN_NAVI_ICON_ENTRY_LEFT_RING_UTURN         = 65,              ///< 标准小环岛 绕环岛调头，左侧通行地区的顺时针环岛
    HCN_NAVI_ICON_SLOPE                         = 66,              ///< 通过斜坡图标
    HCN_NAVI_ICON_BRIDGE                        = 67,              ///< 通过桥图标
    HCN_NAVI_ICON_FERRYBOAT                     = 68,              ///< 通过渡轮图标
    HCN_NAVI_ICON_SUBWAY                        = 69,              ///< 通过地铁图标
    HCN_NAVI_ICON_ENTER_BUILDING                = 70,              ///< 进入建筑物图标
    HCN_NAVI_ICON_LEAVE_BUILDING                = 71,              ///< 离开建筑物图标
    HCN_NAVI_ICON_BY_ELEVATOR                   = 72,              ///< 电梯换层图标
    HCN_NAVI_ICON_BY_STAIR                      = 73,              ///< 楼梯换层图标
    HCN_NAVI_ICON_ESCALATOR                     = 74,              ///< 扶梯换层图标
    HCN_NAVI_ICON_LOW_TRAFFIC_CROSS             = 75,              ///< 非导航段通过红绿灯路口图标
    HCN_NAVI_ICON_LOW_CROSS                     = 76,              ///< 非导航段通过普通路口图标
    HCN_NAVI_ICON_ROTARY_SHARP_LEFT             = 77,              ///< 环岛左后转，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ROTARY_SHARP_RIGHT            = 78,              ///< 环岛后右转，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ROTARY_SLIGHT_LEFT            = 79,              ///< 环岛左前转，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_ROTARY_SLIGHT_RIGHT           = 80,              ///< 环岛右前转，右侧通行地区的逆时针环岛
    HCN_NAVI_ICON_TURN_BRANCH_LEFT              = 81,              ///< 左前方转弯
    HCN_NAVI_ICON_TURN_LEFT_3BRANCH_LEFT        = 82,              ///< 普通三分歧/JCT/SAPA 靠最左
    HCN_NAVI_ICON_TURN_LF_3BRANCH_LEFT          = 83,              ///< 左转，驶入最左侧道路
    HCN_NAVI_ICON_TURN_RIGHT_3BRANCH_LEFT       = 84,              ///< 向左前方行驶，进入最左侧道路
    HCN_NAVI_ICON_TURN_RF_3BRANCH_LEFT          = 85,              ///< 向右前方行驶，进入最左侧道路
    HCN_NAVI_ICON_TURN_LEFT_2BRANCH_RIGHT       = 86,              ///< 左转，驶入右侧道路
    HCN_NAVI_ICON_TURN_LEFT_3BRANCH_RIGHT       = 87,              ///< 左转，驶入最右侧道路
    HCN_NAVI_ICON_TURN_BRANCH_RIGHT             = 88,              ///< 普通三分歧/JCT/SAPA 靠最右
    HCN_NAVI_ICON_TURN_RIGHT_3BRANCH_RIGHT      = 89,              ///< 右转，驶入最右侧道路
    HCN_NAVI_ICON_TURN_LF_3BRANCH_RIGHT         = 90,              ///< 左前方复杂八方向三分歧 右侧道路
    HCN_NAVI_ICON_TURN_RF_3BRANCH_RIGHT         = 91,              ///< 向右前方行驶，进入最右侧道路
    HCN_NAVI_ICON_TURN_LB_2BRANCH_LEFT          = 92,              ///< 左后方复杂八方向二分歧 左侧
    HCN_NAVI_ICON_TURN_LB_2BRANCH_RIGHT         = 93,              ///< 左后方复杂八方向二分歧 右侧
    HCN_NAVI_ICON_TURN_LB_3BRANCH_LEFT          = 94,              ///< 左后方复杂八方向三分歧 左侧
    HCN_NAVI_ICON_TURN_LB_3BRANCH_RIGHT         = 95,              ///< 左后方复杂八方向三分歧 右侧
    HCN_NAVI_ICON_TURN_LB_3BRANCH_MIDDLE        = 96,              ///< 左后方复杂八方向三分歧 中间
    HCN_NAVI_ICON_TURN_LB_NOT_BACK              = 97,              ///< 向左后方行驶，注意不是掉头
    HCN_NAVI_ICON_TURN_RB_2BRANCH_LEFT          = 98,              ///< 向右后方行驶，进入左侧道路
    HCN_NAVI_ICON_TURN_RB_2BRANCH_RIGHT         = 99,              ///< 向右后方行驶，进入右侧道路
    HCN_NAVI_ICON_TURN_RB_3BRANCH_LEFT          = 100,             ///< 向右后方行驶，进入最左侧道路
    HCN_NAVI_ICON_TURN_RB_3BRANCH_RIGHT         = 101,             ///< 向右后方行驶，进入最右侧道路
    HCN_NAVI_ICON_TURN_RB_3BRANCH_MIDDLE        = 102,             ///< 向右后方行驶，进入中间道路
    HCN_NAVI_ICON_TURN_RB_NOT_BACK              = 103,             ///< 向右后方行驶，进入左侧道路
    HCN_NAVI_ICON_TURN_BACK_2BRANCH_LEFT        = 104,             ///< 八方向掉头+随后靠左
    HCN_NAVI_ICON_TURN_BACK_3BRANCH_LEFT        = 105,             ///< 八方向掉头+随后靠最左
    HCN_NAVI_ICON_TURN_BACK_3BRANCH_RIGHT       = 106,             ///< 八方向掉头+随后靠最右
    HCN_NAVI_ICON_TURN_BACK_3BRANCH_MIDDLE      = 107,             ///< 八方向掉头+随后沿中间
    HCN_NAVI_ICON_TURN_BACK_2BRANCH_RIGHT       = 108,             ///< 八方向掉头+随后靠右
    HCN_NAVI_ICON_TURN_LEFT_SIDE_MAIN           = 109,             ///< 左侧走本线
    HCN_NAVI_ICON_TURN_BRANCH_LEFT_STRAIGHT     = 110,             ///< 靠最左走本线
    HCN_NAVI_ICON_TURN_RIGHT_SIDE_MAIN          = 111,             ///< 右侧走本线
    HCN_NAVI_ICON_TURN_BRANCH_RIGHT_STRAIGHT    = 112,             ///< 靠最右走本线
    HCN_NAVI_ICON_TURN_BRANCH_CENTER            = 113,             ///< 中间走本线
    HCN_NAVI_ICON_TURN_LEFT_SIDE_IC             = 114,             ///< IC二分歧左侧走IC
    HCN_NAVI_ICON_TURN_LF_2BRANCH_LEFT          = 115,             ///< 八方向左前方靠左侧
    HCN_NAVI_ICON_TURN_LEFT_2BRANCH_LEFT        = 116,             ///< 左转，驶入左侧道路
    HCN_NAVI_ICON_TURN_RIGHT_2BRANCH_LEFT       = 117,             ///< 右转，驶入左侧道路
    HCN_NAVI_ICON_TURN_RF_2BRANCH_LEFT          = 118,             ///< 八方向右前方靠左侧
    HCN_NAVI_ICON_TURN_RF_NOT_RIGHT             = 119,             ///< 向右前方行驶，注意不是右转
    HCN_NAVI_ICON_TURN_RIGHT_SIDE_IC            = 120,             ///< IC二分歧右侧走IC
    HCN_NAVI_ICON_TURN_LF_2BRANCH_RIGHT         = 121,             ///< 八方向左前方靠右侧
    HCN_NAVI_ICON_TURN_LF_NOT_LEFT              = 122,             ///< 左前，注意不是左转
    HCN_NAVI_ICON_TURN_RIGHT_2BRANCH_RIGHT      = 123,             ///< 右转，驶入右侧道路
    HCN_NAVI_ICON_TURN_RF_2BRANCH_RIGHT         = 124,             ///< 八方向右前方靠右侧
    HCN_NAVI_ICON_TURN_NEAR_RIGHT_FRONT         = 125,             ///< 近距离第二路口 右转
    HCN_NAVI_ICON_TURN_LEFT_3BRANCH_MIDDLE      = 126,             ///< 左转，驶入中间道路
    HCN_NAVI_ICON_TURN_RIGHT_3BRANCH_MIDDLE     = 127,             ///< 右转，驶入中间道路
    HCN_NAVI_ICON_TURN_RF_3BRANCH_MIDDLE        = 128,             ///< 向右前方行驶，进入中间道路
    HCN_NAVI_ICON_TURN_LF_3BRANCH_MIDDLE        = 129,             ///< 向左前方行驶，进入中间道路
    HCN_NAVI_ICON_LEFT_PASSROAD_FRONT           = 130,             ///< 向左到路⼝斜对⾯，继续向前
    HCN_NAVI_ICON_RIGHT_PASSROAD_FRONT          = 131,             ///< 向右到路⼝斜对⾯，继续向前
    HCN_NAVI_ICON_LEFT_PASSROAD_UTURN           = 132,             ///< 左转穿过⻢路(步⾏设施名称)往回⾛
    HCN_NAVI_ICON_RIGHT_PASSROAD_UTURN          = 133,             ///< 右转穿过⻢路(步⾏设施名称)往回⾛
    HCN_NAVI_ICON_LEFTDIAGONAL_PASSROAD_RIGHT   = 134,             ///< 向左到路⼝斜对⾯，向右转弯
    HCN_NAVI_ICON_LEFTDIAGONAL_PASSROAD_RIGHT_FRONT = 135,         ///< 向右到路⼝斜对⾯，向右前⽅转弯
    HCN_NAVI_ICON_RIGHTDIAGONAL_PASSROAD_LEFT_FRONT = 136,         ///< 向右到路⼝斜对⾯，向左前⽅转弯
    HCN_NAVI_ICON_LEFTDIAGONAL_PASSROAD_LEFT    = 137,             ///< 向左到路⼝斜对⾯，向左转弯
    HCN_NAVI_ICON_LEFTDIAGONAL_PASSROAD_LEFT_BACK   = 138,         ///< 向左到路⼝斜对⾯，向左后⽅转弯
    HCN_NAVI_ICON_RIGHTDIAGONAL_PASSROAD_LEFT   = 139,             ///< 向右到路⼝斜对⾯，向左转弯
    HCN_NAVI_ICON_RIGHTDIAGONAL_PASSROAD_RIGHT  = 140,             ///< 向右到路⼝斜对⾯，向右转弯
    HCN_NAVI_ICON_RIGHTDIAGONAL_PASSROAD_RIGHT_BACK = 141,         ///< 向右到路⼝斜对⾯，向右后⽅转弯
    HCN_NAVI_ICON_PASSROAD_LEFT                 = 142,             ///< 过⻢路左转
    HCN_NAVI_ICON_PASSROAD_RIGHT                = 143,             ///< 过⻢路右转
    HCN_NAVI_ICON_GOTO_LEFT_ROAD                = 144,             ///< 进⼊左侧道路继续向前
    HCN_NAVI_ICON_GOTO_RIGHT_ROAD               = 145,             ///< 进⼊右侧道路继续向前
    HCN_NAVI_ICON_GOTO_LEFT_ROAD_UTURN          = 146,             ///< 进⼊左侧道路往回⾛
    HCN_NAVI_ICON_GOTO_RIGHT_ROAD_UTURN         = 147,             ///< 进⼊右侧道路往回⾛
    HCN_NAVI_ICON_FARAWAY_ROUTE                 = 148,             ///< 偏离路线
    HCN_NAVI_ICON_GPS_WEAK                      = 149,             ///< 卫星信号
    HCN_NAVI_ICON_REROUTE                       = 150,             ///< 刷新路线
    HCN_NAVI_ICON_TURN_LEFT_DIAGONAL_PASSROAD_FRONT  = 151,        ///< 向左到路口斜对面继续向前
    HCN_NAVI_ICON_TURN_RIGHT_DIAGONAL_PASSROAD_FRONT = 152,        ///< 向右到路口斜对面继续向前
    HCN_NAVI_ICON_MAX
} hcnNaviIcon;

typedef enum {
    HCN_GPS_SIGNAL_DEFAULT                     = 0,               ///< GPS信号初始值
    HCN_GPS_SIGNAL_WEAK                        = 1,               ///< GPS信号弱
    HCN_GPS_SIGNAL_STRONG                      = 2                ///< GPS信号强
 } hcnGpsSignalIntensity;

#define NAVIGA_ROAD_NAME_LEN    (128)

typedef struct {
    hcn_navi_status status;
    hcnNaviIcon naviIcon;
    char currentRoad[NAVIGA_ROAD_NAME_LEN];
    char nextRoad[NAVIGA_ROAD_NAME_LEN];
    int32_t roadRemainingDistance;
    int32_t destinationRemainingDistance;    
    int32_t signalIntensity;                 //GPS信号强弱
} hcnNavigationHudInfo;

typedef struct {
    uint8_t phoneType;             //手机类型      0:安卓  1:苹果
    char phoneModels[128];          //手机型号  
    char phoneDevName[128];        //手机设备名称 ,iphone无法获取设备名称
} hcnPhoneInfo;

typedef struct  {
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
 } hcnGpSInfo;

typedef struct {
	/**
	 * @brief  Called when the mirror status change.
	 * @param  status  1:mirror on  0:mirror close
	 */
	void (*onHcnVideoStatus)(bool status);

	/**
	 * @brief  Called when the license authorization
	 * @param  status  1:AuthSuccess 0:failed
	 */
	void (*onHcnLicenseStatus)(bool status);

	void (*onHcnLinkConnect)(void);

	void (*onHcnBtChange)(bt_device_id_e id, uint32_t value);

    void (*onHcnWeatherReceived)(const char *weather_json);

    void (*onHcnPhoneNotification)(const phone_notification_t *notification);

    void (*onHcnPhoneAppHUDLaneGuidancePicture)(const road_junction_pic_t * picData);

    void (*onHcnPhoneAppHUDRoadJunctionPicture)(const road_junction_pic_t* picData);

    void (*onHcnEasyNavigation)(const hcnNavigationHudInfo * naviData);    

    void (*onHcnPhoneModel)(const char * phoneInfo);
} IhcnCallBack;

typedef struct {
    int xPos;   ///< 视频显示的X坐标
    int yPos;   ///< 视频显示的Y坐标
    int width;  ///< 视频显示的宽度
    int height; ///< 视频显示的高度

    int safeXpos;               ///< 非遮挡区域x坐标(异形区域时使用)
    int safeYpos;               ///< 非遮挡区域y坐标(异形区域时使用)
    int safeWidth;              ///< 非遮挡区域宽度(异形区域时使用)
    int safeHeight;             ///< 非遮挡区域高度(异形区域时使用)
    bool bt_debug_mode;         ///< turn blueware debug msg on/off
    bool ec_debug_mode;         ///< turn carlink debug msg on/off
    bool support_ota;           ///< 是否打开OTA功能    
    bool support_ancs;          ///< 是否支持ANCS，用于IOS系统第三方消息推送
    bool support_notification;  ///< 是否支持安卓互联以后第三方消息推送
    bool support_safeArea;      ///< 是否支持异形区域
    bool auto_set_time;         ///< 时间设置是否有自动模式，区别于手动模式
    bool support_weather;       ///< 是否支持天气
    bool support_altitude;      ///< 是否支持海拔
    char* soc_version;          ///< SOC版本号，用于OTA升级
    char* customer_name;        ///< 车厂名称缩写，蓝牙名称使用这个头加后缀 如凯越:KY;钱江:QJ
} HcnLibConfig;

///< 亿联
int32_t hcn_ec_loadNightModeStatus(uint32_t isNightModeOn); ///< 切换亿联的白天黑夜模式
int32_t hcn_ec_startMirror();                       ///< 开始镜像
void hcn_ec_stopMirror();                           ///< 停止镜像
const char* hcn_ec_get_version();                   ///< 获取亿联SDK版本
const char* hcn_ec_get_qr_code_url();               ///< 获取亿联连接的二维码
const char* hcn_ec_get_uuid();	                    ///< 获取UUID

///< 蓝牙api
void hcn_bt_switch_state(bool on);     				///< 打开或关闭手机蓝牙  
void hcn_bt_download_book();        				///< 手机蓝牙模块下载电话本
void hcn_bt_pick_up();                  			///< 接听
void hcn_bt_hung_up();                  			///< 挂断
bool hcn_bt_is_Call();                   		    ///< 当前是否在通话

const bt_call_t* hcn_bt_get_call();           ///< 获取通话数据
const bt_data_t* hcn_bt_get_data();           ///< 获取数据
const char *hcn_get_bt_version(void);         ///< 获取蓝牙库版本
const char* hcn_bt_get_name();                 ///< 获取蓝牙名称
const char* hcn_bt_get_mac_addr();
const char* hcn_bt_get_ble_name();
const char* hcn_bt_get_ble_mac_addr();

///< 蓝牙音乐
const bt_music_info_t* hcn_bt_get_music_data();   ///< 获取蓝牙音乐相关信息
void hcn_send_music_cmd(bt_music_cmd_e cmd);

void carlink_cb_init(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CARLINK_CB_H__