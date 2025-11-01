
#ifndef VEHICLE_ARGUMENT__H
#define VEHICLE_ARGUMENT__H

#include <stdint.h>
#include <stdbool.h>
#include "storage_param2/hcn_mile_param.h"

/**
 * @brief  读取usr mile准备状态
 * @mile  none
 * @return false:参数未准备好 true:已准备好
 */

bool vehicle_get_mile_recovery();

//odo
uint32_t vehicle_get_mile_odo();
void vehicle_set_mile_odo(double value) ;

//tripA
uint32_t vehicle_get_mile_tripA();
void vehicle_set_mile_tripA(double value) ;

//tripB
uint32_t vehicle_get_mile_tripB();
void vehicle_set_mile_tripB(double value) ;



#endif