#ifndef BT_CONNECT__H_
#define BT_CONNECT__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_bt_connect_com{
    BT_CONNECT_ON         ,
    BT_CONNECT_OFF        ,
    BT_CONNECT_STATE      ,

    BT_CONNECT_BT_NAME    ,
    BT_CONNECT_PHONE_INFO ,
    BT_CONNECT_NUM_MAX    ,
};

typedef enum {
    BT_CONNECT_ON_OPTION   ,
    BT_CONNECT_OFF_OPTION  ,

    BT_CONNECT_OPTION_MAX  ,
}bt_option_e ;


ret_t set_bt_connect_view_init(widget_t* parent) ;

void refresh_bt_connect_state(bt_option_e state) ;

void refresh_bt_name(const char* name);

void refresh_bt_phone_info(const char* phoneName);

void bt_connect_init() ;

void on_bt_connect_deal_short_key(key_id_e key) ;

void bt_connect_view_clean_state() ;

void bt_connect_view_set_focused_item(bt_option_e focusedIndex) ;

#endif