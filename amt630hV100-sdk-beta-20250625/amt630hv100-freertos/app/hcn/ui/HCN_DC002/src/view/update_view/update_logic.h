#ifndef UPDATE_VIEW_LOGICH__
#define UPDATE_VIEW_LOGICH__
#include "awtk.h"

typedef enum
{
    REFRESH_TIMER_100_MS  ,
    //add

    REFRESH_TIMER_NUM_MAX,
}update_timer_type_e;

void update_timer_init() ;

ret_t update_init(widget_t *win) ;


#endif
