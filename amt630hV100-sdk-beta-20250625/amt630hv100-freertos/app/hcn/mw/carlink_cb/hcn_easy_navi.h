/**
*
* @file hcn_easy_navi.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 10:12
* @author och
*
*/
#ifndef __HCN_EASY_NAVI_H__
#define __HCN_EASY_NAVI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>    
#include "carlink_cb/hcn_carlink_cb.h"

void parse_easy_navi_info(const hcnNavigationHudInfo *info);
hcnNavigationHudInfo *get_easy_navi_info(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // HCN_EASY_NAVI_H__