#include "animation_ctrl.h"
#include "dock_view.h"
#include "../view_manager.h"
#include "carlink_cb/hcn_carlink_cb.h"
#include <stdbool.h>

const char* home_move_animation_name[MVOE_NUM_MAX] = {
    "speed_view" , "power_view" , "dock_slider_view" 
} ;

static widget_t* home_animation_widget[MVOE_NUM_MAX] = { NULL };

static char* animation_name[MVOE_NUM_MAX][ANIMATION_TYPE_MAX] = {
    {"move_speed_out" , "move_speed_in"} ,
    {"move_power_out" , "move_power_in"} ,
    {"move_dock_out"  , "move_dock_in" } ,
};

static bool is_demo_state = false ;

static int timerId = 0 ;

ret_t _timer_refresh(const timer_info_t *info)
{
    if (home_animation_widget[DOCK_SLIDER_VIEW])
        widget_invalidate_force(home_animation_widget[DOCK_SLIDER_VIEW] , NULL);
    
    return RET_REPEAT ;
}


ret_t home_animation_init(widget_t* parent)
{

    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < MVOE_NUM_MAX; i++){
        home_animation_widget[i] = widget_lookup(parent, home_move_animation_name[i], TRUE);
    }

    if (home_animation_widget[PEED_VIEW])
    {
        widget_animator_t * an;
        an = widget_animator_manager_find(widget_animator_manager() ,home_animation_widget[PEED_VIEW] , animation_name[PEED_VIEW][ANIMATION_OUT] );
        if (an)
        {
            widget_animator_on(an ,EVT_ANIM_START , animation_listen_out , NULL) ;
            widget_animator_on(an ,EVT_ANIM_END  , animation_listen_out , NULL) ;
            printf("widget_animator_manager_find ANIMATION_OUT successed \n") ;
        }
        
        an = widget_animator_manager_find(widget_animator_manager() ,home_animation_widget[PEED_VIEW] , animation_name[PEED_VIEW][ANIMATION_IN] );
        if (an)
        {
            widget_animator_on(an ,EVT_ANIM_START , animation_listen_in , NULL) ;
            widget_animator_on(an ,EVT_ANIM_END  , animation_listen_in , NULL) ;
            printf("widget_animator_manager_find ANIMATION_IN successed \n") ;
        }
        
    }
    
    return RET_OK ;
}


ret_t animation_listen_out(void* ctx, event_t* e) 
{
    (void)ctx ;
    if (e->type == EVT_ANIM_START)
    {
        printf("animation out start\n") ;
        set_ready_press_state(false);
    }
    else if (e->type == EVT_ANIM_END)
    {
        printf("animation out end\n") ;
        set_ready_press_state(true);
        if (home_animation_widget[DOCK_SLIDER_VIEW])
        {
            // widget_set_style_str(home_animation_widget[DOCK_SLIDER_VIEW] , STYLE_ID_BG_IMAGE , "left_bg_p") ;
            slide_view_set_active_ex(home_animation_widget[DOCK_SLIDER_VIEW] , ICON_MUSIC_EX , FALSE ) ;
            widget_invalidate_force(home_animation_widget[DOCK_SLIDER_VIEW] , NULL);

            timerId = timer_add( _timer_refresh ,  NULL , 32 ) ;
        }
        
    }
    
    return RET_OK ;
}


ret_t animation_listen_in(void* ctx, event_t* e) 
{
    (void)ctx ;
    if (e->type == EVT_ANIM_START)
    {
        set_ready_press_state(false);
        if (home_animation_widget[DOCK_SLIDER_VIEW])
        {
            printf("animation in start\n") ;
            // widget_set_style_str(home_animation_widget[DOCK_SLIDER_VIEW] , STYLE_ID_BG_IMAGE , "left_bg_n") ;
            slide_view_set_active_ex(home_animation_widget[DOCK_SLIDER_VIEW] , ICON_MUSIC , FALSE ) ;
            widget_invalidate_force(home_animation_widget[DOCK_SLIDER_VIEW] , NULL);
        }
        
        if (timerId != 0 && timer_find(timerId))
        {
            timer_remove(timerId);
        }
        
    }
    else if (e->type == EVT_ANIM_END)
    {
        printf("animation in end\n") ;
        set_ready_press_state(true);
    }
    
    return RET_OK ;
}


ret_t animation_play_out()
{
    for (size_t i = 0; i < MVOE_NUM_MAX; i++)
    {
        widget_t *wget = home_animation_widget[i] ;
        if (wget){
            widget_start_animator(wget , animation_name[i][ANIMATION_OUT]) ;
        }
    }
    
    return RET_OK ;
}


ret_t animation_play_in()
{
    for (size_t i = 0; i < MVOE_NUM_MAX; i++)
    {
        widget_t *wget = home_animation_widget[i] ;
        if (wget){
            widget_start_animator(wget , animation_name[i][ANIMATION_IN]) ;
        }
    }
    
    return RET_OK ;
}


ret_t demonstration_start()
{
    widget_start_animator(NULL , "animation_demo");
    is_demo_state = true ;

    return RET_OK ;
}

ret_t demonstration_stop()
{
    widget_stop_animator(NULL , "animation_demo");
    is_demo_state = false ;
    return RET_OK ;
}

bool get_demonstration_state() 
{
    return is_demo_state ;
}


bool is_call_start = false ;

ret_t calling_animation_start()
{
    if (false ==  is_call_start )
    {
        widget_t *widget =  widget_lookup( window_manager_get_top_window(window_manager()), "call_aniamtor", TRUE);
        if (widget)
        {
            widget_set_visible(widget , true);
            widget_start_animator(widget , "call_animation");
            widget_start_animator(widget , "calling_tips");
            is_call_start = true ;
        }
        

    }
    return RET_OK ;
}

ret_t calling_animation_stop()
{
    if (is_call_start)
    {
        widget_stop_animator(NULL , "calling_tips");
        widget_t *widget =  widget_lookup( window_manager_get_top_window(window_manager()), "call_aniamtor", TRUE);
        if (widget)
        {
            widget_set_visible(widget , false);
            is_call_start = false ;
        }

    }
    return RET_OK ;

}

// static const char* hfp_state[3] = {} ;
void refresh_pop_call_state(int state)
{
    if (state < 0 || state > FIRST_OUTGOING_SECOND_HELD)
        return ;

    //char buff[256] = {0};
    const char * tr_txt = NULL ;
    switch (state)
    {
        case INCOMING_CALL :
            tr_txt = locale_info_tr(locale_info(), "call_pop_incoming");
            break;
        case OUTGOING_CALL :
            tr_txt = locale_info_tr(locale_info(), "call_pop_output");
            break;
        case ACTIVE_CALL :
            tr_txt = locale_info_tr(locale_info(), "call_pop_answing");
            break;
        default:
            break;
    }
    
    widget_t *widget =  widget_lookup( window_manager_get_top_window(window_manager()), "call_pop_tips", TRUE);
    if (widget && tr_txt)
        widget_set_text_utf8(widget , tr_txt) ;
    
        return ;
    
}