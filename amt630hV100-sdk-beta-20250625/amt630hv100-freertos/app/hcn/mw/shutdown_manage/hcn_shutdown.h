/**
*
* @file hcn_shutdown.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/14 17:59
* @author och
*
*/
#ifndef __HCN_SHUTDOWN_H__
#define __HCN_SHUTDOWN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
void hcn_shutdown_init(void);
void start_power_off_timer(void);
void stop_power_off_timer(void);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_SHUTDOWN_H__
