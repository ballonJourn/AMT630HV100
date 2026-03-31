/**
*
* @file hcn_uart_common.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 09:36
* @author och
*
* @note  具体定义见<<恒晨两轮车仪表soc与mcu通信协议V1.0.docx>> 
*/
#ifndef __HCN_UART_COMMON_H__
#define __HCN_UART_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_UART_COMM_ENABLE

/**
 * @brief 串口通信消息队列长度
 */
#define UART_MCU_QUEUE_LEN (10)

/**
 * @brief 通信协议最大最小长度
 */
#define UART_MCU_MSG_MAX_LEN (260)
#define UART_MCU_MSG_MIN_LEN (10)

/**
 * @brief 通信协议命令字地址
 */
#define UART_MCU_CMD_L_ADDR (4)
#define UART_MCU_CMD_H_ADDR (5)

/**
 * @brief 通信协议数据长度地址
 */
#define UART_MCU_DATA_LEN_L_ADDR (6)
#define UART_MCU_DATA_LEN_H_ADDR (7)

/**
 * @brief 通信协议头尾定义
 */
#define UART_MCU_MSG_HEAD_1 (0x24)  ///< $
#define UART_MCU_MSG_HEAD_2 (0x24)  ///< $
#define UART_MCU_MSG_TAIL   (0x23)  ///< #

/**
 * @brief 通用命令范围
 * 命令范围: 0x0000-0x0400
 */
#define UART_MCU_NORMAL_START_CMD (0x0000)
#define UART_MCU_NORMAL_END_CMD (0x0400)

/**
 * @brief 定时命令范围
 * 命令范围: 0x0401-0x0800
 */
#define UART_MCU_TIMING_START_CMD (0x0401)
#define UART_MCU_TIMING_END_CMD (0x0800)

/**
 * @brief MCU升级命令范围
 * 命令范围: 0x1001-0x1400 
 */
#define UART_MCU_UPDATE_STAT_CMD (0x1001)
#define UART_MCU_UPDATE_END_CMD (0x1400)

/**
 * @brief SOC端请求和回复命令范围
 * 命令范围: 0x8000--0x8400
 */
#define UART_MCU_SOC_REQ_START_CMD (0x8000)
#define UART_MCU_SOC_REQ_END_CMD (0x8400)

/**
 * @brief SOC端命令定义
 */
#define UART_MCU_CMD_SHAKE_HANDS (0x8000)
#define UART_MCU_CMD_ACK_SHAKE_HANDS (0x0001)

#define UART_MCU_CMD_SET_TIME (0x8001)
#define UART_MCU_CMD_ACK_SET_TIME (0x0402)

#define UART_MCU_CMD_START_RECORD (0x8802)
#define UART_MCU_CMD_ACK_START_RECORD (0x8802)

#define UART_MCU_CMD_STOP_RECORD (0x8803)
#define UART_MCU_CMD_ACK_STOP_RECORD (0x8803)

#define UART_MCU_CMD_SHUTDOWN (0x8003)
#define UART_MCU_CMD_ACK_SHUTDOWN (0x0004)

#define UART_MCU_CMD_REQ_VER (0x8004)
#define UART_MCU_CMD_CHECKSELF (0x8005)
#define UART_MCU_CMD_CTRL_SYSTEM_MODE (0x8006)
#define UART_MCU_CMD_CLAER_SUB_MILEAGE (0x8007)
#define UART_MCU_CMD_CLAER_EEPROM (0x8008)

#define UART_MCU_CMD_SET_ODO_DATA (0x9002)
#define UART_MCU_CMD_REQUEST_ODO_DATA (0x9003)

#define UART_MCU_CMD_SET_TRIP_A_DATA (0x9004)   
#define UART_MCU_CMD_SET_TRIP_B_DATA (0x9005)   

#define UART_MCU_CMD_REQUEST_TRIP_DATA (0x9006)

/**
 * @brief MCU上报车辆信息相关命令
 */
#define UART_MCU_CMD_ACC_STATE (0x0002)
#define UART_MCU_CMD_VEHICLE_INFO (0x0003)

#define UART_MCU_CMD_VER_INFO (0x0006)
#define UART_MCU_CMD_ODO_INFO (0x0008)
#define UART_MCU_CMD_TRIP_INFO (0x0009)
#define UART_MCU_CMD_RPM_AND_SPEED (0x000A)
#define UART_MCU_CMD_VEHICLE_INFO1 (0x000B)
#define UART_MCU_CMD_GEAR_INFO (0x000C)
#define UART_MCU_CMD_START_SRC (0x000E)
#define UART_MCU_CMD_VEH_LIGHT_INFO (0x000F)
#define UART_MCU_CMD_ABS_INFO (0x0010)
#define UART_MCU_CMD_TORQUE_INFO (0x0011)

/**
 * @brief 时间设置相关命令
 */
#define UART_MCU_CMD_REPORT_TIME_INFO (0x0401)
#define UART_MCU_CMD_ACK_TIME_SET (0x0402)

/**
 * @brief MCU升级相关命令
 */
#define UART_CONTINUE_UPDATE_MCU_CMD (0x1001)
#define UART_RESEND_MCU_MSG_CMD (0x1002)
#define UART_UPDATE_MCU_AGAIN_CMD (0x1003)

/**
 * @brief MCU消息结构体
 */
typedef struct {
    uint8_t buffer[UART_MCU_MSG_MAX_LEN];
} hcn_mcu_msg_t;

/**
 * @brief CRC(BCC)计算校验和
 * @param  buffer 数据缓冲区
 * @return 校验和
 */
uint8_t uart_mcu_calc_crc(uint8_t *buffer);

/**
 * @brief  初始化MCU串口通信
 * @param  none 
 * @return 0:成功 -1:失败
 */
int uart_mcu_init(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_COMMON_H__