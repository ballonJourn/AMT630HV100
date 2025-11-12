#include <stdio.h>  
#include <stdlib.h>
#include "awtk.h"
#include "hcn_logic.h"
#include "view/view_manager.h"
#include "hcn_selfcheck.h"
#include "speed_view_logic.h"
#include "signal_view_logic.h"
#include "navigation_view_logic.h"
#include "buletooth_logic.h"
#include "hcn_global.h"
#include "view/set_view/set_view_interface.h"
#include "view/home_view/home_view_interface.h"
#include "proxy/vehicle_data.h"
#include "proxy/vehicle_time.h"
#include "proxy/vehicle_mile.h"
#include "mileage_calc.h"

#define REFRESH_INTERVAL_50_MS   (50)
#define REFRESH_INTERVAL_100_MS  (200)
#define REFRESH_INTERVAL_500_MS  (500)
#define REFRESH_INTERVAL_1000_MS (1000)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;

uint64_t time_start = 0 ;

ret_t set_view_init(widget_t * win)
{
    if(win == NULL) return RET_FAIL ;

    setting_menu_view_init      (win) ;

    set_cycling_energy_view_init(win) ;
    set_clock_view_init         (win) ;
    set_bt_connect_view_init    (win) ;
    set_language_view_init      (win) ;
    set_unit_view_init          (win) ;
    set_display_view_init       (win) ;
    set_brightness_view_init    (win) ;
    set_device_view_init        (win) ;
    return RET_OK ;
}

ret_t home_view_init(widget_t * win)
{
    if(win == NULL) return RET_FAIL ;

    home_animation_init   (win) ; 
    home_clock_view_init  (win) ;  
    home_dock_view_init   (win) ; 
    home_elec_view_init   (win) ; 
    home_mileage_view_init(win) ;    
    home_power_view_init  (win) ; 
    home_speed_view_init  (win) ; 
    home_signal_view_init (win) ;       
    view_manager_init     (win) ;  
    
    home_dock_music_ex_view_init(win) ;
    home_nav_view_init          (win) ;
    home_dock_music_view_init   (win) ;
    home_phone_view_init        (win) ;
    home_info_view_init         (win) ;

    // 自检
    selfcheck_init();

    mileage_calc_init();

    //添加定时器
    home_timer_init();

    time_start =  time_now_s() ;

    return RET_OK ;
}

ret_t home_timer_init()
{
    timer_array[REFRESH_TIMER_50_MS]  = timer_add( timer_refresh_50_ms ,  NULL , REFRESH_INTERVAL_50_MS ) ;
    timer_array[REFRESH_TIMER_100_MS] = timer_add( global_data_init    ,  NULL , REFRESH_INTERVAL_100_MS) ;
    timer_array[REFRESH_TIMER_500_MS] = timer_add( timer_refresh_500_ms , NULL , REFRESH_INTERVAL_500_MS) ;

    return RET_OK ;
}


ret_t timer_refresh_500_ms(const timer_info_t *info)
{
    //时间刷新闪烁
    (void)info ;
    
    static int  clock_min   = 0 ;
    static int  clock_sec   = 0 ;
    static bool clock_colon = TRUE ;

    int min = vehicle_get_time_hour();
    if (clock_min != min)
    {
        clock_min = min;
        home_refresh_clock_min(min) ;
    }
    
    int sec = vehicle_get_time_min();
    if (clock_sec != sec)
    {
        clock_sec = sec ;
        home_refresh_clock_sec(sec) ;
    }
    
    clock_colon = !clock_colon ;
    home_refresh_clock_colon(clock_colon) ;
    
    //导航
    navigation_view_update();

    //小窗口时间
    uint64_t interval  = time_now_s() - time_start;
    home_refresh_info_time(interval) ;

    return RET_REPEAT ;
}


ret_t timer_refresh_50_ms(const timer_info_t *info)
{
    (void)info ;
    
    if (checkself_get_state() != CHECK_STATE_FINISHED || get_demonstration_state() ) 
        return RET_REPEAT ;
    
    
    if(get_mileage_state())
    {
        global_refresh_mileage();
        set_mileage_state(false);
    }

    bluetooth_view_update();

    //数据刷新
    speed_view_update() ;

    signal_view_update();

    // electrical_view_update()

    return RET_REPEAT ;
}


uint32_t get_timer_ID(timer_type_e timerID)
{
    if(timerID < REFRESH_TIMER_NUM_MAX)
        return timer_array[timerID] ;
    
    return RET_OK ;
}

void clean_timer_ID(timer_type_e timerID)
{
    if(timerID < REFRESH_TIMER_NUM_MAX)
    {
        if (timer_array[timerID] != 0 && timer_find(timer_array[timerID]) )
        {
           timer_remove(timer_array[timerID]) ;
        }
    }

    return ;
}