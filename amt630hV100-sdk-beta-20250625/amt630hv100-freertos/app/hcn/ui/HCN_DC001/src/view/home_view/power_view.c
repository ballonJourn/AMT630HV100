#include "power_view.h"

const char* home_power_widget_name[POWER_NUM_MAX] = {
    "power_value" , "power_progress" 
} ;


static widget_t* home_power_widget[POWER_NUM_MAX] = { NULL };;


ret_t home_power_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < POWER_NUM_MAX; i++){
        home_power_widget[i] = widget_lookup(parent, home_power_widget_name[i], TRUE);
    }
    return RET_OK ;
}


ret_t home_refresh_power(int power) 
{
    power = tk_min(power , POWER_MAX) ;

    float step = (float)100.0f / POWER_MAX ;

    if(home_power_widget[POWER_VALUE]){
        image_value_set_value(home_power_widget[POWER_VALUE] , power) ;
    }

    if(home_power_widget[POWER_BAR]){
        slider_set_value(home_power_widget[POWER_BAR] , (power * step)) ;
    }
    
    return RET_OK ;
}