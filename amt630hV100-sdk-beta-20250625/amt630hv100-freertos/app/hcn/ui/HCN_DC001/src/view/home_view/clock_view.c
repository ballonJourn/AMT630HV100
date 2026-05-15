#include "clock_view.h"

const char* home_clock_widget_name[CLOCK_NUM_MAX] = {
    "min" , "colon" , "sec" 
} ;

static widget_t* home_clock_widget[CLOCK_NUM_MAX] = { NULL };

ret_t home_clock_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;

    for (size_t i = 0; i < CLOCK_NUM_MAX; i++){
        home_clock_widget[i] = widget_lookup(parent, home_clock_widget_name[i], TRUE);
    }

    return RET_OK;
}

ret_t home_refresh_clock_min(int min)
{
    char buff[4] ;
    tk_snprintf(buff , sizeof(buff) , "%02d" , min);
    if (home_clock_widget[CLOCK_MIN])
    {
       widget_set_text_utf8(home_clock_widget[CLOCK_MIN] , buff) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_clock_sec(int sec)
{
    char buff[4] ;
    tk_snprintf(buff , sizeof(buff) , "%02d" , sec);
    if (home_clock_widget[CLOCK_SEC])
    {
       widget_set_text_utf8(home_clock_widget[CLOCK_SEC] , buff) ;
    }
    
    return RET_OK ;
} 

ret_t home_refresh_clock_colon(int visiable)
{
    if (home_clock_widget[CLOCK_COLON])
       widget_set_visible(home_clock_widget[CLOCK_COLON] , visiable ? TRUE : FALSE ) ;
    
    
    return RET_OK ;
} 