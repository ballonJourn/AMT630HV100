/**
*
* @file mcu_update_simulate.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/30 15:33
* @author och
*
*/
#ifndef __MCU_UPDATE_SIMULATE_H__
#define __MCU_UPDATE_SIMULATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include "config/hcn_config.h"
#include "uart_communicate/hcn_uart_common.h"

#ifdef MCU_UPDATE_SIMULATE_ENABLE

int mcu_simulate_init(void);
int send_mcu_simulate_msg(hcn_mcu_msg_t *msg);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __MCU_UPDATE_SIMULATE_H__