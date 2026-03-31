/**
*
* @file hcn_carlink_task.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 12:27
* @author och
*
*/
#ifndef __HCN_CARLINK_TASK_H__
#define __HCN_CARLINK_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_CARLINK_WEATHER_ENABLE
void carlink_query_init(void);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CARLINK_TASK_H__