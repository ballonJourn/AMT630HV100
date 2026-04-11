/**
*
* @file hcn_mileage_maintence.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/07 14:23
* @author och
*
*/
#ifndef __HCN_MILEAGE_MAINTENCE_H__
#define __HCN_MILEAGE_MAINTENCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_MILEAGE_MAINTENCE_ENABLE

/**
 * @brief  清除保养状态（用户保养的情况下清除）
 * @param  无
 * @return 无
 */
void clean_maintenance_state(void);

/**
 * @brief  刷新里程保养状态
 * @param  [in] mileage:总里程数据
 * @return 无
 */
void update_maintence_mileage(int mileage);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MILEAGE_MAINTENCE_H__
