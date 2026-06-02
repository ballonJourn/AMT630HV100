/**
*
* @file hcn_uart_dvr_parse_cmd.h
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
#ifndef __HCN_UART_DVR_PARSE_CMD_H__
#define __HCN_UART_DVR_PARSE_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_UART_DVR_ENABLE

/**
 * @brief  dvr命令解析
 * @param  none
 * @return none
 */
void uart_dvr_parse(uint8_t *data);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_PARSE_CMD_H__