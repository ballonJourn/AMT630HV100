#ifndef LINK_VIEW_LOGICH__
#define LINK_VIEW_LOGICH__
#include "awtk.h"

typedef enum
{
    REFRESH_TIMER_50_MS  ,
    //add

    REFRESH_TIMER_NUM_MAX,
}link_timer_type_e;

void link_timer_init() ;

ret_t link_init(widget_t *win) ;

#endif
