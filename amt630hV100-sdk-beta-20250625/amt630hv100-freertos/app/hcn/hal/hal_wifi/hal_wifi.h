/**
*
* @file hal_wifi.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/09 10:34
* @author och
*
*/
#ifndef __HAL_WIFI_H__
#define __HAL_WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_WIFI_INIT_DELAY_ENABLE
int hcn_wifi_init(void);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HAL_WIFI_H__