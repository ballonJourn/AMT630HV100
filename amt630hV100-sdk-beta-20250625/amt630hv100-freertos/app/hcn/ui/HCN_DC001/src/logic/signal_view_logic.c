#include "signal_view_logic.h"
#include "proxy/vehicle_data.h"
#include "view/home_view/home_view_interface.h"

void signal_view_update()
{
    // signal_phone_GMS();
    signal_bluetooth();
    signal_turn_left();
    signal_turn_right();
    signal_GPS();
    signal_high_beam();
    signal_ready();
    signal_auto_beam();
    signal_ecu();
    signal_tcs();
    signal_abs();
    signal_brake();

    return ; 
}

void signal_phone_GMS()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_GMS);
    //home_refresh_GMS_level(ICON_GMS , 0.6 * value) ;
}

void signal_bluetooth()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_BT);
    home_refresh_signal(ICON_BT , value) ;
    home_refresh_signal(ICON_GMS , value) ;
}

void signal_GPS()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_GPS);
    home_refresh_signal(ICON_GPS , value) ;
}

void signal_turn_right()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_RIGHT);
    home_refresh_signal(ICON_RIGHT , value) ;
}

void signal_turn_left()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_LEFT);
    home_refresh_signal(ICON_LEFT , value) ;
}

void signal_high_beam()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_HIGH_BEAM);
    home_refresh_signal(ICON_HIGH_BEAM , value) ;
}

void signal_ready()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_READY);
    home_refresh_signal(ICON_READY , value) ;
}

void signal_auto_beam()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_AUTO_BEAM);
    home_refresh_signal(ICON_AUTO_BEAM , value) ;
}

void signal_ecu()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_ECU);
    home_refresh_signal(ICON_ECU , value) ;
}

void signal_tcs()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_TCS);
    home_refresh_signal(ICON_TCS , value) ;
}

void signal_abs()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_ABS);
    home_refresh_signal(ICON_ABS , value) ;
}

void signal_brake()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_BRAKE);
    home_refresh_signal(ICON_BRAKE , value) ;
}