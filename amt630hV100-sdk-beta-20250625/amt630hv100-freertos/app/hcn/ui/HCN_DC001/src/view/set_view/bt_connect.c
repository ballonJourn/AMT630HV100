#include "bt_connect.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"
#include "proxy/bluetooth_data.h"

const char* set_bt_conn_widget_name[BT_CONNECT_NUM_MAX] = {
    "bt_on_option" , "bt_off_option" , "bt_state" ,"bt_name" , "bt_phone_info"
} ;

const char* set_bt_conn_state_str[BT_CONNECT_OPTION_MAX] = {
    "setting_bluebooth_open" , "setting_bluebooth_close"
};

static widget_t* set_bt_conn_widget[BT_CONNECT_NUM_MAX] = { NULL };

static bt_option_e option = BT_CONNECT_ON_OPTION ;

ret_t set_bt_connect_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < BT_CONNECT_NUM_MAX; i++){
        set_bt_conn_widget[i] = widget_lookup(parent, set_bt_conn_widget_name[i], TRUE);
    }

    return RET_OK ;

}

static void setting_menu_view_deal_set()
{
    printf("on_bt_conn setting_menu_view_deal_set \n") ;
    if (option == BT_CONNECT_ON_OPTION)
    {
        vehicle_set_param_bluetooth(1);
        vehicle_set_bluetooth_state(1);
    }
    else if(option == BT_CONNECT_OFF_OPTION)
    {
        vehicle_set_param_bluetooth(0);
        vehicle_set_bluetooth_state(0);
    }

    return  ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    bt_connect_view_clean_state() ;

    return  ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + BT_CONNECT_OPTION_MAX) % BT_CONNECT_OPTION_MAX ;
    bt_connect_view_set_focused_item(option) ;

    return  ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % BT_CONNECT_OPTION_MAX ;
    bt_connect_view_set_focused_item(option) ;

    return  ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void bt_connect_init()
{

    uint8_t value = vehicle_get_param_bluetooth();
    bt_connect_view_set_focused_item(value == 0 ? BT_CONNECT_OFF_OPTION : BT_CONNECT_ON_OPTION) ;

    //刷新数据 todo

    return  ;
}

void on_bt_connect_deal_short_key(key_id_e key)
{
    printf("on_bt_conn_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void bt_connect_view_set_focused_item(bt_option_e focusedIndex)
{
    for (size_t i = 0; i < BT_CONNECT_OPTION_MAX ; i++)
    {
        if (set_bt_conn_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_bt_conn_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_bt_conn_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_bt_conn_widget[i] , NULL)  ;
        }
        else{
            printf(" bt_connect_view_set_focused_item not find widget \n");
            return ;
        }
    }

    option = focusedIndex ;

    return ;
}

void bt_connect_view_clean_state()
{
    for (size_t i = 0; i < BT_CONNECT_OPTION_MAX ; i++)
    {
        if (set_bt_conn_widget[i])
        {
            widget_set_state(set_bt_conn_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_bt_conn_widget[i] , NULL)  ;
        }
    }

    return ;
}

void refresh_bt_connect_state(bt_option_e state)
{
    if (state > BT_CONNECT_OFF_OPTION)
        return ;
    
    if (set_bt_conn_widget[BT_CONNECT_STATE])
    {
        image_set_image(set_bt_conn_widget[BT_CONNECT_STATE] , set_bt_conn_state_str[state]) ;
    }
    
    return ;
}

void refresh_bt_name(const char* name)
{
    if (set_bt_conn_widget[BT_CONNECT_BT_NAME])
    {
        widget_set_text_utf8(set_bt_conn_widget[BT_CONNECT_BT_NAME] , name);
    }
    
    return ;
}

void refresh_bt_phone_info(const char* phoneName)
{
    if (set_bt_conn_widget[BT_CONNECT_PHONE_INFO])
    {
        widget_set_text_utf8(set_bt_conn_widget[BT_CONNECT_PHONE_INFO] , phoneName);
    }
    return ;
}