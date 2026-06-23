#include "radar_switch.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"
#include "logic/hcn_global.h"

const char* set_radar_widget_name[RADAR_OPTION_MAX] = {
    "radar_on_option" , "radar_off_option"
} ;

static widget_t* set_radar_widget[RADAR_OPTION_MAX] = { NULL };

static radar_option_e option = RADAR_ON_OPTION ;

ret_t set_radar_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < RADAR_OPTION_MAX; i++){
        set_radar_widget[i] = widget_lookup(parent, set_radar_widget_name[i], TRUE);
    }

    return RET_OK ;
}

static void radar_view_deal_set()
{
    printf("radar setting_menu_view_deal_set = %d\n" , option) ;

    if (option == RADAR_ON_OPTION) {
        vehicle_set_param_radar(1);
    } else {
        vehicle_set_param_radar(0);
    }

    return ;
}

static void radar_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    radar_view_clean_state() ;

    return ;
}

static void radar_view_deal_up()
{
    option = (option - 1 + RADAR_OPTION_MAX) % RADAR_OPTION_MAX ;
    radar_view_set_focused_item(option) ;

    return ;
}

static void radar_view_deal_down()
{
    option = (option + 1) % RADAR_OPTION_MAX ;
    radar_view_set_focused_item(option) ;

    return ;
}

static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = radar_view_deal_up  ,
    [KEY_SHORT_DOWN] = radar_view_deal_down,
    [KEY_SHORT_SET]  = radar_view_deal_set ,
    [KEY_SHORT_BACK] = radar_view_deal_back,
};

void radar_switch_init()
{
    uint8_t value = vehicle_get_param_radar();
    if (value)
        option = RADAR_ON_OPTION ;
    else
        option = RADAR_OFF_OPTION ;

    radar_view_set_focused_item(option) ;

    return ;
}

void on_radar_switch_deal_short_key(key_id_e key)
{
    printf("on_radar_switch_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return ;
}

void radar_view_set_focused_item(radar_option_e focusedIndex)
{
    for (size_t i = 0; i < RADAR_OPTION_MAX ; i++)
    {
        if (set_radar_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_radar_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_radar_widget[i], STATE_NORMAL ) ;

            widget_invalidate_force(set_radar_widget[i] , NULL)  ;
        }
        else{
            printf(" radar_view_set_focused_item not find widget \n");
            return ;
        }
    }

    option = focusedIndex ;
    
    return ;
}

void radar_view_clean_state()
{
    for (size_t i = 0; i < RADAR_OPTION_MAX ; i++)
    {
        if (set_radar_widget[i])
        {
            widget_set_state(set_radar_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_radar_widget[i] , NULL)  ;
        }
    }

    return ;
}
