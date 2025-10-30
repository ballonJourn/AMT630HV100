/**
*
* @file hcn_light_sensor.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/10 09:01
* @author och
*
*/
#ifndef __HCN_LIGHT_SENSOR_H__
#define __HCN_LIGHT_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_ADC_LIGHT_SENSOR_ENABLE

#define LIGHT_SENSOR_AD_CHANNEL   (ADC_CH_AUX7)

uint32_t get_light_sensor_value(void);
void set_light_sensor_value(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_LIGHT_SENSOR_H__