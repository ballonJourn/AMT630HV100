#include "cycling_energy.h"
#include <stdio.h>
#include <stdlib.h>

const char* set_energy_widget_name[RIDE_NUM_MAX] = {
    "ride_5km" , "ride_20km" , "last_elec" ,"curr_elec" , "avg_elec" ,
    "line_series" 
} ;

static uint32_t update_timer = 0 ;

static widget_t* set_energy_widget[RIDE_NUM_MAX] = { NULL };

static widget_t* chart_view = NULL ;

static cycling_engrgy_option_e option = RIDE_5KM_OPTION ;

static cycling_engrgy_option_e clicked_option = RIDE_5KM_OPTION ;

ret_t set_cycling_energy_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < RIDE_NUM_MAX; i++){
        set_energy_widget[i] = widget_lookup(parent, set_energy_widget_name[i], TRUE);
    }

    chart_view = widget_lookup(parent, "chartview", TRUE);

    return RET_OK ;
}

static void setting_menu_view_deal_set()
{
    clicked_option = option ;
    printf("on_cycling_engrgy setting_menu_view_deal_set \n") ;

    return  ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    cycling_engergy_view_clean_state() ;

    if (update_timer != 0 && timer_find(update_timer))
    {
        timer_remove(update_timer) ;
        update_timer = 0 ;
    }
    
    return  ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + RIDE_OPTION_MAX) % RIDE_OPTION_MAX ;
    cycling_engergy_view_set_focused_item(option) ;

    return  ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % RIDE_OPTION_MAX ;
    cycling_engergy_view_set_focused_item(option) ;

    return  ;
}

static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


ret_t on_update_chart(const timer_info_t* timer)
{
    (void)timer ;
    float_t value = (float_t)(rand() % 121 - 30.0f);
    series_push(set_energy_widget[RIDE_LINE_SERIES] , &value , 1);

    if (chart_view)
        widget_invalidate_force( chart_view , NULL ) ;

    return RET_REPEAT;
}

void cycling_engrgy_init()
{
    cycling_engergy_view_set_focused_item(clicked_option) ;
    option =  clicked_option ;
    
    //刷新数据 todo
    if (update_timer != 0 && timer_find(update_timer))
        return ;
    
    update_timer = timer_add(on_update_chart ,NULL , 2000) ;
    
    return  ;
}

void on_cycling_engrgy_deal_short_key(key_id_e key)
{
    printf("on_cycling_engrgy_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void cycling_engergy_view_set_focused_item(cycling_engrgy_option_e focusedIndex)
{
    for (size_t i = 0; i < RIDE_OPTION_MAX ; i++)
    {
        if (set_energy_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_energy_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_energy_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_energy_widget[i] , NULL)  ;
        }
        else{
            printf(" cycling_engergy_view_set_focused_item not find widget \n");
        }
    }

    return ;
}

void cycling_engergy_view_clean_state()
{
    for (size_t i = 0; i < RIDE_OPTION_MAX ; i++)
    {
        if (set_energy_widget[i])
        {
            widget_set_state(set_energy_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_energy_widget[i] , NULL)  ;
        }
    }

    return ;
}