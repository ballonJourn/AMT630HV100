#include "unit.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"
#include "logic/hcn_global.h"

const char* set_unit_widget_name[UNIT_NUM_MAX] = {
    "unit_km_option" , "unit_mile_option" 
} ;

static widget_t* set_unit_widget[UNIT_NUM_MAX] = { NULL };

static unit_option_e option = UNIT_KM_OPTION ;

ret_t set_unit_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < UNIT_NUM_MAX; i++){
        set_unit_widget[i] = widget_lookup(parent, set_unit_widget_name[i], TRUE);
    }

    return RET_OK ;
}

static void setting_menu_view_deal_set()
{
    printf("unit setting_menu_view_deal_set \n") ;
    vehicle_set_param_unit(option);
    global_refresh_unit((uint8_t)option);
    return  ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    unit_view_clean_state() ;

    return  ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + UNIT_OPTION_MAX) % UNIT_OPTION_MAX ;
    unit_view_set_focused_item(option) ;

    return  ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % UNIT_OPTION_MAX ;
    unit_view_set_focused_item(option) ;

    return  ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void unit_init()
{
    unit_view_set_focused_item(vehicle_get_param_unit()) ;
    //刷新数据 todo
    return  ;
}

void on_unit_deal_short_key(key_id_e key)
{
    printf("on_bt_conn_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void unit_view_set_focused_item(unit_option_e focusedIndex)
{
    if (focusedIndex > UNIT_MILE_OPTION)
    {
        focusedIndex = UNIT_KM_OPTION ;
    }
    
    for (size_t i = 0; i < UNIT_OPTION_MAX ; i++)
    {
        if (set_unit_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_unit_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_unit_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_unit_widget[i] , NULL)  ;
        }
        else{
            printf(" unit_view_set_focused_item not find widget \n");
            return ;
        }
    }

    option = focusedIndex ;

    return ;
}

void unit_view_clean_state()
{
    for (size_t i = 0; i < UNIT_OPTION_MAX ; i++)
    {
        if (set_unit_widget[i])
        {
            widget_set_state(set_unit_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_unit_widget[i] , NULL)  ;
        }
    }

    return ;
}
