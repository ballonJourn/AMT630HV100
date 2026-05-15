#ifndef DEVICE_VIEW_LOGICH__
#define DEVICE_VIEW_LOGICH__
#include "awtk.h"

typedef enum
{
    REFRESH_TIMER_100_MS  ,
    //add

    REFRESH_TIMER_NUM_MAX,
}device_timer_type_e;

void device_timer_init() ;

ret_t device_init(widget_t *win) ;


void device_page_deal_key_down ();
void device_page_deal_key_up   ();
void device_page_deal_key_set  ();
void device_page_deal_key_back ();

#endif
