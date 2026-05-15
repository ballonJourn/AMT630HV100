#include "navigation_view.h"
#include "../../../3rd/awtk-widget-qr/src/qr/qr.h"

#define ICON_NAVI_DEFAULT  "icon_nav_1"

const char* home_navi_widget_name[NAVI_NUM_MAX] = {
    "navi_qr" , "label_road" , "navi_image" , "label_distance"
} ;

static widget_t* home_navi_widget[NAVI_NUM_MAX] = { NULL };

static widget_t* nav_slide_view =  NULL ;

//static home_navigate_view_e navigate_view = QR_VIEW ;

ret_t home_nav_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < NAVI_NUM_MAX; i++){
        home_navi_widget[i] = widget_lookup(parent, home_navi_widget_name[i], TRUE);
    }

    nav_slide_view = widget_lookup(parent,"nav_slide_view", TRUE);

    return RET_OK ;
}

ret_t home_refresh_qr(char *value)
{
    if (home_navi_widget[NAVI_QR])
    {
        qr_set_value(home_navi_widget[NAVI_QR], value);
    }

    return RET_OK ;
}

ret_t home_refresh_nav_road(char *value)
{
    if (home_navi_widget[NAVI_LABEL_ROAD])
    {
        widget_set_text_utf8(home_navi_widget[NAVI_LABEL_ROAD], value);
    }

    return RET_OK ;
}

ret_t home_refresh_nav_image(char *value)
{
    if (home_navi_widget[NAVI_IMAGE])
    {
        image_set_image(home_navi_widget[NAVI_IMAGE], value) ;

        const asset_info_t* asset = assets_manager_ref(assets_manager(), ASSET_TYPE_IMAGE, value );
        if (NULL == asset)
            image_set_image(home_navi_widget[NAVI_IMAGE] , ICON_NAVI_DEFAULT);
        
        widget_invalidate_force(home_navi_widget[NAVI_IMAGE] , NULL);

    }

    return RET_OK ;
}

ret_t home_refresh_nav_distance(char *value)
{
    if (home_navi_widget[NAVI_LABEL_DIST])
    {
        widget_set_text_utf8(home_navi_widget[NAVI_LABEL_DIST], value);
    }

    return RET_OK ;
}


//简易导航页面
ret_t home_refresh_nav_view(home_navigate_view_e view)
{
    if (view >= NVAI_VIEW_MAX  || (NULL == nav_slide_view) )
        return RET_FAIL;
    
    slide_view_set_active_ex(nav_slide_view , view , false);
    
    return RET_OK ; 
}