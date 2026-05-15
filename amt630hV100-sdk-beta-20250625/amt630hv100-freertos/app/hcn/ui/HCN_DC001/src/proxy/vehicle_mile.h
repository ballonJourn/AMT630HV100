
#ifndef VEHICLE_MILE__H
#define VEHICLE_MILE__H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  读取usr mile准备状态
 * @mile  none
 * @return false:参数未准备好 true:已准备好
 */

bool vehicle_get_mile_recovery();

//odo
uint32_t vehicle_get_mile_odo();
void vehicle_set_mile_odo(double value) ;

// send mcu odo
void vehicle_save_odo_mileage(void);

//tripA
uint32_t vehicle_get_mile_tripA();
void vehicle_set_mile_tripA(double value) ;
void vehicle_save_mile_tripA(void);

//tripB
uint32_t vehicle_get_mile_tripB();
void vehicle_set_mile_tripB(double value) ;

//单次里程
uint32_t vehicle_get_mile_once();
void vehicle_set_mile_once(double value) ;

int32_t vehicle_get_mile_changed() ;

#endif