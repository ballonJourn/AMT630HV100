#include <stdio.h>
#include <stdlib.h>
#include "home_view/music_page_key.h"
#include "home_view/home_page_key.h"
#include "set_view/set_page_key.h"
#include "home_view/dock_view.h"
#include "proxy/vehicle_data.h"
#include "common/navigator.h"
#include "view_manager.h"

#if !ON_PC_CACLE
#include "key_module/hcn_key_common.h"
#endif

static dock_view_e current_dock = ICON_INFO ;     //

static menu_level_e current_level = MENU_LEVEL_0 ;   //初始状态为0级别 进去music 、setting为二级  setting进入选项为三级 时间调整为四级

// static menu_list_e current_list_item = MENU_SET_TMPS ;  //应该丢到设置页面当中

const char* window_name_str[WINDOWS_NUM_MAX] = {
    "pages" , "dock_slider_view" 
} ;

static widget_t* window_page[WINDOWS_NUM_MAX] = { NULL };


#define HCN_KEY_DISPATCH(key_type) do {                                               \
    if ((current_level) == (MENU_LEVEL_0)) {                                          \
        home_page_deal_key_##key_type();                                              \
    } else if (((current_level) == (MENU_LEVEL_1)) && ((current_dock) == (ICON_MUSIC))) { \
        music_page_deal_key_##key_type();                                             \
    } else {                                                                          \
        set_page_deal_key_##key_type();                                               \
    }                                                                                 \
} while (0);;                                                                        



static ret_t on_key_event(void* ctx, event_t* e) {

    if (e->type == EVT_KEY_DOWN) {
    key_event_t* evt = (key_event_t*)e;
    uint32_t key     = evt->key;
    
    // printf("on_key_event key event: %d\n", key);
    switch (key) {
        case TK_KEY_w:
            deal_key_up_short_press() ;
            break;
        case TK_KEY_s:
            deal_key_down_short_press();
            break;
        case TK_KEY_a:
            deal_key_back_short_press();
            break;
        case TK_KEY_d:
            deal_key_set_short_press() ;
            break;
        default:
            printf("Unhandled key event: %u", key);
            break;
        }
    }   
    return RET_OK;
}

#if !ON_PC_CACLE
static void hcn_key_cb(uint8_t id) 
{
    printf( "set_key_cb key = %d \n", id) ;
    switch (id)
    {
    case  SET_KEY_LONG_PR :
        navigator_switch_to(LINK_PAGE , false)   ;
        break;
    case  BACK_KEY_SHORT_PR :
        navigator_back_to_home( )   ;
        break;
    default:
        break;
    }

}
#endif


ret_t view_manager_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < WINDOWS_NUM_MAX; i++){
        window_page[i] = widget_lookup(parent, window_name_str[i], TRUE);
    }
    
    widget_on( window_manager(), EVT_KEY_DOWN, on_key_event, NULL);

#if !ON_PC_CACLE
    set_key_event_cb(hcn_key_cb);
#endif

    return RET_OK ;
}


ret_t set_dock_view(dock_view_e dock_view)
{
    if (window_page[DOCK_SELECT_VIEW])
    {
        slide_view_set_active_ex(window_page[DOCK_SELECT_VIEW] , dock_view , FALSE ) ;
    }

    home_refresh_dock_icon(dock_view) ;

    set_window_page( dock_view == ICON_SETTING );

    return RET_OK;
}

ret_t set_window_page(window_page_e type)
{
    if (window_page[MAIN_PAGE])
    {
        pages_t *page = PAGES(window_page[MAIN_PAGE]) ;

        if(page->active != type )
            pages_set_active(window_page[MAIN_PAGE] , type);
    }

    return RET_OK;
}


int get_current_win(){

    return  current_dock ;
}

void set_current_win(int cur_dock){

    current_dock = cur_dock ;
}


int get_current_levle(){

    return  current_level ;
}

void set_current_level(int cur_level){

    current_level = cur_level ;
}


void deal_key_set_short_press()
{
    HCN_KEY_DISPATCH(set) ;
}

void deal_key_back_short_press()
{
    HCN_KEY_DISPATCH(back) ;
}

void deal_key_up_short_press()
{
    HCN_KEY_DISPATCH(up) ;
}

void deal_key_down_short_press()
{
   HCN_KEY_DISPATCH(down) ;

   return ;
}





