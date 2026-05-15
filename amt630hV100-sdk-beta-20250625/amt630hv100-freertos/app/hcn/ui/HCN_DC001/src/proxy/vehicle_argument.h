
#ifndef VEHICLE_ARGUMENT__H
#define VEHICLE_ARGUMENT__H

#include <stdint.h>
#include <stdbool.h>
#include "storage_param1/hcn_usr_param.h"

/**
 * @brief  读取usr param准备状态
 * @param  none
 * @return false:参数未准备好 true:已准备好
 */
bool vehicle_get_param_recovery();

//语言
uint8_t vehicle_get_param_language() ;

void vehicle_set_param_language(uint8_t value) ;


//单位
uint8_t vehicle_get_param_unit();

void vehicle_set_param_unit(uint8_t value);


//亮度等级
uint8_t vehicle_get_param_brightness() ;

void vehicle_set_param_brightness(uint8_t value) ;


//主题
uint8_t vehicle_get_param_display();

void vehicle_set_param_display(uint8_t value) ;


//蓝牙开关
uint8_t vehicle_get_param_bluetooth();

void vehicle_set_param_bluetooth(uint8_t value);

#endif