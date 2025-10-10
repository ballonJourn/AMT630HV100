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
#include "chip.h"
#include "board.h"
#include "light_sensor/hcn_light_sensor.h"

#ifdef HCN_ADC_LIGHT_SENSOR_ENABLE

static uint32_t sensor_value = 0;

void set_light_sensor_value(uint32_t value) {
	sensor_value = value;
    printf("light sensor value = %d\n", value);
}

void light_sensor_init(void) {
	adc_channel_enable(ADC_CH_AUX7);
}

uint32_t get_light_sensor_value() {
	return sensor_value;
}

void light_sensor_process(void) {
    uint32_t value = adc_get_channel_value(ADC_CH_AUX7);
    printf("sensor value = %d\n", value);
}

#endif