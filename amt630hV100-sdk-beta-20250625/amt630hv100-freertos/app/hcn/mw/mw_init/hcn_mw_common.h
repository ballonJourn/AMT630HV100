/**
*
* @file hcn_mw_common.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 12:20
* @author och
*
*/
#ifndef __HCN_MW_COMMON_H__
#define __HCN_MW_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "rtc.h"

void mw_common_init(void);

/**
 * @brief  设置系统时间（区分mcu时间和630内部时间）
 * @param  none
 * @return no
 */
void set_os_date_time(SystemTime_t date_time);

/**
 * @brief  获取系统时间（区分mcu时间和630内部时间）
 * @param  none
 * @return 系统时间
 */
SystemTime_t get_os_date_time(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_MW_COMMON_H__