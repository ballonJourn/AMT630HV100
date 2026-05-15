#ifndef HOME_POWER_VIEW_H
#define HOME_POWER_VIEW_H
#include "common.h"

enum home_power_com{
    POWER_VALUE ,
    POWER_BAR   ,
    POWER_NUM_MAX
};


ret_t home_power_view_init(widget_t* parent) ;

ret_t home_refresh_power(int power) ;
#endif