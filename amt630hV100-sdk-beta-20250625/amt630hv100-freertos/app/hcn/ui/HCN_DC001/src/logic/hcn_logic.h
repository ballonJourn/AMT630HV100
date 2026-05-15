#ifndef _HCH_LOGIC_H__
#define _HCH_LOGIC_H__

#include "view/home_view/home_view_interface.h"

typedef enum
{
    REFRESH_TIMER_50_MS   ,
    REFRESH_TIMER_100_MS  ,
    REFRESH_TIMER_500_MS  ,
    REFRESH_TIMER_1000_MS ,
    //add

    REFRESH_TIMER_NUM_MAX,
}timer_type_e;

ret_t set_view_init(widget_t * win);

ret_t home_view_init(widget_t * view) ;

ret_t home_timer_init();

ret_t timer_refresh_50_ms(const timer_info_t *info);

ret_t timer_refresh_500_ms(const timer_info_t *info);




#endif