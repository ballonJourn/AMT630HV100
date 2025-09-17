#ifndef HOME_DOCK_VIEW_H
#define HOME_DOCK_VIEW_H
#include "common.h"

enum home_dock_com{
    ICON_INFO    ,
    ICON_NAVI    ,
    ICON_MUSIC   ,
    ICON_PHONE   ,
    ICON_SETTING ,
    ICON_NUM_MAX ,
};


ret_t home_dock_view_init(widget_t* parent) ;

ret_t home_refresh_dock_item(int power) ;

#endif