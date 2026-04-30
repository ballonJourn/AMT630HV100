#include "brightness.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"

static widget_t* set_brightness_scroll_widget =  NULL ;

static int  option = 0 ;
static int  count  = 0 ;

ret_t set_brightness_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;

    set_brightness_scroll_widget = widget_lookup(parent, "brightness_scroll" , TRUE);

    if (set_brightness_scroll_widget)
        count = widget_count_children(set_brightness_scroll_widget) ;
    else
        printf("set_brightness_scroll_widget faild\n" ) ;
    
    return RET_OK ;
}

static void setting_menu_view_deal_set()
{
    printf("brightness setting_menu_view_deal_set \n") ;
    vehicle_set_param_brightness(option);
    return  ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    brightness_view_clean_state()  ;

    return  ;
}

static void setting_menu_view_deal_up()
{
    if (0 == count)
        return ;
    
    option =  (option - 1 + count) % count ;
    brightness_view_set_focused_item(option) ;

    return  ;
}

static void setting_menu_view_deal_down()
{
    if (0 == count)
        return ;

    option =  (option + 1 ) % count ;
    brightness_view_set_focused_item(option) ;

    return  ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void brightness_init()
{
    brightness_view_set_focused_item(vehicle_get_param_brightness()) ;

    //刷新数据 todo
    return  ;
}


void on_brightness_deal_short_key(key_id_e key)
{
    printf("on_bt_conn_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

int prevIndex = 0 ;
int endIndex  = 2 ;

void brightness_view_set_focused_item(int focusedIndex)
{
    if (set_brightness_scroll_widget == NULL ) 
        return ;

    if (focusedIndex < 0 || focusedIndex > count) 
        focusedIndex = 0 ;

    widget_t *children = NULL ;

    for (size_t i = 0; i < count ; i++)
    {
        children = widget_get_child(set_brightness_scroll_widget , i) ;
        if (children)
        {
            if (i == focusedIndex)
                widget_set_state(children, STATE_SELECTE) ;
            else
                widget_set_state(children, STATE_NORMAL ) ;

            widget_invalidate_force(children , NULL)  ;
        }
        else{
            printf(" brightness_view_set_focused_item not find widget \n");
            return ;
        }
    }

    option = focusedIndex ;

    if (focusedIndex < prevIndex || focusedIndex > endIndex)
    {
        int offset = 0 ;
        if (focusedIndex > endIndex){
            offset = focusedIndex - endIndex ;
        }else{
            offset = focusedIndex - prevIndex ;
        }
        prevIndex += offset ; endIndex += offset ;

        if (set_brightness_scroll_widget)
        {
            scroll_view_t* scroll_view = SCROLL_VIEW(set_brightness_scroll_widget);
            scroll_view->xoffset_end = scroll_view->xoffset ;
            scroll_view->yoffset_end = scroll_view->yoffset + offset * 60;

            scroll_view_set_offset(set_brightness_scroll_widget, scroll_view->xoffset_end, scroll_view->yoffset_end);
        }
        
    }

    return ;
}

void brightness_view_clean_state()
{
    if (set_brightness_scroll_widget == NULL ) return ;

    widget_t *children = NULL ;

    for (size_t i = 0; i < count ; i++)
    {
        children = widget_get_child(set_brightness_scroll_widget , i) ;
        if (children)
        {
            widget_set_state(children, STATE_NORMAL ) ;
        }
    }

    return ;
}
