#include "language.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"
#include "logic/hcn_global.h"

const char* set_language_widget_name[LANGUAGE_NUM_MAX] = {
    "chinese_option" , "english_option" 
} ;

static widget_t* set_language_widget[LANGUAGE_NUM_MAX] = { NULL };

static language_option_e option = LANGUAGE_CHINESE_OPTION ;

ret_t set_language_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < LANGUAGE_NUM_MAX; i++){
        set_language_widget[i] = widget_lookup(parent, set_language_widget_name[i], TRUE);
    }

    return RET_OK ;
}

static void setting_menu_view_deal_set()
{
    printf("language setting_menu_view_deal_set \n") ;

    vehicle_set_param_language(option);
    global_refresh_language(option);
    return  ;
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    language_view_clean_state() ;

    return  ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + LANGUAGE_OPTION_MAX) % LANGUAGE_OPTION_MAX ;
    language_view_set_focused_item(option) ;

    return  ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % LANGUAGE_OPTION_MAX ;
    language_view_set_focused_item(option) ;

    return  ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void language_init()
{
    ///< 语言 0:中文 1:英文,预留位
    uint8_t value = vehicle_get_param_language();

    language_view_set_focused_item(value) ;
    //刷新数据 todo

    return  ;
}

void on_language_deal_short_key(key_id_e key)
{
    printf("on_bt_conn_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void language_view_set_focused_item(language_option_e focusedIndex)
{
    if (focusedIndex > LANGUAGE_ENGLISH_OPTION)
    {
        focusedIndex = LANGUAGE_CHINESE_OPTION ;
    }
    

    for (size_t i = 0; i < LANGUAGE_OPTION_MAX ; i++)
    {
        if (set_language_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_language_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_language_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_language_widget[i] , NULL)  ;
        }
        else{
            printf(" language_view_set_focused_item not find widget \n");
            return ;
        }
    }

    option = focusedIndex ;
    
    return ;
}

void language_view_clean_state()
{
    for (size_t i = 0; i < LANGUAGE_OPTION_MAX ; i++)
    {
        if (set_language_widget[i])
        {
            widget_set_state(set_language_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_language_widget[i] , NULL)  ;
        }
    }

    return ;
}
