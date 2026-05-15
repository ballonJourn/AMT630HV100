#ifndef CLOCK_VIEW_H
#define CLOCK_VIEW_H
#include "common.h"

enum home_clcok
{
    CLOCK_MIN     ,
    CLOCK_COLON   ,
    CLOCK_SEC     ,
    CLOCK_NUM_MAX ,
};

ret_t home_clock_view_init(widget_t* parent) ;

ret_t home_refresh_clock_min(int min) ;

ret_t home_refresh_clock_sec(int sec) ;

ret_t home_refresh_clock_colon(int visiable) ;

#endif