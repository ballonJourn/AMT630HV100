#include <stddef.h>
#include <stdbool.h>
#include "vehicle_data.h"
#include "view/home_view/common.h"
#include "vehicle_param/vehicle_param.h"


typedef struct {
  veh_signal_e signal_lamp;
  veh_data_e veh_data;
} Signal_Lamp_Mapping_t;

#define VEH_INVALID_VALUE INT32_MIN

static const Signal_Lamp_Mapping_t signalMaps[] = {
    { VEH_HIGH_BEAM , VEH_LIGHT_HIGH_BEAM     } ,
    { VEH_LEFT      , VEH_INDICATOR_TURN_LEFT } ,
    { VEH_RIGHT     , VEH_INDICATOR_TURN_RIGHT} ,
    { VEH_HIGH_BEAM , VEH_LIGHT_HIGH_BEAM     } ,
    { VEH_ABS       , VEH_LIGHT_ABS           } ,
    { VEH_ENGINE    , VEH_LIGHT_HIGH_BEAM     } ,
    { VEH_ENGINE    , VEH_LIGHT_ENGINE_FAULT  } ,
};

static const int gearMaps[] = {
    [0] = GEAR_N ,
    [1] = GEAR_D ,
    [2] = GEAR_R ,
};

int32_t vehicle_get_data_signal_lamp(veh_signal_e lamp)
{
    int count = sizeof(signalMaps) / sizeof(Signal_Lamp_Mapping_t) ;
    for (size_t i = 0; i < count; i++){
        if (signalMaps[i].signal_lamp == lamp){
            #if  ON_PC_CACLE == 0 
            return vehicle_get_data(signalMaps[i].veh_data);
            #endif
        }
    }
    return VEH_INVALID_VALUE;
}


int32_t vehicle_get_data_speed()
{
#if !ON_PC_CACLE
    int32_t speed = vehicle_get_data(VEH_SPEED_CURRENT) ;
    return speed > SPEED_MAX ? SPEED_MAX : speed;
#endif

    return 0 ; 
}


int32_t vehicle_get_data_rpm()
{
#if !ON_PC_CACLE
    int32_t rpm = vehicle_get_data(VEH_SPEED_ENGINE);
    return rpm > RPM_MAX ? RPM_MAX : rpm;
#endif

    return 0 ; 
}


int32_t vehicle_get_data_gear() 
{
#if !ON_PC_CACLE
    int32_t gear_id = vehicle_get_data(VEH_GEAR_POSITION);
    return (gear_id > GEAR_R) ?  VEH_INVALID_VALUE : gearMaps[gear_id] ;
#endif 

    return 0 ; 

}


int32_t vehicle_get_data_power() 
{

#if !ON_PC_CACLE
    int32_t power = vehicle_get_data(VEH_TRAM_POWR);
    return power > POWER_MAX ? POWER_MAX : power;
#endif

    return 0 ; 
}

