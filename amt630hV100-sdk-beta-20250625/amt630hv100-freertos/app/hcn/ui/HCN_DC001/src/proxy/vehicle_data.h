#ifndef VEHICLE_SIGNAL_H
#define VEHICLE_SIGNAL_H

// #include "vehicle_param/vehicle_param.h"
#include "E:\HCN_WorkSpace\AMT630HV100\amt630hV100-sdk-beta-20250625\amt630hv100-freertos\app\hcn\mw\vehicle_param/vehicle_param.h"

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