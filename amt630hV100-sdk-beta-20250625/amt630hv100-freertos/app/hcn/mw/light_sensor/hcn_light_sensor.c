/**
*
* @file hcn_light_sensor.c
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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "light_sensor/hcn_light_sensor.h"
#include "log/hcn_log.h"
#include "chip.h"
#include "board.h"

#ifdef HCN_ADC_LIGHT_SENSOR_ENABLE

static uint32_t sensor_value = 0;

void set_light_sensor_value(void) {
    sensor_value = adc_get_channel_value(LIGHT_SENSOR_AD_CHANNEL);
}

uint32_t get_light_sensor_value(void) {
	return sensor_value;
}

#endif