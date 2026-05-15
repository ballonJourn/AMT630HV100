#ifndef NAVIGATION_VIEW_H__
#define NAVIGATION_VIEW_H__

#include "awtk.h"
#include "view/view_manager.h"

enum home_navigation_com{
    NAVI_QR         ,
    NAVI_LABEL_ROAD ,
    NAVI_IMAGE      , 
    NAVI_LABEL_DIST , 

    NAVI_NUM_MAX    ,
};

typedef enum {
    QR_VIEW       ,
    TIPS_VIEW     ,
    NAVI_VIEW     ,

    NVAI_VIEW_MAX ,
}home_navigate_view_e ;


ret_t home_nav_view_init(widget_t* parent) ;

ret_t home_refresh_qr(char *value);

ret_t home_refresh_nav_road(char *value);

ret_t home_refresh_nav_image(char *value);

ret_t home_refresh_nav_distance(char *value);

ret_t home_refresh_nav_view(home_navigate_view_e view)  ;



#endif