#include "dock_view.h"

const char* home_dock_widget_name[ICON_NUM_MAX] = {
    "icon_info" , "icon_navi" , "icon_music" , "icon_phone" , "icon_setting" , " " , "icon_dvr"
} ;

static widget_t* home_dock_widget[ICON_NUM_MAX] = { NULL };


ret_t home_dock_view_init(widget_t* parent)
{

    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < ICON_NUM_MAX; i++){
        home_dock_widget[i] = widget_lookup(parent, home_dock_widget_name[i], TRUE);
    }

    if (home_dock_widget[ICON_INFO]){
        widget_set_state(home_dock_widget[ICON_INFO] , STATE_SELECTE) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_dock_icon(int index)
{
    index = tk_min(index, ICON_NUM_MAX);
    index = tk_max(index, ICON_INFO);


    for (size_t i = ICON_INFO; i < ICON_NUM_MAX; i++){
        if (home_dock_widget[i]){
            if (index == i)
                widget_set_state(home_dock_widget[i] , STATE_SELECTE);
            else
                widget_set_state(home_dock_widget[i] , STATE_NORMAL) ;
        }
        
    }
    
    return RET_OK ;
}

