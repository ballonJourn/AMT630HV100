/**
*
* @file hcn_mmwave_radar.h
*
* @brief 毫米波雷达模块头文件
*        协议参考：毫米波雷达串口通信协议V08_8_4
*        支持BSD(盲区检测)/CVW(交叉车辆预警)/RCW(后方碰撞预警)
*
* @ingroup mmwave_radar
*
* @date	2026/05/27
* @author yunlong
*
*/
#ifndef __HCN_MMWAVE_RADAR_H__
#define __HCN_MMWAVE_RADAR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_MMWAVE_RADAR_ENABLE

/*=============================================================================
 * 协议帧定义
 *===========================================================================*/

/**
 * @brief 通用协议 - 发送帧/回复帧
 * 帧结构: 帧头(2B) + 数据域(8B) + 校验和(2B) = 12字节
 */
#define RADAR_CMD_FRAME_LEN         (12)

///< 发送帧帧头
#define RADAR_CMD_TX_HEAD0          (0xDF)
#define RADAR_CMD_TX_HEAD1          (0x07)

///< 回复帧帧头
#define RADAR_CMD_RX_HEAD0          (0x60)
#define RADAR_CMD_RX_HEAD1          (0x07)

///< 产品代号 固定0x10
#define RADAR_PRODUCT_ID            (0x10)

/**
 * @brief SID 功能号
 */
#define RADAR_SID_READ_REQ          (0x11)  ///< 读取数据请求
#define RADAR_SID_WRITE_REQ         (0x12)  ///< 写入数据请求
#define RADAR_SID_READ_RESP         (0x51)  ///< 回复读取数据
#define RADAR_SID_WRITE_RESP        (0x52)  ///< 回复数据写入

/**
 * @brief DID 功能号
 */
#define RADAR_DID_VERSION           (0x05)  ///< 读取软件版本号
#define RADAR_DID_BSD_START_SPEED   (0x09)  ///< 设置/读取BSD启动速度
#define RADAR_DID_DATA_SWITCH       (0x24)  ///< 数据自动发送开关
#define RADAR_DID_VEHICLE_SPEED     (0x2A)  ///< 车身速度输入
#define RADAR_DID_DATA_TYPE         (0x67)  ///< 目标数据切换

/**
 * @brief 预警信息帧
 * 由两段组成:
 * Part1: 帧头 0x80 0x05 + 8B数据域 + 2B校验 = 12字节
 * Part2: 帧头 0xA0 0x04 + 8B数据域 + 2B校验 = 12字节
 * 总计24字节，连续上传
 */
#define RADAR_WARN_FRAME_TOTAL_LEN  (24)

///< 预警信息Part1帧头
#define RADAR_WARN_P1_HEAD0         (0x80)
#define RADAR_WARN_P1_HEAD1         (0x05)

///< 预警信息Part2帧头
#define RADAR_WARN_P2_HEAD0         (0xA0)
#define RADAR_WARN_P2_HEAD1         (0x04)

/**
 * @brief 预警标志位定义 (Part1 data5)
 */
#define RADAR_WARN_BSD_RIGHT        (0x01)  ///< Bit0: BSD右侧预警
#define RADAR_WARN_BSD_LEFT         (0x02)  ///< Bit1: BSD左侧预警
#define RADAR_WARN_CVW_RIGHT        (0x04)  ///< Bit2: CVW右侧预警
#define RADAR_WARN_CVW_LEFT         (0x08)  ///< Bit3: CVW左侧预警
#define RADAR_WARN_RCW              (0x10)  ///< Bit4: RCW后方碰撞预警

/**
 * @brief 目标信息帧
 * 帧头 0xAA 0x55 + num(1B) + 3个目标*6B + 校验(2B) = 22字节
 */
#define RADAR_TARGET_FRAME_LEN      (22)
#define RADAR_TARGET_HEAD0          (0xAA)
#define RADAR_TARGET_HEAD1          (0x55)
#define RADAR_TARGET_MAX_NUM        (3)

/**
 * @brief 接收缓冲区大小
 * 预警帧24字节 + 目标帧22字节，取较大值，留余量
 */
#define RADAR_RX_BUF_SIZE           (64)

/**
 * @brief 发送消息队列
 */
#define RADAR_TX_QUEUE_LEN          (4)

/*=============================================================================
 * 数据结构
 *===========================================================================*/

/**
 * @brief 预警信息数据
 */
typedef struct {
    uint8_t  warn_flags;         ///< 预警标志位 (BSD_R/BSD_L/CVW_R/CVW_L/RCW)
    uint8_t  bsd_start_speed;    ///< BSD启动速度 km/h
    uint8_t  current_speed;      ///< 当前车速 km/h
    uint8_t  left_distance;      ///< 左侧最近预警目标距离 m (默认0x64=100m无目标)
    uint8_t  right_distance;     ///< 右侧最近预警目标距离 m (默认0x64)
    uint8_t  rear_distance;      ///< 后侧预警目标距离 m (默认0x64)
    uint8_t  left_speed;         ///< 左侧最近预警目标速度 km/h (默认0xFF)
    uint8_t  right_speed;        ///< 右侧最近预警目标速度 km/h (默认0xFF)
    uint8_t  rear_speed;         ///< 后侧最近预警目标速度 km/h (默认0xFF)
} radar_warn_data_t;

/**
 * @brief 单个目标信息
 */
typedef struct {
    int16_t  x;       ///< X位置 精度0.1m (正=右侧, 负=左侧)
    int16_t  y;       ///< Y位置 精度0.1m (正=前方)
    int16_t  speed;   ///< 速度 精度1m/s (正=远离, 负=靠近)
} radar_target_t;

/**
 * @brief 目标信息数据
 */
typedef struct {
    uint8_t        target_num;                    ///< 探测到的目标数量 (0-3)
    radar_target_t targets[RADAR_TARGET_MAX_NUM]; ///< 目标数组
} radar_target_data_t;

/**
 * @brief 雷达发送消息结构体
 */
typedef struct {
    uint8_t buffer[RADAR_CMD_FRAME_LEN];
} radar_tx_msg_t;

/*=============================================================================
 * API
 *===========================================================================*/

/**
 * @brief  初始化毫米波雷达模块
 * @return 0:成功 -1:失败
 */
int mmwave_radar_init(void);

/**
 * @brief  获取当前预警数据
 * @return 预警数据指针
 */
const radar_warn_data_t *mmwave_radar_get_warn_data(void);

/**
 * @brief  获取当前目标数据
 * @return 目标数据指针
 */
const radar_target_data_t *mmwave_radar_get_target_data(void);

/**
 * @brief  获取雷达在线状态
 * @return true:在线 false:离线
 */
bool mmwave_radar_is_online(void);

/**
 * @brief  发送命令给雷达 (开关数据上传/设置BSD速度等)
 */
int mmwave_radar_send_cmd(uint8_t sid, uint8_t did,
                          uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4);

/**
 * @brief  请求读取雷达版本号
 */
int mmwave_radar_read_version(void);

/**
 * @brief  设置BSD启动速度
 * @param  speed_kmh 启动速度 km/h
 */
int mmwave_radar_set_bsd_speed(uint8_t speed_kmh);

/**
 * @brief  开关数据自动上传
 * @param  enable 1:开启 0:关闭
 */
int mmwave_radar_data_switch(uint8_t enable);

/**
 * @brief  切换上传数据类型
 * @param  type 0:目标数据 1:预警数据
 */
int mmwave_radar_set_data_type(uint8_t type);

/**
 * @brief  输入车身速度给雷达
 * @param  enable 1:使用车身速度 0:不使用
 * @param  speed  车速 精度0.1km/h
 */
int mmwave_radar_input_vehicle_speed(uint8_t enable, int16_t speed);

#endif /* HCN_MMWAVE_RADAR_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* __HCN_MMWAVE_RADAR_H__ */
