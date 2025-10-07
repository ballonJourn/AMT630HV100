/**
*
* @file hcn_uart_parse_cmd.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 12:23
* @author och
*
*/
#ifndef __HCN_UART_PARSE_CMD_H__
#define __HCN_UART_PARSE_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_UART_COMM_ENABLE

/**
 * @brief  串口解析mcu命令任务初始化
 * @param  none
 * @return none
 */
int uart_mcu_parse_task_init(void);

/**
 * @brief  添加mcu命令解析任务
 * @param  data: uart msg pointer
 * @return none
 */
void uart_mcu_parse_add_task(uint8_t *data);

/**
 * @brief  获取mcu状态
 * @param  none 
 * @return 0:no status 1：APP staus 2:Preparing to upgrade 3:Update ok
 */
uint8_t get_mcu_status(void);

/**
 * @brief  获取mcu点火状态
 * @param  none 
 * @return true:acc on false:acc off
 */
bool get_mcu_ign_state(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_PARSE_CMD_H__