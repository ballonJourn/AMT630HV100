/**
*
* @file hcn_uart_dvr.h
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
#ifndef __HCN_UART_DVR_H__
#define __HCN_UART_DVR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_UART_DVR_ENABLE

/**
 * @brief 串口通信消息队列长度
 */
#define UART_DVR_QUEUE_LEN (4)

/**
 * @brief 通信协议最大最小长度
 */
#define UART_DVR_MSG_MAX_LEN (16)
#define UART_DVR_MSG_MIN_LEN (4)

/**
 * @brief 通信协议命令字地址
 */
#define UART_DVR_CMD_ADDR (1)

/**
 * @brief 通信协议头尾定义
 */
#define UART_DVR_MSG_HEAD (0xAA)  ///< $
#define UART_DVR_MSG_TAIL   (0x7F)  ///< #

/**
 * @brief DVR通用命令范围
 * 命令范围: 0x05-0x09
 */
#define UART_DVR_NORMAL_START_CMD (0x05)
#define UART_DVR_NORMAL_END_CMD (0x09)

/**
 * @brief SOC端请求和回复命令范围
 * 命令范围: 0x00--0x04
 */
#define UART_DVR_SOC_REQ_START_CMD (0x00)
#define UART_DVR_SOC_REQ_END_CMD (0x04)

/**
 * @brief DVR上报信息命令
 */
#define UART_DVR_CMD_TFCARD_LOAD_STATE (0x05)
#define UART_DVR_CMD_RECORD_STATE (0x06)

#define UART_DVR_CMD_MODE_TYPE (0x08)
#define UART_DVR_CMD_RECORD_TIME (0x09)

/**
 * @brief APP控制DVR命令
 */
#define UART_DVR_CMD_CTRL_RECORD (0x00)
#define UART_DVR_CMD_CTRL_MODE (0x01)
#define UART_DVR_CMD_CTRL_CAMERA (0x02)
#define UART_DVR_CMD_CTRL_TIME (0x03)
#define UART_DVR_CMD_CTRL_PRINTSCREEN (0x04)

/**
 * @brief DVR消息结构体
 */
typedef struct {
    uint8_t buffer[UART_DVR_MSG_MAX_LEN];
} hcn_dvr_msg_t;

/**
 * @brief  初始化DVR串口通信
 * @param  none 
 * @return 0:成功 -1:失败
 */
int uart_dvr_init(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_DVR_H__