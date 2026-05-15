#ifndef BRIGHTNESS_H__
#define BRIGHTNESS_H__

#include "awtk.h"
#include "../view_manager.h"


ret_t set_brightness_view_init(widget_t* parent) ;

void brightness_init() ;

void on_brightness_deal_short_key(key_id_e key) ;

void brightness_view_clean_state() ;

void brightness_view_set_focused_item(int focusedIndex) ;

#endif