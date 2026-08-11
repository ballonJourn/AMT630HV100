#include "setting_menu.h"
#include "view/home_view/common.h"
#include "config/hcn_config.h"


const char* setting_menu_name[SETTING_MENU_NUM_MAX] = {
    "tmps" , "ride_ele" , "connect" , "language" , 
    "brightness", "unit" , "clock"  , "display"  , "device" , "radar" , "carlink"
} ;

static widget_t* setting_menu_widget[SETTING_MENU_NUM_MAX] = { NULL };
static widget_t* scroll_widget = NULL ;
static widget_t* slider_setting_menu  = NULL ;

bool setting_menu_item_is_enabled(setting_menu_e item)
{
    if (item < 0 || item >= SETTING_MENU_NUM_MAX)
        return false;

#if !defined(HCN_CARLINK_SWITCH_ENABLE)
    /* 宏关闭:互联选型项不显示、不可导航,但不影响 CarPlay/亿连 互联与按键逻辑 */
    if (item == SETTING_MENU_CARLINK)
        return false;
#endif

    return true;
}

setting_menu_e setting_menu_get_next_item(setting_menu_e cur)
{
    setting_menu_e next = cur;
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++) {
        next = (setting_menu_e)((next + 1) % SETTING_MENU_NUM_MAX);
        if (setting_menu_item_is_enabled(next))
            return next;
    }
    return cur;
}

setting_menu_e setting_menu_get_prev_item(setting_menu_e cur)
{
    setting_menu_e prev = cur;
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++) {
        prev = (setting_menu_e)((prev - 1 + SETTING_MENU_NUM_MAX) % SETTING_MENU_NUM_MAX);
        if (setting_menu_item_is_enabled(prev))
            return prev;
    }
    return cur;
}

ret_t setting_menu_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++){
        setting_menu_widget[i] = widget_lookup(parent, setting_menu_name[i], TRUE);
        if (setting_menu_widget[i] && !setting_menu_item_is_enabled((setting_menu_e)i)) {
            widget_set_visible(setting_menu_widget[i], FALSE);
        }
    }

    scroll_widget = widget_lookup(parent, "scroll_menu" , TRUE);

    slider_setting_menu = widget_lookup(parent, "setting_menu" , TRUE);

    return RET_OK ;
}


static int prevIndex = SETTING_MENU_TMPS ;
static int endIndex  = SETTING_MENU_BRIGHTNESS ;

void setting_menu_set_focused_item(setting_menu_e item)
{
    int currentIndex = 0 ;
    value_t v ;

    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        if (setting_menu_widget[i])
        {
            widget_get_prop(setting_menu_widget[i] ,WIDGET_PROP_STATE_FOR_STYLE , &v) ;

            if(tk_str_cmp(value_str(&v) , STATE_SELECTE) == 0) 
                currentIndex = i ;
        }
        else{
            printf("setting_menu_init not found name \"%s\" widget \n" , setting_menu_name[i]) ;
            return ;
        }
        
    }


    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        if (setting_menu_widget[i]){
            if( i == item )
                widget_set_state(setting_menu_widget[i] , STATE_SELECTE) ;
            else
                widget_set_state(setting_menu_widget[i] , STATE_NORMAL ) ;
        }
    }
    
    if (item < prevIndex || item > endIndex)
    {
        int offset = 0 ;
        if (item > endIndex){
            offset = item - endIndex ;
        }else{
            offset = item - prevIndex ;
        }
        prevIndex += offset ; endIndex += offset ;

        if (scroll_widget)
        {
            scroll_view_t* scroll_view = SCROLL_VIEW(scroll_widget);
            scroll_view->xoffset_end = scroll_view->xoffset ;
            scroll_view->yoffset_end = scroll_view->yoffset + offset * 62;

            scroll_view_set_offset(scroll_widget, scroll_view->xoffset_end, scroll_view->yoffset_end);
        }
        
    }


    if (slider_setting_menu)
    {
        slide_view_set_active_ex(slider_setting_menu , item , false);
    }
    

    return ;
}


void setting_menu_clean_state()
{
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        if (setting_menu_widget[i]){
            widget_set_state(setting_menu_widget[i] , STATE_NORMAL ) ;
        }
    }
    return ;
}