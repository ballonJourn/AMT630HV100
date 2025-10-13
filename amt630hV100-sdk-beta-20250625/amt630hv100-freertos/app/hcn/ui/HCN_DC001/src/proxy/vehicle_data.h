#ifndef VEHICLE_SIGNAL_H
#define VEHICLE_SIGNAL_H

#include <stdint.h>
#define ON_PC_CACLE  1

typedef enum signal{
    VEH_GMS            ,   
    VEH_GPS            ,    
    VEH_BT             ,
    VEH_HIGH_BEAM      ,     
    VEH_LEFT           , 
    VEH_READY          , 
    VEH_RIGHT          ,
    VEH_NEAR_BEAM      ,   
    VEH_ABS            ,
    VEH_ECU            ,
    VEH_TCS            , 
    VEH_ENGINE         ,     

    VEH_SIGNAL_MAX     ,
}veh_signal_e;


int32_t vehicle_get_data_signal_lamp(veh_signal_e lamp);

int32_t vehicle_get_data_speed();

int32_t vehicle_get_data_rpm();

int32_t vehicle_get_data_gear();

int32_t vehicle_get_data_power();

#endif