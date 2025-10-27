#include <stdio.h>  
#include <stdlib.h>
#include "awtk.h"
#include "hcn_logic.h"
#include "view/view_manager.h"
#include "hcn_selfcheck.h"
#include "speed_view_logic.h"
#include "signal_view_logic.h"
#include "view/set_view/set_view_interface.h"
#include "proxy/vehicle_data.h"
#include "navigation_view_logic.h"

#define REFRESH_INTERVAL_33_MS   (33)
#define REFRESH_INTERVAL_50_MS   (50)
#define REFRESH_INTERVAL_500_MS  (500)
#define REFRESH_INTERVAL_1000_MS (1000)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;


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
    
    //sliderview
    home_dock_music_ex_view_init(win);
    home_nav_view_init    (win) ;
    
    // 设置语言

    // 设置时间 

    // 设置亮度

    // 设置里程程息 、 剩余里程 、 档位 、驾驶模式

    // 自检
    selfcheck_init();

    //添加定时器
    home_timer_init();

    return RET_OK ;
}

ret_t home_timer_init()
{
    timer_array[REFRESH_TIMER_50_MS]  = timer_add( timer_refresh_50_ms ,  NULL , REFRESH_INTERVAL_50_MS ) ;
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

    home_refresh_clock_min(clock_min) ;

    home_refresh_clock_sec(clock_sec) ;

    clock_colon = !clock_colon ;
    home_refresh_clock_colon(clock_colon) ;
    

    navigation_view_update();

    return RET_REPEAT ;
}


ret_t timer_refresh_50_ms(const timer_info_t *info)
{
    (void)info ;
    
    if (checkself_get_state() != CHECK_STATE_FINISHED || get_demonstration_state() ) 
        return RET_REPEAT ;
    

    //数据刷新
    speed_view_update() ;

    // signal_view_update();

    // electrical_view_update()

    return RET_REPEAT ;
}