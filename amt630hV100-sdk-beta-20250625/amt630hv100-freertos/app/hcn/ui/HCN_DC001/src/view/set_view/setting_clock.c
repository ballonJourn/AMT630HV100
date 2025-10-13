#include "setting_clock.h"
#include <stdio.h>

const char* set_clock_widget_name[CLOCK_MAX] = {
    "clock_h_1" , "clock_h_2" , "clock_m_1" , "clock_m_2" 
} ;

static widget_t* set_clock_widget[CLOCK_MAX] = { NULL };

static clock_option_e option = CLOCK_H_1_OPTION ;

ret_t set_clock_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < CLOCK_MAX; i++){
        set_clock_widget[i] = widget_lookup(parent, set_clock_widget_name[i], TRUE);
    }

    return RET_OK ;
}

static void setting_menu_view_deal_set()
{
    printf("on_clock setting_menu_view_deal_set \n") ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    clock_view_clean_state() ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + CLOCK_OPTION_NUM_MAX) % CLOCK_OPTION_NUM_MAX ;
    clock_view_set_focused_item(option) ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % CLOCK_OPTION_NUM_MAX ;
    clock_view_set_focused_item(option) ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void clock_init()
{
    clock_view_set_focused_item(option) ;
    //刷新数据 todo

    return  ;
}

void on_clock_deal_short_key(key_id_e key)
{
    printf("on_clock_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void clock_view_set_focused_item(clock_option_e focusedIndex)
{
    for (size_t i = 0; i < CLOCK_OPTION_NUM_MAX ; i++)
    {
        if (set_clock_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_clock_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_clock_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_clock_widget[i] , NULL)  ;
        }
    }

    return ;
}

void clock_view_clean_state()
{
    for (size_t i = 0; i < CLOCK_OPTION_NUM_MAX ; i++)
    {
        if (set_clock_widget[i])
        {
            widget_set_state(set_clock_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_clock_widget[i] , NULL)  ;
        }
    }
}