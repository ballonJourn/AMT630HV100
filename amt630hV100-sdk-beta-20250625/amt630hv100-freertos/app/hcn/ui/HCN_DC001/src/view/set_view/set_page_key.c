#include "set_page_key.h"
#include "setting_menu.h"
#include "../view_manager.h"
#include "view/set_view/set_view_interface.h"

static setting_menu_e menu_index = SETTING_MENU_TMPS ;

typedef struct {
    void (*view_init)();
    void (*deal_short_key)(key_id_e);
}setting_entry_t;


static setting_entry_t setting_entry[SETTING_MENU_NUM_MAX] = {
    [SETTING_MENU_RIDE_ELE]   = { cycling_engrgy_init  , on_cycling_engrgy_deal_short_key } ,
    [SETTING_MENU_CLOCK]      = { clock_init           , on_clock_deal_short_key          } ,
    [SETTING_MENU_CONNECT]    = { bt_connect_init      , on_bt_connect_deal_short_key     } ,
    [SETTING_MENU_LANGUAGE]   = { language_init        , on_language_deal_short_key       } ,
    [SETTING_MENU_UNIT]       = { unit_init            , on_unit_deal_short_key           } , 
    [SETTING_MENU_DISPLAY]    = { display_init         , on_display_deal_short_key        } ,
    [SETTING_MENU_BRIGHTNESS] = { brightness_init      , on_brightness_deal_short_key     } ,
};

static setting_entry_t option_entry[SETTING_MENU_NUM_MAX] = {
    [SETTING_MENU_CLOCK]      = { clock_option_init    , on_clock_option_deal_short_key   } ,
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
            if (option_entry[menu_index].deal_short_key)
                option_entry[menu_index].deal_short_key(KEY_SHORT_DOWN);
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
            if (option_entry[menu_index].deal_short_key)
                option_entry[menu_index].deal_short_key(KEY_SHORT_UP);
            break;
        default:
            break;
    }

}


void set_page_deal_key_set  ()
{
    int level = get_current_levle() ;
    switch (level)
    {
        case MENU_LEVEL_0: break;
        case MENU_LEVEL_1:  
            if (setting_entry[menu_index].view_init){
                setting_entry[menu_index].view_init();
                set_current_level( level + 1 ) ;
            }
            break;
        case MENU_LEVEL_2: 
            if (setting_entry[menu_index].deal_short_key){
                setting_entry[menu_index].deal_short_key(KEY_SHORT_SET);
            }
            break;
        case MENU_LEVEL_3: 
            if (option_entry[menu_index].deal_short_key)
                option_entry[menu_index].deal_short_key(KEY_SHORT_SET);
            break;
        default:
            break;
    }
    
}


void set_page_deal_key_back ()
{
    int level = get_current_levle() ;
    switch (level)
    {
        case MENU_LEVEL_0: break;
        case MENU_LEVEL_1:  
            set_current_level( level - 1 ) ;
            setting_menu_clean_state();
            break;
        case MENU_LEVEL_2: 
            if (setting_entry[menu_index].deal_short_key){
                setting_entry[menu_index].deal_short_key(KEY_SHORT_BACK);
            }
            
            break;
        case MENU_LEVEL_3: 
            if (option_entry[menu_index].deal_short_key)
                option_entry[menu_index].deal_short_key(KEY_SHORT_BACK);
            break;
        default:
            break;
    }

}


int get_current_menu_index()
{
    return menu_index ;
}