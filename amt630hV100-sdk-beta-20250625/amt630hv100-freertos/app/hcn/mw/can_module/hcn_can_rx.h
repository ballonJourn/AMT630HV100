/**
*
* @file hcn_can_rx.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 09:40
* @author och
*
*/
#ifndef __HCN_CAN_RX_H__
#define __HCN_CAN_RX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "FreeRTOS.h"

typedef struct {
    uint32_t ecu_110_rx_timeout;
    uint32_t ecu_402_rx_timeout;
    uint32_t ecu_12b_rx_timeout;
} can_rx_timeout;

int can_module_init(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CAN_RX_H__