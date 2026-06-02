/**
*
* @file hcn_display_mode.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/29 12:27
* @author och
*
*/
#ifndef __HCN_DISPLAY_MODE_H__
#define __HCN_DISPLAY_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_ADC_LIGHT_SENSOR_ENABLE

#define LIGHT_SENSOR_ARR_LEVEL  (6)
#define LIGHT_SENSOR_ARR_SIZE   (5)

typedef enum {
    DAY_MODE = 0x00,
    NIGHT_MODE = 0x01,
    AUTO_MODE  = 0x02
} display_mode_e;

/**
 * @brief  显示模式初始化
 * @param  none
 * @return none
 */
void display_mode_init(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_DISPLAY_MODE_H__