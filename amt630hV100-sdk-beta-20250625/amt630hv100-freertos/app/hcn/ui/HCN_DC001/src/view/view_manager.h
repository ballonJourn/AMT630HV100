#ifndef VIEW_MANAGER
#define VIEW_MANAGER

#include "home_view/dock_view.h"

typedef void (*short_click_deal)();

typedef enum 
{
    KEY_SHORT_UP, 
    KEY_SHORT_DOWN,

    KEY_SHORT_SET, 
    KEY_SHORT_BACK,

}key_id_e ;

typedef enum menu_level{
    MENU_LEVEL_0 ,
    MENU_LEVEL_1 ,
    MENU_LEVEL_2 ,
    MENU_LEVEL_3 , 
}menu_level_e;


typedef enum menu_list_item{
    MENU_SET_TMPS     , 
    MENU_SET_RIDE_ELE ,
    MENU_SET_CONNECT  ,
    MENU_SET_LANGUAGE , 
    MENU_SET_UNIT     ,
    MENU_SET_CLOCK    , 
    MENU_SET_DEVICE   ,
    MENU_SET_NUM_MAX  ,
}menu_list_e;


typedef enum window{
    MAIN_PAGE ,            //有车速页面
    DOCK_SELECT_VIEW ,     //无车速页面
    WINDOWS_NUM_MAX ,
}window_page_e ;

ret_t view_manager_init(widget_t* parent) ;

ret_t set_dock_view(dock_view_e dock_view) ;

ret_t set_window_page(window_page_e type) ;


int get_current_win();

void set_current_win(int cur_dock) ;


int get_current_levle();

void set_current_level(int cur_level);


void deal_key_set_short_press();

void deal_key_back_short_press();

void deal_key_back_long_press();

void deal_key_up_short_press();

void deal_key_down_short_press();

void deal_key_super_long_press();

void set_ready_press_state(bool state ) ;

bool get_ready_press_state();

#endif