/**
*
* @file hcn_version.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/11/10 17:23
* @author och
*
*/
#ifndef __HCN_VERSION_H__
#define __HCN_VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "config/hcn_config.h"

#define APP_PROJECT_NUM "DC001"
#define APP_UI_VERSION  "XMAX"
#define APP_SUB_NUM     "V1"

void soc_version_init(void);
const char *get_soc_version(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_VERSION_H__