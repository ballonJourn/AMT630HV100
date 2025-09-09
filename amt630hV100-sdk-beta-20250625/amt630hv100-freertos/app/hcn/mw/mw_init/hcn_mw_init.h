/**
*
* @file hcn_mw_init.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 14:45
* @author och
*
*/
#ifndef __HCN_MW_INIT_H__
#define __HCN_MW_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "storage_param1/hcn_usr_param.h"
#include "storage_param2/hcn_mile_param.h"
#include "backlight/hcn_backlight.h"
#include "key_module/hcn_gpio_key.h"
#include "can_module/hcn_can_rx.h"

void hcn_mw_init(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MW_INIT_H__