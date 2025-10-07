#include "home_page_key.h"
#include "../view_manager.h"
#include "view/set_view/setting_menu.h"
#include "home_view_interface.h"
#include "common/navigator.h"

void home_page_deal_key_set()
{
    int index = get_current_win();
    switch (index)
    {
        case ICON_INFO:
            /* code */
            break;
        case ICON_NAVI:
            navigator_switch_to(LINK_PAGE , false);
            break;
        case ICON_MUSIC:
            set_current_level(MENU_LEVEL_1);
            animation_play_out();
            music_ex_view_init();
            break;
        case ICON_PHONE:
            // if (bt_call_is_connect)
            // {
            // }
            break;
        case ICON_SETTING:
            set_current_level(MENU_LEVEL_1);
            setting_menu_init();
            // set_focused_item();
            break;
        default:
            break;
    }
}

void home_page_deal_key_back()
{
    int index = get_current_win();
    switch (index)
    {
    case ICON_INFO:
        /* code */
        break;
    case ICON_NAVI:
        break;
    case ICON_MUSIC:
        
        break;
    case ICON_PHONE:
        // if (bt_call_is_connect)
        // {
        //挂断
        // }
        break;
    case ICON_SETTING:
        break;
    default:
        break;
    }
    
}

void home_page_deal_key_down ()
{
    int index =  get_current_win() ;
    index = (index + 1) % (ICON_SETTING + 1) ;

    //设置页面参数
    set_current_win(index) ;
    
    //刷新页面
    set_dock_view(index) ;
}

void home_page_deal_key_up   ()
{
    int index =  get_current_win() ;
    index = (index - 1 + ICON_MUSIC_EX ) % (ICON_SETTING + 1) ;

    //设置页面参数
    set_current_win(index) ;
    
    //刷新页面
    set_dock_view(index) ;
    
}



