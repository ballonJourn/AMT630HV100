#include "speed_view_logic.h"
#include "../proxy/vehicle_data.h"
#include "view/home_view/home_view_interface.h"
#include "../proxy/vehicle_argument.h"
#include "logic/hcn_global.h"

static int32_t speed         = 0 ;
static int32_t rpm           = 0 ;
static int32_t power         = 0 ;
static int32_t remain_battary = 100 ;
static drv_mode_e drv_mode   = DRV_MODE_MAX ;
static gear_e  gear          = GEAR_MAX ;

void update_speed()
{
    int32_t value = vehicle_get_data_speed() ;
    if (value != speed)
    {
        int32_t temp_value = value ;
        if (MPH == vehicle_get_param_unit())
            temp_value  *= KM_CONVERT_MILE ; 
        
        home_refresh_speed(temp_value);

        home_refresh_rpm(value) ;  //车速 转速同一值

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
    int32_t value = vehicle_get_data_drv_mode();
    if (value != drv_mode)
    {
        home_refresh_drv_mode(value) ;
        drv_mode = value ;
    }
    
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

void update_power()
{
    int32_t value = vehicle_get_data_power() ;
    if (value != power)
    {
        home_refresh_power(value);
        power = value ;
    }
}

void update_remain_battary()
{
    int32_t value = vehicle_get_data_remain_battary() ;
    if (value != remain_battary)
    {
        home_refresh_electrical(value);
        remain_battary = value ;
    }
}

void speed_view_update()
{
    update_gear() ;
    update_drv_mode() ;
    // update_rpm() ;
    update_speed() ;

    update_power();
    update_remain_battary();
    return ;
}