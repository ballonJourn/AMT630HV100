#include "signal_view_logic.h"
#include "proxy/vehicle_data.h"
#include "view/home_view/home_view_interface.h"

void signal_view_update()
{
    // for (size_t i = VEH_HIGH_BEAM; i < VEH_SIGNAL_MAX ; i++)
    // {
    //     bool visiable =  vehicle_get_data_signal_lamp(i);
    // }
     
    signal_turn_left();
    signal_turn_right();
}


void signal_bt()
{
    return ;
}

void signal_GMS()
{
    return ;
}


void signal_turn_right()
{
    static bool_t visiable = FALSE ; 
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_RIGHT);
    if (visiable != value)
    {
        home_refresh_signal(ICON_RIGHT , visiable) ;
        visiable = value ;
    }
    return ;
}

void signal_turn_left()
{
    bool_t value = (bool_t)vehicle_get_data_signal_lamp(VEH_LEFT);
    home_refresh_signal(ICON_LEFT , value) ;

    return ;
}