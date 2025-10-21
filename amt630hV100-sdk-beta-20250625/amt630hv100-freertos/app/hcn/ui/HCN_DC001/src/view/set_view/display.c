#include "display.h"
#include <stdio.h>

const char* set_display_widget_name[DISPLAY_NUM_MAX] = {
    "display_auto_option" , "display_day_option" , "display_night_option"
} ;

static widget_t* set_display_widget[DISPLAY_NUM_MAX] = { NULL };

static display_option_e option = DIAPLAY_AUTO_OPTION ;

ret_t set_display_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < DISPLAY_NUM_MAX; i++){
        set_display_widget[i] = widget_lookup(parent, set_display_widget_name[i], TRUE);
    }

    return RET_OK ;
}

static void setting_menu_view_deal_set()
{
    printf("display setting_menu_view_deal_set \n") ;

    return  ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    display_view_clean_state() ;

    return  ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + DIAPLAY_OPTION_MAX) % DIAPLAY_OPTION_MAX ;
    display_view_set_focused_item(option) ;

    return  ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % DIAPLAY_OPTION_MAX ;
    display_view_set_focused_item(option) ;

    return  ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void display_init()
{
    display_view_set_focused_item(option) ;
    //刷新数据 todo

    return  ;
}

void on_display_deal_short_key(key_id_e key)
{
    printf("on_bt_conn_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void display_view_set_focused_item(display_option_e focusedIndex)
{
    for (size_t i = 0; i < DIAPLAY_OPTION_MAX ; i++)
    {
        if (set_display_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_display_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_display_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_display_widget[i] , NULL)  ;
        }
        else{
            printf(" display_view_set_focused_item not find widget \n");
        }
    }

    return ;
}

void display_view_clean_state()
{
    for (size_t i = 0; i < DIAPLAY_OPTION_MAX ; i++)
    {
        if (set_display_widget[i])
        {
            widget_set_state(set_display_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_display_widget[i] , NULL)  ;
        }
    }

    return ;
}
