#ifndef SETTING_MENU_H_
#define SETTING_MENU_H_

#include "awtk.h"
#include <stdbool.h>

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
    SETTING_MENU_RADAR      ,
    SETTING_MENU_CARLINK    ,

    SETTING_MENU_NUM_MAX    ,
}setting_menu_e;

ret_t setting_menu_view_init(widget_t* parent) ;

void setting_menu_set_focused_item(setting_menu_e item) ;

void setting_menu_clean_state();

///< 菜单项是否启用(可见且可导航)。宏关闭时互联选型项返回 false。
bool setting_menu_item_is_enabled(setting_menu_e item) ;

///< 环形取下一个启用项(跳过禁用项)。
setting_menu_e setting_menu_get_next_item(setting_menu_e cur) ;

///< 环形取上一个启用项(跳过禁用项)。
setting_menu_e setting_menu_get_prev_item(setting_menu_e cur) ;
#endif