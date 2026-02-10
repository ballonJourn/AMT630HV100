#ifndef VEHICLE_SIGNAL_H
#define VEHICLE_SIGNAL_H

#include <stdint.h>
#define ON_PC_CACLE   0

typedef enum signal{
    VEH_GMS            ,
    VEH_BT             ,
    VEH_GPS            ,    
    VEH_HIGH_BEAM      ,     
    VEH_LEFT           , 
    VEH_READY          , 
    VEH_RIGHT          ,
    VEH_AUTO_BEAM      ,   
    VEH_ABS            ,
    VEH_ECU            ,
    VEH_TCS            , 
    VEH_BRAKE          ,     

    VEH_SIGNAL_MAX     ,
}veh_signal_e;


int32_t vehicle_get_data_signal_lamp(veh_signal_e lamp);

int32_t vehicle_get_data_speed();

int32_t vehicle_get_data_rpm();

int32_t vehicle_get_data_gear();

int32_t vehicle_get_data_power();

int32_t vehicle_get_data_drv_mode() ;

int32_t vehicle_get_data_remain_battary();

int32_t vehicle_get_data_current_display() ;

int32_t vehicle_get_ota_state() ;

const char* veicle_get_data_version();

const char* veicle_get_data_mcu_ver();

void vehicle_set_ota_page_state(int32_t state);

#endif