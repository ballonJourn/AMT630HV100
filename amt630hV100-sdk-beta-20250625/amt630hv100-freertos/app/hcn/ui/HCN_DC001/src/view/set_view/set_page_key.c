#include "set_page_key.h"
#include "setting_menu.h"

static setting_menu_e menu_index = SETTING_MENU_TMPS ;

void set_page_deal_key_down ()
{
    menu_index = ( menu_index + 1 ) % SETTING_MENU_NUM_MAX ;
    setting_menu_set_focused_item(menu_index) ;
}


void set_page_deal_key_up   ()
{
    menu_index = ( menu_index - 1 + SETTING_MENU_NUM_MAX) % SETTING_MENU_NUM_MAX ;
    setting_menu_set_focused_item(menu_index) ;
    
}


void set_page_deal_key_set  ()
{

}


void set_page_deal_key_back ()
{

}