#include "speed_view_logic.h"
#include "../proxy/vehicle_data.h"
#include "view/home_view/home_view_interface.h"

static int32_t speed         = 0 ;
static int32_t rpm           = 0 ;
static drv_mode_e drv_mode   = DRV_MODE_E ;
static gear_e  gear          = GEAR_N ;

void update_speed()
{
    int32_t value = vehicle_get_data_speed() ;
    if (value != speed)
    {
        home_refresh_speed(value);
        speed = value ;
    }
    
    return ;
}

void update_rpm()
{
    int32_t value = vehicle_get_data_rpm() ;
    if (value != rpm)
    {
        home_refresh_rpm(value);
        rpm = value ;
    }
    
    return ;
}

void update_drv_mode()
{
    (void)drv_mode ;
    return ;
}

void update_gear()
{
    uint32_t value = vehicle_get_data_gear() ;
    if (value != gear)
    {
        home_refresh_gear(value);
        gear = value ;
    }

    return ;
}


void speed_view_update()
{
    update_gear() ;
    update_drv_mode() ;
    update_rpm() ;
    update_speed() ;
}