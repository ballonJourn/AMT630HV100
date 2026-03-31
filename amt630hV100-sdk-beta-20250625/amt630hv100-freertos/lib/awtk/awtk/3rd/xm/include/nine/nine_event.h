#ifndef NINE_EVENT_H
#define NINE_EVENT_H
#include "base/events.h"

#define EVT_NINE_EVT (EVT_USER_START + 1)

BEGIN_C_DECLS

typedef enum _nine_event_type_t {
   NINE_EVENT_NONE = 0,
  /**
   * @const NINE_EVENT_VCU_IDLE
   * 待机(无参数）。
   */
  NINE_EVENT_VCU_IDLE,  

  /**
   * @const NINE_EVENT_VCU_PHONE_CONNECTED
   * 手机已连接(无参数）。
   */
  NINE_EVENT_VCU_PHONE_CONNECTED,  

  /**
   * @const NINE_EVENT_VCU_NOT_READY
   * 未激活。（int32_t state）
   */
  NINE_EVENT_VCU_NOT_READY, 
  /**
   * @const NINE_EVENT_VCU_READY
   * 激活。（无参数）
   */
  NINE_EVENT_VCU_READY,
  /**
   * @const NINE_EVENT_VCU_CYCLING_MODE
   * 骑行模式 (int32_t mode)。
   */
  NINE_EVENT_VCU_CYCLING_MODE, 

  /**
   * @const NINE_EVENT_VCU_UNLOCK_MODE
   * 解除。（无参数）
   */
  NINE_EVENT_VCU_UNLOCK_MODE, 

  /**
   * @const NINE_EVENT_VCU_RUNNING_STATUS
   * 运行中状态  (输出功率、速度、转向角）
   */
  NINE_EVENT_VCU_RUNNING_STATUS,
 
  /**
   * @const NINE_EVENT_VCU_GEAR
   * 档位 (正整数)
   */
  NINE_EVENT_VCU_GEAR,

  
  /**
   * @const NINE_EVENT_BLE_UPDATE_SYSTEM
   * 蓝牙升级事件（无参数)
   */
  NINE_EVENT_VCU_BLE_UPDATE_SYSTEM,
  /**
   * @const NINE_EVENT_WIFI_CONNECT,
   * WIFI连接事件(int32_t，由WiFi驱动层给出)
   */
  NINE_EVENT_WIFI_CONNECT,
  /**
   * @const NINE_EVENT_UPDATE_PROCESS,
   * 升级进度指示 (float_t step)
   */
  NINE_EVENT_UPDATE_PROCESS,
  /**
   * @const NINE_EVENT_UPDATE_FINISH,
   * 升级完成 (0 失败， 1成功)
   */
  NINE_EVENT_UPDATE_FINISH,

  
  /**
   * @const NINE_EVENT_BASIC_ERROR_DETAILS
   * 基本错误详情信息 (int32_t code)
   */
  NINE_EVENT_BASIC_ERROR_DETAILS,
  /**
   * @const NINE_EVENT_BASIC_ERROR_CLOSE
   * 基本错误详情收起 (无参数)
   */
  NINE_EVENT_BASIC_ERROR_CLOSE,

  /**
   * @brief NINE_EVENT_WEATHER_DETAILS,
   * 天气预警详情信息
   */
  NINE_EVENT_WEATHER_DETAILS,
  /**
   * @brief NINE_EVENT_WEATHER_CLOSE,
   * 天气预警详情收起(无参数)
   */
  NINE_EVENT_WEATHER_CLOSE,

  /**
   * @brief NINE_EVENT_LOW_BATTERY,
   * 低电量报警 (int32_t votage[3])
   */
  NINE_EVENT_LOW_BATTERY,
  /**
   * @brief NINE_EVENT_KEY_LONG_AUTO,
   * 长按AUTO键 (int32_t locked_time, 剩余密码解锁锁定时间)
   */
  NINE_EVENT_KEY_LONG_AUTO,

  /**
   * @brief NINE_EVENT_KEY_AUTO,
   * AUTO键
   */
  NINE_EVENT_KEY_AUTO,
  /**
   * @brief NINE_EVENT_KEY_TRIGGER,
   * 扳机键
   */
  NINE_EVENT_KEY_TRIGGER,
  /**
   * @brief NINE_EVENT_KEY_WHEEL,
   * 滚轮键
   */
  NINE_EVENT_KEY_WHEEL,
  /**
   * @brief NINE_EVENT_KEY_BRAKE,
   * 刹车键
   */
  NINE_EVENT_KEY_BREAK,
  /**
   * @brief NINE_EVENT_KEY_ADD,
   * 加法键
   */
  NINE_EVENT_KEY_ADD,
  /**
   * @brief NINE_EVENT_KEY_SUB,
   * 减法键
   */
  NINE_EVENT_KEY_SUB,
  /**
   * @brief NINE_EVENT_KEY_USER,
   * 自定义键
   */
  NINE_EVENT_KEY_USER, 
  /**
   * @brief NINE_EVENT_POWER_DISSIPATION,
   * 功耗数据事件
   */
  NINE_EVENT_POWER_DISSIPATION,
  /**
   * @brief NINE_EVENT_POWER_DISSIPATION_REALTIME,
   * 功率曲线的实时功耗
   */
  NINE_EVENT_POWER_DISSIPATION_REALTIME,

   /**
   * @brief NINE_EVENT_TRAFFIC_DATA,
   * 行驶数据
   */
  NINE_EVENT_TRAFFIC_DATA, 
  
  /**
   * @brief NINE_EVENT_NAVIGATION,
   * 导航事件
   */
  NINE_EVENT_NAVIGATION,

  /**
   * @brief NINE_EVENT_HIGH_BEAM,
   * 远光灯 
   */
  NINE_EVENT_HIGH_BEAM,

  /**
   * @brief NINE_EVENT_LOW_BEAM,
   * 近光灯 
   */
  NINE_EVENT_LOW_BEAM,


  /**
   * @brief NINE_EVENT_LEFT_TURN,
   * 左转向灯 
   */
  NINE_EVENT_LEFT_TURN,

  /**
   * @brief NINE_EVENT_RIGHT_TURN,
   * 右转向灯 
   */
  NINE_EVENT_RIGHT_TURN,

  /**
   * @brief NINE_EVENT_ABS,
   * ABS 
   */
  NINE_EVENT_ABS,

  /**
   * @brief NINE_EVENT_CRUISE_CONTROL,
   * 定速巡航 
   */
  NINE_EVENT_CRUISE_CONTROL,  

  /**
   * @brief NINE_EVENT_READY,
   * READY提示 
   */
  NINE_EVENT_READY_HINT,   

  /**
   * @brief NINE_EVENT_GSM,
   * GSM连接强度 4种
   */
  NINE_EVENT_GSM,   
 
  /**
   * @brief NINE_EVENT_GPS,
   * GPS强度 3种
   */
  NINE_EVENT_GPS,   
  /**
   * @brief NINE_EVENT_BLE1,
   * BLE1 接近解锁蓝牙
   */
  NINE_EVENT_BLE1,
  /**
   * @brief NINE_EVENT_BLE2,
   * BLE2 
   */
  NINE_EVENT_BLE2,  
  /**
   * @brief NINE_EVENT_DATETIME,
   * 系统时间 
   */
  NINE_EVENT_DATETIME,
  
  NINE_EVENT_ODO,
  
  NINE_EVENT_TRIP,
  
  /**
   * @brief NINE_EVENT_LIGHT_SWITCH,
   * 光敏日夜切换 
   */
  NINE_EVENT_LIGHT_SWITCH,

  /**
   * @brief NINE_EVENT_THEME_SWITCH,
   * 主题切换 
   */
  NINE_EVENT_THEME_SWITCH,

  /**
   * @brief NINE_EVENT_PASSWORD,
   * 密码校验结果 (int32_t)
   */
  NINE_EVENT_PASSWORD,

  /**
   * @brief NINE_EVENT_PAGE_SWITCH,
   * 页面切换 (int32_t)
   */
  NINE_EVENT_PAGE_SWITCH,

  /**
   * @brief NINE_EVENT_REMAINING_MILEAGE,
   * 电池剩余里程 (int32_t)
   */
  NINE_EVENT_REMAINING_MILEAGE,

  /**
    * @brief NINE_EVENT_CHARGE_TIME,
    * 充电剩余时间 (int32_t)
  */
  NINE_EVENT_CHARGE_TIME,


} nine_event_type_t;

#define NINE_VCU_NOT_READY_STATE_PACK_UP_TEMPLE   1 // 收起边撑后骑行
#define NINE_VCU_NOT_READY_STATE_SIT_SIT_BARRELS  2 // 坐上坐桶后骑行
#define NINE_VCU_NOT_READY_STATE_PACK_UP_TEMPLE_AND_SIT_SIT_BARRELS 3 // 收起边撑并坐上坐桶后骑行
/**
 * @class nine_vcu_not_ready_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * VCU未激活事件
 */
typedef struct _nine_vcu_not_ready_event_t {
  /**
   * @property {int32_t} mode
   * @annotation ["readable", "scriptable"]
   * VCU未激活事件。
   */
  int32_t state;
} nine_vcu_not_ready_event_t;

// 骑行模式
#define NINE_VCU_CYCLING_MODE_NONE        0 // 非骑行模式
#define NINE_VCU_CYCLING_MODE_HELP_MOVE   1 // 助力推行
#define NINE_VCU_CYCLING_MODE_BACK_CAR    2 // 倒车中
/**
 * @class nine_vcu_cycling_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 骑行模式
 */
typedef struct _nine_vcu_cycling_event_t {
 /**
   * @property {int32_t} mode
   * @annotation ["readable", "scriptable"]
   * 骑行模式。
   */
  int32_t mode;
} nine_vcu_cycling_event_t;

// 运行状态事件（输出功率，速度，转向角）
/**
 * @class nine_vcu_running_status_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 运行状态事件
 */
typedef struct _nine_vcu_running_status_event_t {
   /**
   * @property {float_t} power_output
   * @annotation ["readable", "scriptable"]
   * 功率输出值。
   */
  float_t power_output;
  /**
   * @property {float_t} speed
   * @annotation ["readable", "scriptable"]
   * 速度。
   */
  float_t speed;
  /**
   * @property {float_t} steering_angle
   * @annotation ["readable", "scriptable"]
   * 转向角
   */
  float_t steering_angle;
} nine_vcu_running_status_event_t;

#define NINE_VCU_GEAR_ASSIT     0
#define NINE_VCU_GEAR_ECO       1   
#define NINE_VCU_GEAR_COAST     2   
#define NINE_VCU_GEAR_FURIOUS   3   
// 档位事件
/**
 * @class nine_vcu_gear_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 档位
 */
typedef struct _nine_vcu_gear_event_t {
  /**
   * @property {int32_t} gear
   * @annotation ["readable", "scriptable"]
   * 档位。
   */
  float_t gear;
} nine_vcu_gear_event_t;

#define NINE_WIFI_STATE_DISCONNECT 0
#define NINE_WIFI_STATE_CONNECTING 1
#define NINE_WIFI_STATE_CONNECTED 2
// WiFi连接事件
/**
 * @class nine_wifi_connect_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * WiFi事件
 */
typedef struct _nine_wifi_connect_event_t {
  /**
   * @property {int32_t} state
   * @annotation ["readable", "scriptable"]
   * 连接状态。
   */
  int32_t state;
} nine_wifi_connect_event_t;

// 系统升级进度事件
/**
 * @class nine_update_progress_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 升级进度事件
 */
typedef struct _nine_update_progress_event_t {
  /**
   * @property {float_t} step
   * @annotation ["readable", "scriptable"]
   * 升级进度, 0.0f ~ 1.0f
   */
  float_t step;
} nine_update_progress_event_t;

// 系统升级完成事件
/**
 * @class nine_update_finish_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 升级完成事件
 */
typedef struct _nine_update_finish_event_t {
  /**
   * @property {float_t} result
   * @annotation ["readable", "scriptable"]
   * 升级完成结果, 0 失败 1 成功
   */
  int32_t result;
} nine_update_finish_event_t;

#define NINE_MAX_BASIC_ERROR_DATA_SIZE 256
typedef struct _nine_basic_error_event_t {
  int32_t code;
  char text[NINE_MAX_BASIC_ERROR_DATA_SIZE];    // 故障提示的信息正文
} nine_basic_error_event_t;

#define NINE_MAX_WEATHER_TITLE_SIZE 256
#define NINE_MAX_WEATHER_DATA_SIZE 256
// 天气预警详情信息事件
/**
 * @class nine_weather_details_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 天气预警详情信息事件
 */
typedef struct _nine_weather_details_event_t {
  int32_t INTERVAL;	// 请求间隔			// 10	60	10	min
  int32_t SIGN;		// 天气预报标识
  int32_t TEMP;		// 天气预报温度		// 50	-50	100
  int32_t SCALE;	// 天气预报风力等级	// 0	0	0x0c
  int32_t ALERT;	// 天气预报是否预警  0x01是 0x02否
  int8_t TYPE[8];		// 天气预警类型, 允许多条预警A|B|C, 最多8条, 0xff表示无
  int32_t TIME;		// 天气预警时间（uint32_t 时间戳）
  char title[NINE_MAX_WEATHER_TITLE_SIZE];  // 天气预警标题(多条标题A|B|C会合并为一条title(使用换行符隔离)
  char text[NINE_MAX_WEATHER_DATA_SIZE];    // 天气预警正文
} nine_weather_details_event_t;

// 低电量报警事件
/**
 * @class nine_low_battery_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 低电量报警事件
 */
typedef struct _nine_low_battery_event_t {
  /**
   * @property {int32_t} votage
   * @annotation ["readable", "scriptable"]
   * 3块电池的电压值(毫伏单位)
   */
  int32_t votage[3];
} nine_low_battery_event_t;

#define NINE_MAX_POWER_DISSIPATION_DATA_SIZE 1024
// 功耗数据事件
/**
 * @class nine_power_dissipation_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 功耗数据事件
 */
typedef struct _nine_power_dissipation_event_t {
   /**
   * @property {char} data
   * @annotation ["readable", "scriptable"]
   * 包数据
   */
  short int data[NINE_MAX_POWER_DISSIPATION_DATA_SIZE];
  int count;    // 有效数据格式
  int index;    // 取值索引，即是从该位置开始往后取30个点

} nine_power_dissipation_event_t;

// 功耗曲线实时功耗事件
/**
 * @class nine_power_dissipation_realtime_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 功耗曲线实时功耗事件
 */
typedef struct _nine_power_dissipation_realtime_event_t {
 int32_t value;   // 功率曲线的实时功耗
} nine_power_dissipation_realtime_event_t;

// 功率曲线的行驶数据(时间，里程以及平均速度)
/**
 * @class nine_traffic_data_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 功率曲线的行驶数据
 */
typedef struct _nine_traffic_data_event_t {
 int32_t time;   // 行驶时间
 int32_t distance;   // 行驶里程
 int32_t speed;   // 行驶s速度
} nine_traffic_data_event_t;

// 导航事件
/**
 * @class nine_navigation_event_t
 * @annotation ["scriptable"]
 * @parent event_t
 * 导航事件
 */
typedef struct _nine_navigation_event_t {
  int32_t totalDistance;  // 本次导航总距离,单位m
  int32_t retainDistance;   // 剩余总距离,单位m
  int32_t retainTime;   // 剩余总时间, 单位s
  int iconType;     // 下一个导航动作
  int curStepRetainDis; // 距下一导航动作距离, 单位m
  int trafficLightNum;  // 当前剩余红绿灯个数
  int gpsStrength;      // 当前GPS信号强度
  char currentRoadName[64];   // 当前路名
  char nextRoadName[64];      // 下一路名
  char NavigationText[256];   // 语音播报内容
} nine_navigation_event_t;

typedef struct _nine_high_beam_event_t {
  int32_t on;
} nine_high_beam_event_t;

typedef struct _nine_low_beam_event_t {
  int32_t on;
} nine_low_beam_event_t;

typedef struct _nine_cruise_control_event_t {
  int32_t on;
} nine_cruise_control_event_t;

typedef struct _nine_left_turn_event_t {
	int32_t on;
} nine_left_turn_event_t;

typedef struct _nine_right_turn_event_t {
	int32_t on;
} nine_right_turn_event_t;

typedef struct _nine_abs_event_t {
	int32_t on;
} nine_abs_event_t;

#define NINE_GSM_STATE_NO_SIGNAL    0   // 无信号
#define NINE_GSM_STATE_ONE          1   // 1格
#define NINE_GSM_STATE_TWO          2   // 2格
#define NINE_GSM_STATE_FULL         3   // 满格
/**
 * @class nine_gsm_event_t
 * @annotation ["scriptable"]
 * GSM信号强度
 */
typedef struct _nine_gsm_event_t {
  int32_t state;
} nine_gsm_event_t;

typedef struct _nine_ready_hint_t {
  char hint[8];
} nine_ready_hint_t;

#define NINE_GPS_STATE_NO_SIGNAL    0 // 无信号
#define NINE_GPS_STATE_STRONG       1 // 强
#define NINE_GPS_STATE_WEAK         2 // 弱
/**
 * @class nine_gps_event_t
 * @annotation ["scriptable"]
 * GPS信号强度
 */
typedef struct _nine_gps_event_t {
  int32_t state;
} nine_gps_event_t;

typedef struct _nine_date_time_event_t {
  int32_t msec;   // 毫秒 (0 ~ 999)
  int32_t second; // 秒 (0 ~ 59)
  int32_t minute; // 分 (0 ~ 59)
  int32_t hour;   // 时 (0 ~ 23)
  int32_t day;    // 天 (1 ~ 31)
  int32_t wday;   // 星期 (1 ~ 7)
  int32_t month;  // 月 (1 ~ 12)
  int32_t year;   // 年 (2021 ~ 2049)  
} nine_date_time_event_t;

typedef struct _nine_odo_event_t {
  int32_t odo;
} nine_odo_event_t;

typedef struct _nine_trip_event_t {
  int32_t trip;
} nine_trip_event_t;

#define NINE_LIGHT_MODE_DAY          0   // 白天模式
#define NINE_LIGHT_MODE_NIGHT        1   // 黑夜模式
/**
 * @class nine_light_switch_event_t
 * @annotation ["scriptable"]
 * 光敏黑白切换 
 */
typedef struct _nine_light_switch_event_t {
  int32_t mode; // 白天/黑夜模式
} nine_light_switch_event_t;


#define NINE_THEME_1          1   // 主题1
#define NINE_THEME_2          2   // 主题2
#define NINE_THEME_3          3   // 主题3
/**
 * @class nine_theme_switch_event_t
 * @annotation ["scriptable"]
 * 主题切换 
 */
typedef struct _nine_theme_switch_event_t {
  int32_t theme; // 主题
} nine_theme_switch_event_t;

typedef struct _nine_key_long_auto_event_t {
  int32_t locked_time; // 使用密码解锁的剩余锁定时间
} nine_key_long_auto_event_t;

typedef struct _nine_password_event_t {
  int32_t result; // 0 密码校验失败 1 密码校验成功
} nine_password_event_t;

typedef struct _nine_page_switch_event_t {
  int32_t page; // 页标识id
} nine_page_switch_event_t;

// 电池剩余里程事件定义
typedef struct _nine_remaining_mileage_event_t {
  int32_t remaining_mileage; // 电池剩余里程
} nine_remaining_mileage_event_t;

// 充电剩余时间事件定义
typedef struct _nine_charge_time_event_t {
  int32_t remaining_time; // 充电剩余时间(分钟)
  int32_t percent;   // 百分比
} nine_charge_time_event_t;

typedef struct _nine_event_t {
  event_t e;
  nine_event_type_t type;
  union {
    nine_vcu_not_ready_event_t not_ready_event;
    nine_vcu_cycling_event_t  cycling_event;
    nine_vcu_running_status_event_t running_status_event;
    nine_vcu_gear_event_t gear_event;
    nine_wifi_connect_event_t wifi_event;
    nine_update_progress_event_t update_progress_event;
    nine_update_finish_event_t update_finish_event;
    nine_basic_error_event_t basic_error_event;
    nine_weather_details_event_t weather_details_event;
    nine_low_battery_event_t low_battery_event;
    nine_power_dissipation_event_t power_dissipation_event;
    nine_power_dissipation_realtime_event_t power_dissipation_realtime_event;
    nine_traffic_data_event_t traffic_data_event;
    nine_navigation_event_t navigation_event;
    nine_high_beam_event_t high_beam_event;
    nine_low_beam_event_t low_beam_event;
    nine_left_turn_event_t left_turn_event;
    nine_right_turn_event_t right_turn_event;
    nine_cruise_control_event_t cruise_control_event;
    nine_abs_event_t abs_event;
    nine_gsm_event_t gsm_event;
    nine_gps_event_t gps_event;
    nine_date_time_event_t date_time_event;
    nine_odo_event_t odo_event;
    nine_trip_event_t trip_event;
    nine_light_switch_event_t light_switch_event;
    nine_theme_switch_event_t theme_switch_event;
    nine_key_long_auto_event_t key_long_auto_event;
    nine_password_event_t password_event;
    nine_page_switch_event_t page_switch_event;
    nine_remaining_mileage_event_t remaining_mileage_event;
    nine_charge_time_event_t charge_time_event;
  };
} nine_event_t;



END_C_DECLS

#endif