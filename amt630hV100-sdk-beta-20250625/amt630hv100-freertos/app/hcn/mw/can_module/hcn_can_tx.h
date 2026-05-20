/**
*
* @file hcn_can_tx.h
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
#ifndef __HCN_CAN_TX_H__
#define __HCN_CAN_TX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "semphr.h"
#include "can.h"

int can_msg_tx_msg_init(CanPort_t *cap);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CAN_TX_H__