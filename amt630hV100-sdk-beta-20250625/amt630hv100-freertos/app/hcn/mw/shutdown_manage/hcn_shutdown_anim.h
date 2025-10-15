/**
*
* @file hcn_shutdown_anim.h
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
#ifndef __HCN_SHUTDOWN_ANIM_H__
#define __HCN_SHUTDOWN_ANIM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

/**
 * @brief  hcn ign off
 * @param  none
 * @return no
 */
void hcn_ign_off(void);

/**
 * @brief  hcn ign on
 * @param  none
 * @return no
 */
void hcn_ign_on(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_SHUTDOWN_ANIM_H__