#include "link_view_logic.h"
#include "link_view.h"
#include "vehicle_param/vehicle_param.h"
#include "proxy/vehicle_argument.h"
#include "view/link_view/link_view.h"
#include "config/hcn_config.h"
#include "proxy/vehicle_data.h"
#include "proxy/mirror_data.h"
#include "../3rd/awtk-widget-qr/src/qr/qr.h"
#include "logic/hcn_global.h"
#include "view/home_view/speed_view.h"


#define REFRESH_INTERVAL_50_MS   (50)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;

static ret_t timer_refresh_50_ms(const timer_info_t *info) ;

static ret_t on_link_page_changed(void* ctx, event_t* e) ;

static int32_t speed         = 0 ;
static int32_t poewr         = 0 ;
static drv_mode_e drv_mode   = DRV_MODE_MAX ;
static gear_e  gear          = GEAR_MAX ;
static int32_t electriacl    = 0 ;

#if ON_PC_CACLE == 0
extern void clear_rect(float x, float y, float w, float h, float a, float r, float g, float b);
#endif

ret_t on_clear_bg(void *ctx, event_t *e)
{
    (void)ctx ; 
    (void)e   ;

#if ON_PC_CACLE == 0
  clear_rect(0, 0, HCN_LCD_WIDTH, HCN_LCD_HEIGHT, 0, 0, 0, 0);
#endif

  return RET_OK;
}

ret_t link_init(widget_t *win) 
{
    if (win == NULL) return RET_FAIL ;

    link_view_init(win) ;

    widget_on(win , EVT_BEFORE_PAINT     , on_clear_bg, win );
    widget_on(win , EVT_WINDOW_CLOSE     , on_link_page_changed ,win);
    widget_on(win , EVT_WINDOW_WILL_OPEN , on_link_page_changed ,win);
    link_timer_init() ;

    return RET_OK ;
}


void link_timer_init()
{
    timer_array[REFRESH_TIMER_50_MS]  = timer_add( timer_refresh_50_ms ,  NULL , REFRESH_INTERVAL_50_MS ) ;

    return ;
}


static ret_t on_link_page_changed(void* ctx, event_t* e)
{
    
    if (e->type == EVT_WINDOW_CLOSE)
    {
        printf("on_link_page_changed EVT_WINDOW_CLOSE\n") ;
        if(timer_array[REFRESH_TIMER_50_MS] != 0 
            && timer_find(timer_array[REFRESH_TIMER_50_MS]))
        {
            timer_remove(timer_array[REFRESH_TIMER_50_MS]) ;
            timer_array[REFRESH_TIMER_50_MS] = 0 ;
            printf("on_link_page_changed timer_remove successed\n");
        }
        speed         = 0 ;
        poewr         = 0 ;
        drv_mode      = DRV_MODE_MAX ;
        gear          = GEAR_MAX ;
        electriacl    = 0 ;

        rest_data();

        /* link_page关闭，恢复home_page上的驾驶模式图标 */
        home_drv_mode_set_visible(TRUE) ;
    }
    else if(e->type == EVT_WINDOW_WILL_OPEN)
    {
        printf("on_link_page_changed EVT_WINDOW_WILL_OPEN\n") ;

        /* home_page未关闭在底下，其driving_mode图标会透过link_page
         * 的半透明背景显示出来，导致E/N/S重叠，进入时隐藏它 */
        home_drv_mode_set_visible(FALSE) ;

        link_refresh_electricalret_unit(vehicle_get_param_unit());

        link_refresh_unit(vehicle_get_param_unit()) ;


        link_refresh_electrical(8);
        link_refresh_electrical(0);
        timer_refresh_50_ms(NULL);

    #if ON_PC_CACLE == 0
        extern int get_qr_text_buf(char *buf, int len) ;
        char buff[256 ] ;
        get_qr_text_buf(buff , sizeof(buff)) ;
        widget_t* qr = widget_lookup((widget_t *)ctx, "link_qr", TRUE);
        if (qr)
            qr_set_value(qr , buff) ;
 
    #endif
    
    }

  return RET_OK ;
}

static ret_t timer_refresh_50_ms(const timer_info_t *info)
{
    (void)info ;

    int __state = vehicle_get_data(VEH_CARLINK_CONNECTED) ;
    link_refresh_qr(!__state) ;


    int32_t _speed = vehicle_get_data_speed();
    if (speed != _speed)
    {
        int32_t temp_value = _speed ;
        if (MPH == vehicle_get_param_unit())
            temp_value  *= KM_CONVERT_MILE ; 

        link_refresh_speed(temp_value) ;

        speed = _speed ;
    }

    int32_t _gear = vehicle_get_data_gear();
    if(gear != _gear)
    {
        link_refresh_gear(_gear);
        gear = _gear ;
    }

    int32_t _poewr = vehicle_get_data_power();
    if(poewr != _poewr)
    {
        link_refresh_power(_poewr);
        poewr = _poewr ;
    }

    int32_t _drv_mode = vehicle_get_data_drv_mode();
    if(drv_mode != _drv_mode)
    {
        link_refresh_drv_mode(_drv_mode);
        drv_mode = _drv_mode ;
    }

    int32_t _electriacl = vehicle_get_data_remain_battary() ;
    if (_electriacl != electriacl)
    {
        link_refresh_electrical(_electriacl);
        electriacl = _electriacl ;
        // printf("link_refresh_electrical\n");
    }
    
    return RET_REPEAT ;
}