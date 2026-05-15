/**
*
* @file hcn_uart_tx.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 14:41
* @author och
*
*/
#ifndef __HCN_UART_TX_H__
#define __HCN_UART_TX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "uart_communicate/hcn_uart_common.h"
#include "chip.h"
#include "board.h"
#include "uart.h"

#ifdef HCN_UART_COMM_ENABLE

int uart_mcu_tx_init(UartPort_t *uap);
hcn_mcu_msg_t *uart_alloc_msg(void);
void uart_mcu_free_msg(hcn_mcu_msg_t *msg);
int send_mcu_msg(hcn_mcu_msg_t *msg);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UART_TX_H__