#include "setting_menu.h"
#include "view/home_view/common.h"


const char* setting_menu_name[SETTING_MENU_NUM_MAX] = {
    "tmps" , "ride_ele" , "connect" , "language" , 
    "brightness", "unit" , "clock"   , "device"
} ;

static widget_t* setting_menu_widget[SETTING_MENU_NUM_MAX] = { NULL };
static widget_t* scroll_widget = NULL ;

ret_t setting_menu_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++){
        setting_menu_widget[i] = widget_lookup(parent, setting_menu_name[i], TRUE);
    }
    
    scroll_widget = widget_lookup(parent, "scroll_menu" , TRUE);

    return RET_OK ;
}

void setting_menu_init()
{
    setting_menu_set_focused_item(SETTING_MENU_TMPS) ;
}

// static int minValue = 0 ;
// static int maxValue = 200 ;

static int prevIndex = SETTING_MENU_TMPS ;
static int endIndex  = SETTING_MENU_BRIGHTNESS ;

void setting_menu_set_focused_item(setting_menu_e item)
{
    int currentIndex = 0 ;
    value_t v ;

    printf("scroll bar parent:\n");
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
        
        printf("%s = %d ",setting_menu_widget[i]->name ,setting_menu_widget[i]->y);
    }

#if 0 
    printf("\n");

    point_t point ;
    printf("widget_to_global:\n");
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        widget_to_global(setting_menu_widget[i] , &point);
        printf(" %s = %d ",setting_menu_widget[i]->name ,point.y);
    }
    printf("\n");
    
    printf("widget_to_local :\n");
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        widget_to_local (setting_menu_widget[i] , &point);
        printf("%s = %d ",setting_menu_widget[i]->name ,point.y);
    }
    printf("\n");

    printf("widget_to_screen :\n");
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        widget_to_screen  (setting_menu_widget[i] , &point);
        printf("%s = %d ",setting_menu_widget[i]->name ,point.y);
    }
    printf("\n");

     
    printf("widget_to_screen_ex :\n");
    for (size_t i = 0; i < SETTING_MENU_NUM_MAX; i++)
    {
        widget_to_screen_ex  (setting_menu_widget[i] ,win ,&point);
        printf("%s = %d ",setting_menu_widget[i]->name ,point.y);
    }
    printf("\n");
    printf("currentIndex = %d  item = %d \n" ,currentIndex ,item) ;
#endif 

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
    
#if 0

    if (setting_menu_widget[item]->y < minValue || setting_menu_widget[item]->y > maxValue )
    {
        int offset = 0 ; 
        if (item > currentIndex)
        {   
            offset = (setting_menu_widget[item]->y - setting_menu_widget[currentIndex]->y )  ;
            offset = offset > 150 ? 150 : offset ;
        }
        else
        {
            offset = (setting_menu_widget[item]->y - setting_menu_widget[currentIndex]->y )  ;
            offset = offset < -150 ? -150 : offset ;
        }

        minValue += offset ; maxValue += offset ;
        if(scroll_widget)
        {
            scroll_view_t* scroll_view = SCROLL_VIEW(scroll_widget);
            scroll_view->xoffset_end = scroll_view->xoffset ;
            scroll_view->yoffset_end = scroll_view->yoffset + offset;

            scroll_view_set_offset(scroll_widget, scroll_view->xoffset_end, scroll_view->yoffset_end);
        }

    }
#endif 
    
    
}