#include <stdio.h>  
#include <stdlib.h>
#include "awtk.h"
#include "hcn_logic.h"
#include "view/view_manager.h"
#include "hcn_selfcheck.h"
#include "speed_view_logic.h"

#define REFRESH_INTERVAL_10_MS (10)
#define REFRESH_INTERVAL_50_MS (50)
#define REFRESH_INTERVAL_500_MS (500)
#define REFRESH_INTERVAL_1000_MS (1000)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;

ret_t add_timer_init()
{
    timer_array[REFRESH_TIMER_50_MS]  = timer_add( timer_refresh_50_ms ,  NULL , REFRESH_INTERVAL_50_MS ) ;
    timer_array[REFRESH_TIMER_500_MS] = timer_add( timer_refresh_500_ms , NULL , REFRESH_INTERVAL_500_MS) ;

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

    // 设置语言

    // 设置时间 

    // 设置亮度

    // 设置里程程息 、 剩余里程 、 档位 、驾驶模式

    // 自检
    selfcheck_init();

    //添加定时器
    add_timer_init();

    return RET_OK ;
}

ret_t timer_refresh_500_ms(const timer_info_t *info)
{
    //时间刷新闪烁

    return RET_REPEAT ;
}

ret_t timer_refresh_50_ms(const timer_info_t *info)
{
    (void)info ;

    //数据刷新

    speed_view_update() ;

    // electrical_view_update()

    return RET_REPEAT ;
}