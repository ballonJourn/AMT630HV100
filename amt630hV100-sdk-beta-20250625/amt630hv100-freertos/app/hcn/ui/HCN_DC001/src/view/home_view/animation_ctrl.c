#include "animation_ctrl.h"
#include "dock_view.h"
const char* home_move_animation_name[MVOE_NUM_MAX] = {
    "speed_view" , "power_view" , "dock_slider_view" 
} ;

static widget_t* home_animation_widget[MVOE_NUM_MAX] = { NULL };

static char* animation_name[MVOE_NUM_MAX][ANIMATION_TYPE_MAX] = {
    {"move_speed_out" , "move_speed_in"} ,
    {"move_power_out" , "move_power_in"} ,
    {"move_dock_out"  , "move_dock_in" } ,
};


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
        printf("animation start\n") ;
    }
    else if (e->type == EVT_ANIM_END)
    {
        printf("animation end\n") ;
        if (home_animation_widget[DOCK_SLIDER_VIEW])
        {
            widget_set_style_str(home_animation_widget[DOCK_SLIDER_VIEW] , STYLE_ID_BG_IMAGE , "left_bg_p") ;
            slide_view_set_active_ex(home_animation_widget[DOCK_SLIDER_VIEW] , ICON_MUSIC_EX , FALSE ) ;
            widget_invalidate_force(home_animation_widget[DOCK_SLIDER_VIEW] , NULL);
        }
        
    }
    
    return RET_OK ;
}


ret_t animation_listen_in(void* ctx, event_t* e) 
{
    (void)ctx ;
    if (e->type == EVT_ANIM_START)
    {
        if (home_animation_widget[DOCK_SLIDER_VIEW])
        {
            widget_set_style_str(home_animation_widget[DOCK_SLIDER_VIEW] , STYLE_ID_BG_IMAGE , "left_bg_n") ;
            slide_view_set_active_ex(home_animation_widget[DOCK_SLIDER_VIEW] , ICON_MUSIC , FALSE ) ;
            widget_invalidate_force(home_animation_widget[DOCK_SLIDER_VIEW] , NULL);
        }
        
    }
    else if (e->type == EVT_ANIM_END)
    {
        printf("animation end\n") ;
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


// ret_t set_dock_view(dock_view_e dock_view)
// {
//      if (home_animation_widget[DOCK_SLIDER_VIEW])
//         {
//             slide_view_set_active_ex(home_animation_widget[DOCK_SLIDER_VIEW] , dock_view , FALSE ) ;
//         }

//     return RET_OK;
// }

