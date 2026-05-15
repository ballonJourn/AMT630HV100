#ifndef ANIMATION_CTRL_H
#define ANIMATION_CTRL_H
#include "common.h"
#include <stdbool.h>

enum animation_move_com{
    PEED_VIEW        , 
    POWER_VIEW       ,
    DOCK_SLIDER_VIEW ,
    MVOE_NUM_MAX     ,
};

enum animation_type{
    ANIMATION_OUT ,
    ANIMATION_IN  ,
    ANIMATION_TYPE_MAX ,
};

ret_t home_animation_init(widget_t* parent) ;

ret_t animation_listen_out(void* ctx, event_t* e) ;

ret_t animation_listen_in(void* ctx, event_t* e) ;

ret_t animation_play_out();

ret_t animation_play_in();


//演示模式API
ret_t demonstration_start() ;

ret_t demonstration_stop();

bool get_demonstration_state() ; 


ret_t calling_animation_start() ;

ret_t calling_animation_stop() ;

void refresh_pop_call_state(int state);

#endif