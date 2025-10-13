#include "set_page_key.h"
#include "setting_menu.h"
#include "../view_manager.h"
#include "view/set_view/set_view_interface.h"

static setting_menu_e menu_index = SETTING_MENU_TMPS ;

// static menu_list_e current_list_item = MENU_SET_TMPS ;  //应该丢到设置页面当中


typedef struct {
    void (*view_init)();
    void (*deal_short_key)(key_id_e);
}setting_entry_t;


static setting_entry_t setting_entry[SETTING_MENU_NUM_MAX] = {
    [SETTING_MENU_RIDE_ELE] = { cycling_engrgy_init  , on_cycling_engrgy_deal_short_key } ,
    [SETTING_MENU_CLOCK]    = { clock_init           , on_clock_deal_short_key          } ,
};


void set_page_deal_key_down ()
{
    int level = get_current_levle() ;
    switch (level)
    {
        case MENU_LEVEL_0: break;
        case MENU_LEVEL_1:  
            menu_index = ( menu_index + 1 ) % SETTING_MENU_NUM_MAX ;
            setting_menu_set_focused_item(menu_index) ;
            break;
        case MENU_LEVEL_2: 
            if (setting_entry[menu_index].deal_short_key)
                setting_entry[menu_index].deal_short_key(KEY_SHORT_DOWN);
            break;
        case MENU_LEVEL_3: 

            break;
        default:
            break;
    }
    
}


void set_page_deal_key_up   ()
{
    int level = get_current_levle() ;
    switch (level)
    {
        case MENU_LEVEL_0: break;
        case MENU_LEVEL_1:  
            menu_index = ( menu_index - 1 + SETTING_MENU_NUM_MAX) % SETTING_MENU_NUM_MAX ;
            setting_menu_set_focused_item( menu_index ) ;
            break;
        case MENU_LEVEL_2: 
            if (setting_entry[menu_index].deal_short_key)
                setting_entry[menu_index].deal_short_key(KEY_SHORT_UP);
            
            break;
        case MENU_LEVEL_3: 

            break;
        default:
            break;
    }

}


void set_page_deal_key_set  ()
{
    int level = get_current_levle() ;
    if (level == MENU_LEVEL_1)
    {
        if (setting_entry[menu_index].view_init){
        setting_entry[menu_index].view_init();
        }else{
            printf ("setting_entry deal_short_key == NULL   \n ") ;
            return ;
        }
        set_current_level( level + 1 ) ;
    }
    else if(level == MENU_LEVEL_2){
        if (setting_entry[menu_index].view_init){
            setting_entry[menu_index].deal_short_key(KEY_SHORT_SET);
        }else{
            printf ("setting_entry deal_short_key == NULL   \n ") ;
            return ;
        }
    }
    
}


void set_page_deal_key_back ()
{
    int level = get_current_levle() ;
    if (level == MENU_LEVEL_1)
    {
        set_current_level( level - 1 ) ;
        setting_menu_clean_state();
        // menu_index = SETTING_MENU_TMPS ;
    }else if(level == MENU_LEVEL_2)
    {
        if (setting_entry[menu_index].deal_short_key){
            setting_entry[menu_index].deal_short_key(KEY_SHORT_BACK);
        }else{
            printf ("setting_entry deal_short_key KEY_SHORT_BACK == NULL   \n ") ;
            return ;
        }
    }
    
}


int get_current_menu_index()
{
    return menu_index ;
}