#include "home_page_key.h"
#include "../view_manager.h"
#include "view/set_view/setting_menu.h"
#include "view/set_view/set_page_key.h"
#include "home_view_interface.h"
#include "common/navigator.h"
#include "proxy/bluetooth_data.h"

void home_page_deal_key_set()
{
    int index = get_current_win();
    switch (index)
    {
        case ICON_INFO:
            /* code */
            break;
        case ICON_NAVI: navigator_switch_to(LINK_PAGE , false);
            break;
        case ICON_MUSIC:
            set_current_level(MENU_LEVEL_1);
            animation_play_out();
            music_ex_view_init();
            break;
        case ICON_PHONE:
            if (vehicle_buluetooth_is_calling())
                vehicle_calling_pick_up();
            break;
        case ICON_SETTING:
            set_current_level(MENU_LEVEL_1);
            setting_menu_set_focused_item(get_current_menu_index());
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
        if (vehicle_buluetooth_is_calling())
        {
            vehicle_calling_hung_up();
            calling_animation_stop();
        }
        
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

    //刷新页面
    set_dock_view(index) ;
}

void home_page_deal_key_up   ()
{
    int index =  get_current_win() ;
    index = (index - 1 + ICON_MUSIC_EX ) % (ICON_SETTING + 1) ;

    //刷新页面
    set_dock_view(index) ;
    
}



