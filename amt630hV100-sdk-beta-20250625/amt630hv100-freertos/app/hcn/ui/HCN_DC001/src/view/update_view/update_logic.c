#include "update_logic.h"
#include <string.h>
#include <stdlib.h>
#include "update_view.h"
#include "proxy/vehicle_data.h"
#include "proxy/mirror_data.h"
#include "proxy/vehicle_ota.h"

#define REFRESH_INTERVAL_100_MS   (100)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;

static ret_t timer_refresh_100_ms(const timer_info_t *info) ;

static ret_t on_update_page_changed(void* ctx, event_t* e) ;

static update_info_t update_info = {0};

ret_t update_init(widget_t *win) 
{
    if (win == NULL) return RET_FAIL ;

    update_view_init(win) ;

    widget_on(win , EVT_WINDOW_CLOSE     , on_update_page_changed ,win);
    widget_on(win , EVT_WINDOW_WILL_OPEN , on_update_page_changed ,win);

    update_timer_init() ;

    return RET_OK ;
}


void update_timer_init()
{
    timer_array[REFRESH_TIMER_100_MS]  = timer_add( timer_refresh_100_ms ,  NULL , REFRESH_INTERVAL_100_MS ) ;

    return ;
}


static ret_t on_update_page_changed(void* ctx, event_t* e)
{
    
    if (e->type == EVT_WINDOW_CLOSE)
    {
        printf("on_update_page_changed EVT_WINDOW_CLOSE\n") ;
        if(timer_array[REFRESH_TIMER_100_MS] != 0 
            && timer_find(timer_array[REFRESH_TIMER_100_MS]))
        {
            timer_remove(timer_array[REFRESH_TIMER_100_MS]) ;
            timer_array[REFRESH_TIMER_100_MS] = 0 ;
            printf("on_update_page_changed timer_remove successed\n");
        }

        memset(&update_info , 0x00 , sizeof(update_info)) ;
    }
    else if(e->type == EVT_WINDOW_WILL_OPEN)
    {
        printf("on_update_page_changed EVT_WINDOW_WILL_OPEN\n") ;
        timer_refresh_100_ms(NULL);
    
    }

  return RET_OK ;
}

static ret_t timer_refresh_100_ms(const timer_info_t *info)
{
    (void)info ;

    update_info_t *_update_info =  vehicle_get_uptate_info() ;
    if (_update_info)
    {
        if (_update_info->type != update_info.type)
        {
            update_refresh_type(_update_info->type) ;
            update_info.type = _update_info->type ;
        }

        if (_update_info->status != update_info.status)
        {
            update_refresh_state(_update_info->status) ;
            update_info.status = _update_info->status ;
        }
        
        if (_update_info->progress != update_info.progress)
        {
            update_refresh_bar(_update_info->progress) ;
            update_info.progress =  _update_info->progress ;
        }
        
        if (_update_info->error != update_info.error)
        {
            update_refresh_error(_update_info->error) ;
            update_info.error = _update_info->error ;
        }

    }
    
    return RET_REPEAT ;
}
