/**
*
* @file hcn_usr_param.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/08/25 11:49
* @author och
*
*/
#ifndef __HCN_USR_PARAM_H__
#define __HCN_USR_PARAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tpms/hcn_uart_tpms.h"

#define pads(index, num)   uint8_t pads##index[num]
#define MAX_UUID_LEN        (20)

typedef struct {
    uint16_t year;             ///< 年    
    uint8_t mon;               ///< 月
    uint8_t day;              ///< 日
} maintain_date_t;

typedef struct {
    maintain_date_t last_maintain_date;  ///< 上次保养日期 年月日
    uint16_t maintain_days;     ///< 维修天数，默认365天，可设置
    uint8_t is_sync_time;    ///< 是否同步过互联网时间 0:未同步 1:同步过
    uint8_t reserve;        ///< 预留1字节
} maintain_time_t;

typedef struct {
    uint16_t maintain_count;        ///< 保养次数
    uint16_t cur_maintain_mileage;  ///< 当前段保养距离 1000Km, 5000Km等，可设置
    uint32_t last_maintain_total_mileage; ///< 上次保养总里程
} maintain_mileage_t;

/**
 * @brief 保养信息结构体
 * @note 分为里程保养和时间保养
 */
typedef struct {
    maintain_mileage_t maintain_mile; ///< 里程保养
    maintain_time_t maintain_time;  ///< 时间保养
} maintain_info_t;

typedef struct {
    uint32_t ride_time_a;  ///< 骑行时间a,单位s
    uint32_t ride_time_b;  ///< 骑行时间b,单位s 
} ride_param_t;

typedef struct {
    uint32_t mile_format :1;  ///< 里程格式 0:公制 1：英制
    uint32_t mile_display :2; ///< 里程显示模式 0：显示ODO 1:trip_a 2:trip_b
    uint32_t theme :2;        ///< 显示主题 0：白天 1:黑夜 2:自动
    uint32_t brightness :3;  ///< 亮度等级 0:自动 1-5级
    uint32_t language   :3; ///< 语言 0:中文 1:英文,预留位
    uint32_t bt_switch :1;  ///< 蓝牙开关 0：关闭 1:打开
    uint32_t uuid_active_staus :1; ///< uuid激活状态 0:未激活 1:已激活
    uint32_t start_src :1;       ///< 启动源 0:bat 1:ign
    uint32_t tcs_switch:1;       ///< tcs开关 0:关闭 1:开启
    uint32_t temp_unit :1;      ///< 温度单位 0:摄氏度 1:华氏度、
    uint32_t time_format :1;   ///< 时间格式 0:12小时制 1:24小时制
    uint32_t tpms_unit:1;      ///< 胎压单位 0:KPA 1:BAR 2:PSI
    uint32_t auto_headlight :1; ///< 自动大灯状态 0:关闭 1:打开
    uint32_t phone_type :1;   ///< 手机类型 0:安卓  1:ios
    uint32_t sys_log :1;     ///< 系统日志控制开关 0:关闭 1:开启
    uint32_t drive_mode :1;  ///< 驾驶模式 0：eco经济模式  1:驾驶模式
    uint32_t resume :10;
    uint32_t usr_reserve;   ///< 用户设置参数预留 4字节
} usr_setting_t;

typedef struct {
    maintain_info_t maintain_info;
    usr_setting_t usr_set;
    ride_param_t ride_info;
    tpms_param_t tpms[MAX_WHEEL_POS_NUM];
    char carlink_uuid[MAX_UUID_LEN];
    pads(1, 40);        ///< 预留40个字节
} usr_param_t;

/**
 * @brief usr param enum
 */
typedef enum {
    HCN_PARAM_NONE,

    /* 里程保养参数 */
    HCN_PARAM_MAINTAIN_COUNTS,
    HCN_PARAM_CUR_MAINTAIN_MILEAGE,
    HCN_PARAM_LAST_MAINTAIN_MILEAGE,

    /* 里程保养参数 */
    HCN_PARAM_MAINTAIN_DAYS,
    HCN_PARAM_MAINTAIN_DATE,
    HCN_PARAM_MAINTAIN_IS_SYNC_TIME,

    /* 仪表设置参数 */
    HCN_PARAM_MILE_FORMAT,
    HCN_PARAM_MILE_DISPLAY,
    HCN_PARAM_THEME,
    HCN_PARAM_BRIGHTNESS_LEVEL,
    HCN_PARAM_LANGUAGE,
    HCN_PARAM_BT_SWITCH,
    HCN_PARAM_UUID_REGISTER,
    HCN_PARAM_METER_START_SRC,
    HCN_PARAM_TCS_SWITCH,
    HCN_PARAM_TEMP_UNIT,
    HCN_PARAM_TIME_FORMAT,
    HCN_PARAM_TPMS_PRESSURE_UNIT,
    HCN_PARAM_AUTO_HEAD_LIGHT,
    HCN_PARAM_PHONE_TYPE,
    HCN_PARAM_SYS_LOG,
    HCN_PARAM_DRIVE_MODE,

    /* 手机互联uuid */
    HCN_PARAM_EC_UUID,

    /* 骑行时间 */
    HCN_PARAM_RIDE_TIME_A,
    HCN_PARAM_RIDE_TIME_B,
    
    /* 胎压信息 */
    HCN_PARAM_LEFT_FRONT_TIRE_INFO,
    HCN_PARAM_RIGHT_FRONT_TIRE_INFO,
    HCN_PARAM_LEFT_REAR_TIRE_INFO,
    HCN_PARAM_RIGHT_REAR_TIRE_INFO,
} usr_param_handle_e;

/**
 * @brief  读取usr param准备状态
 * @param  none
 * @return false:参数未准备好 true:已准备好
 */
bool get_recovery_usr_param(void);

/**
 * @brief  读取usr param参数
 * @param  id 用户参数句柄
 * @param  param 获取用户参数指针变量，根据不同类型参数，传入对应类型指针
 * @return false:读取失败 true:读取成功
 */
bool get_hcn_usr_param(usr_param_handle_e id, void *param);

/**
 * @brief  设置usr param参数
 * @param  id 用户参数句柄
 * @param  param 设置用户参数指针变量，根据不同类型参数，传入对应类型指针
 * @return false:设置失败  true:设置成功
 */
bool set_hcn_usr_param(usr_param_handle_e id, void *param);

/**
 * @brief  检测仪表启动源，恢复部分设置参数
 * @param  start_src 0:bat 1:ign
 * @return 无
 */
void check_start_source(uint8_t start_src);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_USR_PARAM_H__