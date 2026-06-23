#ifndef RADAR_SWITCH__H_
#define RADAR_SWITCH__H_

#include "awtk.h"
#include "../view_manager.h"

typedef enum {
    RADAR_ON_OPTION   ,
    RADAR_OFF_OPTION  ,
    RADAR_OPTION_MAX  ,
}radar_option_e ;

ret_t set_radar_view_init(widget_t* parent) ;

void radar_switch_init() ;

void on_radar_switch_deal_short_key(key_id_e key) ;

void radar_view_clean_state() ;

void radar_view_set_focused_item(radar_option_e focusedIndex) ;

#endif
