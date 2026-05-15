#ifndef HOME_DOCK_VIEW_H
#define HOME_DOCK_VIEW_H
#include "common.h"

typedef enum home_dock_com{
    ICON_INFO    ,
    ICON_NAVI    ,
    ICON_MUSIC   ,
    ICON_PHONE   ,
    ICON_SETTING ,

    ICON_MUSIC_EX,
    ICON_NUM_MAX ,
}dock_view_e;


ret_t home_dock_view_init(widget_t* parent) ;

ret_t home_refresh_dock_icon(int power) ;

#endif