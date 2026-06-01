#ifndef SETTING_MENU_H_
#define SETTING_MENU_H_

#include "awtk.h"

typedef enum setting_menu{
    SETTING_MENU_TMPS       ,
    SETTING_MENU_RIDE_ELE   ,
    SETTING_MENU_CONNECT    ,
    SETTING_MENU_LANGUAGE   ,
    SETTING_MENU_BRIGHTNESS ,
    SETTING_MENU_UNIT       ,
    SETTING_MENU_CLOCK      ,
    SETTING_MENU_DISPLAY    ,
    SETTING_MENU_DEVICE     ,

    SETTING_MENU_NUM_MAX    ,
}setting_menu_e;

ret_t setting_menu_view_init(widget_t* parent) ;

void setting_menu_set_focused_item(setting_menu_e item) ;

void setting_menu_clean_state();
#endif