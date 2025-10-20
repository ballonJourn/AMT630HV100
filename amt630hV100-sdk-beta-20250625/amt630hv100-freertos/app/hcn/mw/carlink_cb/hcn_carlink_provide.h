/**
*
* @file hcn_carlink_provide.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/16 10:59
* @author och
*
*/
#ifndef __HCN_CARLINK_PROVIDE_H__
#define __HCN_CARLINK_PROVIDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "hcn_carlink_cb.h"

void hcn_initialize(HcnLibConfig* HcnCfg, IhcnCallBack* HcnCallback);
IhcnCallBack *get_hcn_callback(void);
HcnLibConfig *get_hcn_lib_config(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CARLINK_PROVIDE_H__

