/**
*
* @file hcn_ign.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/14 14:45
* @author och
*
*/
#ifndef __HCN_IGN_H__
#define __HCN_IGN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

typedef enum {
    IGN_OFF_NONE,
    IGN_OFF_PREPARE, ///< acc-det off
    IGN_OFF,         ///< acc-det off more than 300ms
} ign_state_e;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_IGN_H__
