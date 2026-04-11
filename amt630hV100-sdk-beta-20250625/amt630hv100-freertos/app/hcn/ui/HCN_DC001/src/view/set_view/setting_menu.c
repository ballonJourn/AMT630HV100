#include "setting_menu.h"
#include "view/home_view/common.h"


const char* setting_menu_name[SETTING_MENU_NUM_MAX] = {
    "tmps" , "ride_ele" , "connect" , "language" , 
    "brightness", "unit" , "clock"  , "display"  , "device"
} ;

static widget_t* setting_menu_widget[SETTING_MENU_NUM_MAX] = { NULL };
static widget_t* scroll_widget = NULL ;
static widget_t* slider_setting_menu  = NULL ;

ret_t setting_menu_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++){
        setting_menu_widget[i] = widget_lookup(parent, setting_menu_name[i], TRUE);
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
            scroll_view->yoffset_end = scroll_view->yoffset + offset * 50;

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
