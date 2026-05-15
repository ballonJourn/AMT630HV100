#include "setting_clock.h"
#include <stdio.h>
#include "proxy/vehicle_time.h"

const char* set_clock_widget_name[CLOCK_MAX] = {
    "clock_h_1" , "clock_h_2" , "clock_m_1" , "clock_m_2" 
} ;

char time_value[CLOCK_MAX] = { 0 } ;

static widget_t* set_clock_widget[CLOCK_MAX] = { NULL };

static clock_option_e option = CLOCK_H_1_OPTION ;

ret_t set_clock_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < CLOCK_MAX; i++){
        set_clock_widget[i] = widget_lookup(parent, set_clock_widget_name[i], TRUE);
    }

    return RET_OK ;
}


void get_label_clock(int32_t *min , int32_t *sec)
{
    if (min == NULL || sec == NULL) return ;

    int32_t h_1 = 0, h_2 = 0, m_1 = 0 , m_2 = 0;

    if(set_clock_widget[CLOCK_H_1_OPTION]){
       h_1 = widget_get_value_int(set_clock_widget[CLOCK_H_1_OPTION]);
    
    }
    if(set_clock_widget[CLOCK_H_2_OPTION]){
       h_2 = widget_get_value_int(set_clock_widget[CLOCK_H_2_OPTION]);

    }
    if(set_clock_widget[CLOCK_M_1_OPTION]){
       m_1 = widget_get_value_int(set_clock_widget[CLOCK_M_1_OPTION]);

    }
    if(set_clock_widget[CLOCK_M_2_OPTION]){
       m_2 = widget_get_value_int(set_clock_widget[CLOCK_M_2_OPTION]);
    }

    *min = h_1 * 10 + h_2;
    *sec = m_1 * 10 + m_2;   

    return ;
}

void refresh_clock(int min ,int sec)
{
    if(set_clock_widget[CLOCK_H_1_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_H_1_OPTION], min / 10);
    
    }
    if(set_clock_widget[CLOCK_H_2_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_H_2_OPTION], min % 10);

    }
    if(set_clock_widget[CLOCK_M_1_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_M_1_OPTION], sec / 10);

    }
    if(set_clock_widget[CLOCK_M_2_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_M_2_OPTION], sec % 10);
    }

    return ;
}

int32_t option_current_value = 0 ;  //记录设置时的时间

static void setting_menu_view_deal_set()
{
    // printf("on_clock setting_menu_view_deal_set \n") ;
    set_current_level(MENU_LEVEL_3);
    clock_option_init();
    if (set_clock_widget[option])
    {
        option_current_value = widget_get_value_int(set_clock_widget[option] ) ;
    }
    
}

static void setting_menu_view_deal_back()
{
    set_current_level(MENU_LEVEL_1);
    clock_view_clean_state() ;
}

static void setting_menu_view_deal_up()
{
    option =  (option - 1 + CLOCK_OPTION_NUM_MAX) % CLOCK_OPTION_NUM_MAX ;
    clock_view_set_focused_item(option) ;
}

static void setting_menu_view_deal_down()
{
    option =  (option + 1 ) % CLOCK_OPTION_NUM_MAX ;
    clock_view_set_focused_item(option) ;
}


static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = setting_menu_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_menu_view_deal_down,
    [KEY_SHORT_SET]  = setting_menu_view_deal_set ,
    [KEY_SHORT_BACK] = setting_menu_view_deal_back,
};


void clock_init()
{
    clock_view_set_focused_item(option) ;
    //刷新数据 todo

    return  ;
}

void on_clock_deal_short_key(key_id_e key)
{
    printf("on_clock_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal) 
        && short_click[key]) {
        short_click[key]();
    }

    return  ;
}

void clock_view_set_focused_item(clock_option_e focusedIndex)
{
    for (size_t i = 0; i < CLOCK_OPTION_NUM_MAX ; i++)
    {
        if (set_clock_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_clock_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_clock_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_clock_widget[i] , NULL)  ;
        }
    }

    return ;
}

void clock_view_clean_state()
{
    for (size_t i = 0; i < CLOCK_OPTION_NUM_MAX ; i++)
    {
        if (set_clock_widget[i])
        {
            widget_set_state(set_clock_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_clock_widget[i] , NULL)  ;
        }
    }
}

/// @brief  三级页面功能
static uint32_t timer_clock_Id = 0 ;

void clock_ctrl_end()
{
    if (timer_clock_Id != 0 && timer_find(timer_clock_Id))
    {
        timer_remove(timer_clock_Id) ;
        timer_clock_Id = 0 ;
    }
    
    if (set_clock_widget[option])
        widget_set_visible(set_clock_widget[option] , true);
    
}

void clock_cacle(int direc)
{
    uint32_t option_time = widget_get_value_int(set_clock_widget[option]);
    uint32_t temp_time   = 0;
    int offset = direc > 0 ? 1 : -1 ;
    
    switch (option)
    {
        case CLOCK_H_1_OPTION:
            temp_time = widget_get_value_int(set_clock_widget[CLOCK_H_2_OPTION]);
            if (temp_time > 3 )
                option_time = (option_time + offset + 2) % 2 ;
            else
                option_time = (option_time + offset + 3) % 3 ;

            break;
        case CLOCK_H_2_OPTION:
            temp_time = widget_get_value_int(set_clock_widget[CLOCK_H_1_OPTION]);
            if (temp_time == 2 )
                option_time = (option_time + offset + 4) % 4 ;
            else
                option_time = (option_time + offset + 10) % 10 ;

            break;
        case CLOCK_M_1_OPTION:
            option_time = (option_time + offset + 6) % 6 ;

            break;
        case CLOCK_M_2_OPTION:
            option_time = (option_time + offset + 10) % 10 ;
        
            break;
        
        default:
            break;
    }

    widget_set_value_int(set_clock_widget[option] ,option_time) ;
}

static void setting_option_view_deal_set()
{
    set_current_level(MENU_LEVEL_2);
 
    clock_ctrl_end();

    //to do  set systerm time
    int32_t min = 0, sec = 0 ;
    get_label_clock(&min , &sec);
    vehicle_set_time(min, sec);
    return ;
}

static void setting_option_view_deal_back()
{
    set_current_level(MENU_LEVEL_2);
    
    clock_ctrl_end();

    if (set_clock_widget[option])
    {
        widget_set_value_int(set_clock_widget[option] , option_current_value) ;
    }

    return ;
}

static void setting_option_view_deal_up()
{
    clock_cacle(-1) ;
    return ;

}

static void setting_option_view_deal_down()
{
    clock_cacle(1) ;
    return ;
}


static short_click_deal option_click[] = {
    [KEY_SHORT_UP]   = setting_option_view_deal_up  ,
    [KEY_SHORT_DOWN] = setting_option_view_deal_down,
    [KEY_SHORT_SET]  = setting_option_view_deal_set ,
    [KEY_SHORT_BACK] = setting_option_view_deal_back,
};

ret_t on_timer_flicker(const timer_info_t* timer)
{
    (void)timer ;
    if (set_clock_widget[option])
        widget_set_visible(set_clock_widget[option] , !set_clock_widget[option]->visible) ;
    
    return RET_REPEAT ;
}


void clock_option_init()
{
    timer_clock_Id = timer_add(on_timer_flicker , NULL , 500) ;
    return  ;
}

void on_clock_option_deal_short_key(key_id_e key)
{
    printf("on_clock_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(option_click) / sizeof(short_click_deal) 
        && option_click[key]) {
        option_click[key]();
    }

    return  ;
}
