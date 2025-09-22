#include "home_page_key.h"
#include "dock_view.h"
#include "../view_manager.h"
#include "animation_ctrl.h"

#if 0
static ret_t on_key_event(void* ctx, event_t* e) {
  if (e->type == EVT_KEY_DOWN) {
    key_event_t* evt = (key_event_t*)e;
    uint32_t key     = evt->key;

    switch (key) {
      case TK_KEY_UP:
        button_manager_event(BUTTON_ID_SHORT_UP);
        break;
      case TK_KEY_DOWN:
        button_manager_event(BUTTON_ID_SHORT_DOWN);
        break;
      case TK_KEY_LEFT:
        button_manager_event(BUTTON_ID_SHORT_BACK);
        break;
      case TK_KEY_RIGHT:
        button_manager_event(BUTTON_ID_SHORT_SET);
        break;
      default:
        button_manager_event(key);
        LOG_WARN("Unhandled key event: %u", key);
        break;
    }
  }
  return RET_OK;
}
widget_on(window, EVT_KEY_DOWN, on_key_event, NULL);
#endif 
     

void home_page_deal_key_set()
{
    int index = get_current_win();
    switch (index)
    {
        case ICON_INFO:
            /* code */
            break;
        case ICON_NAVI:
            //navigator_switch_to("link_page",false);
            break;
        case ICON_MUSIC:
            set_current_level(MENU_LEVEL_1);
            animation_play_out();
            break;
        case ICON_PHONE:
            // if (bt_call_is_connect)
            // {
            // }
            break;
        case ICON_SETTING:
            // slider_view();
            set_current_level(MENU_LEVEL_1);
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
    index = (index + 1 ) % (ICON_SETTING + 1) ;

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



