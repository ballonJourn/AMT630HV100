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
#include "proxy/vehicle_time.h"
#include "proxy/vehicle_mile.h"


#define REFRESH_INTERVAL_50_MS   (50)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;

static ret_t timer_refresh_50_ms(const timer_info_t *info) ;

static ret_t on_link_page_changed(void* ctx, event_t* e) ;

static int32_t speed         = 0 ;
static int32_t poewr         = 0 ;
static drv_mode_e drv_mode   = DRV_MODE_MAX ;
static gear_e  gear          = GEAR_MAX ;
static int32_t electriacl    = 0 ;

/* Top bar state cache */
static int32_t  lk_clock_hour  = -1 ;
static int32_t  lk_clock_min   = -1 ;
static bool_t   lk_clock_colon = TRUE ;
static uint8_t  lk_last_time_fmt = 0xFF ;

/* Bottom bar state cache */
static uint32_t lk_last_trip = 0xFFFFFFFF ;
static uint32_t lk_last_odo  = 0xFFFFFFFF ;
static int32_t  lk_last_bot_elec = -1 ;

/* Link-page alpha-clear API (implemented in main_awtk.c) */
#if ON_PC_CACLE == 0
extern void link_api_set_preview_enable(uint8_t enable);
extern uint8_t link_api_get_preview_enable(void);
#endif

/* ── Triple-buffer full-repaint counter ──
 *
 * The VG driver uses dirty-rect-copy: each frame, "clean" areas are
 * copied from a previous buffer to the current one.  With triple-
 * buffering, it takes 3 consecutive full-screen-dirty frames to ensure
 * every buffer slot has been completely repainted by AWTK with
 * link_page's black background.  If any frame has only a partial dirty
 * rect, dirty-rect-copy fills the rest from an older buffer that may
 * still contain home_page wallpaper → ghost artifact.
 *
 * lk_fullscreen_repaint_cnt is set to 3 on page open.  The 50ms timer
 * decrements it and calls widget_invalidate_force() each tick until 0.
 * Cost: 3 frames × ~16ms = negligible one-time overhead at page entry. */
static uint8_t  lk_fullscreen_repaint_cnt = 0;
static widget_t *lk_win_ref = NULL;

ret_t link_init(widget_t *win) 
{
    if (win == NULL) return RET_FAIL ;

    link_view_init(win) ;
    link_topbar_init(win) ;
    link_bottombar_init(win) ;

    /* No EVT_BEFORE_PAINT / clear_rect — alpha punching is now handled
     * by the post-render hook in main_awtk.c (link_alpha_clear_hook),
     * which runs AFTER AWTK completes rendering, eliminating flicker. */
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


static void link_refresh_topbar(void)
{
    /* Clock: refresh every 500ms equivalent (toggle colon) */
    int hour = vehicle_get_time_hour();
    int min  = vehicle_get_time_min();
    uint8_t time_fmt = vehicle_get_param_time_format();

    /* Force refresh on format change */
    if (lk_last_time_fmt != time_fmt) {
        lk_last_time_fmt = time_fmt;
        lk_clock_hour = -1;
    }

    if (lk_clock_hour != hour || lk_clock_min != min) {
        lk_clock_hour = hour;
        lk_clock_min  = min;
    }

    /* Colon blink driven by 50ms timer counter */
    static uint8_t colon_cnt = 0;
    colon_cnt++;
    if (colon_cnt >= 10) {  /* 500ms toggle */
        colon_cnt = 0;
        lk_clock_colon = !lk_clock_colon;
    }

    link_topbar_refresh_clock(lk_clock_hour, lk_clock_min, lk_clock_colon, lk_last_time_fmt);

    /* Signals */
    link_topbar_refresh_signal(1,  (bool_t)vehicle_get_data_signal_lamp(VEH_GPS));
    link_topbar_refresh_signal(2,  (bool_t)vehicle_get_data_signal_lamp(VEH_BT));
    link_topbar_refresh_signal(3,  (bool_t)vehicle_get_data_signal_lamp(VEH_HIGH_BEAM));
    link_topbar_refresh_signal(4,  (bool_t)vehicle_get_data_signal_lamp(VEH_LEFT));
    link_topbar_refresh_signal(5,  (bool_t)vehicle_get_data_signal_lamp(VEH_READY));
    link_topbar_refresh_signal(6,  (bool_t)vehicle_get_data_signal_lamp(VEH_RIGHT));
    link_topbar_refresh_signal(7,  (bool_t)vehicle_get_data_signal_lamp(VEH_AUTO_BEAM));
    link_topbar_refresh_signal(8,  (bool_t)vehicle_get_data_signal_lamp(VEH_ABS));
    link_topbar_refresh_signal(9,  (bool_t)vehicle_get_data_signal_lamp(VEH_ECU));
    link_topbar_refresh_signal(10, (bool_t)vehicle_get_data_signal_lamp(VEH_TCS));
    link_topbar_refresh_signal(11, (bool_t)vehicle_get_data_signal_lamp(VEH_BRAKE));
#ifdef HCN_MMWAVE_RADAR_ENABLE
    link_topbar_refresh_signal(12, (bool_t)vehicle_get_data(VEH_RADAR_STATUS));
#endif
    /* GMS/BT share visibility */
    link_topbar_refresh_signal(0,  (bool_t)vehicle_get_data_signal_lamp(VEH_BT));
}


static void link_refresh_bottombar(void)
{
    /* Mileage */
    uint32_t trip_val = vehicle_get_mile_tripA();
    uint32_t odo_val  = vehicle_get_mile_odo();

    if (trip_val != lk_last_trip) {
        lk_last_trip = trip_val;
        double trip_d = (double)trip_val;
        if (MPH == vehicle_get_param_unit())
            trip_d *= KM_CONVERT_MILE;
        link_bottombar_refresh_trip(trip_d);
    }

    if (odo_val != lk_last_odo) {
        lk_last_odo = odo_val;
        double odo_d = (double)odo_val;
        if (MPH == vehicle_get_param_unit())
            odo_d *= KM_CONVERT_MILE;
        link_bottombar_refresh_odo(odo_d);
    }

    /* Electrical */
    int32_t bot_elec = vehicle_get_data_remain_battary();
    if (bot_elec != lk_last_bot_elec) {
        lk_last_bot_elec = bot_elec;
        link_bottombar_refresh_electrical(bot_elec);
    }
}


static ret_t on_link_page_changed(void* ctx, event_t* e)
{
    
    if (e->type == EVT_WINDOW_CLOSE)
    {
        printf("on_link_page_changed EVT_WINDOW_CLOSE\n") ;

#if ON_PC_CACLE == 0
        /* Disable alpha-clear hook and restore full-screen VIDEO layout */
        link_api_set_preview_enable(0);
#endif

        if(timer_array[REFRESH_TIMER_50_MS] != 0 
            && timer_find(timer_array[REFRESH_TIMER_50_MS]))
        {
            timer_remove(timer_array[REFRESH_TIMER_50_MS]) ;
            timer_array[REFRESH_TIMER_50_MS] = 0 ;
            printf("on_link_page_changed timer_remove successed\n");
        }
        speed         = 0 ;
        poewr         = 0 ;
        drv_mode      = DRV_MODE_E ;
        gear          = GEAR_MAX ;
        electriacl    = 0 ;

        /* Reset top/bottom bar cache */
        lk_clock_hour = -1 ;
        lk_clock_min  = -1 ;
        lk_last_time_fmt = 0xFF ;
        lk_last_trip = 0xFFFFFFFF ;
        lk_last_odo  = 0xFFFFFFFF ;
        lk_last_bot_elec = -1 ;

        rest_data();

        lk_fullscreen_repaint_cnt = 0;
        lk_win_ref = NULL;
    }
    else if(e->type == EVT_WINDOW_WILL_OPEN)
    {
        printf("on_link_page_changed EVT_WINDOW_WILL_OPEN\n") ;

#if ON_PC_CACLE == 0
        /* Enable alpha-clear hook: punch alpha=0 hole at (0,60,800,480) in
         * each frame AFTER AWTK renders, and position VIDEO layer to match.
         * This is the same architecture DVR uses — zero flicker. */
        link_api_set_preview_enable(1);
#endif

        link_refresh_electricalret_unit(vehicle_get_param_unit());
        link_refresh_unit(vehicle_get_param_unit()) ;

        link_refresh_electrical(8);
        link_refresh_electrical(0);

        /* Init bottom bar units */
        link_bottombar_refresh_mileage_unit(vehicle_get_param_unit());
        link_bottombar_refresh_electrical_unit(vehicle_get_param_unit());

        /* Force immediate refresh of all bars */
        link_refresh_topbar();
        link_refresh_bottombar();

        timer_refresh_50_ms(NULL);

    #if ON_PC_CACLE == 0
        extern int get_qr_text_buf(char *buf, int len) ;
        char buff[256 ] ;
        get_qr_text_buf(buff , sizeof(buff)) ;
        widget_t* qr = widget_lookup((widget_t *)ctx, "link_qr", TRUE);
        if (qr)
            qr_set_value(qr , buff) ;
    #endif

        /* Arm triple-buffer full-repaint: see comment at lk_fullscreen_repaint_cnt.
         * The 50ms timer will call widget_invalidate_force() for the next 3 ticks,
         * ensuring all 3 framebuffer slots are fully painted with link_page content
         * so VG dirty-rect-copy never restores stale home_page wallpaper. */
        lk_win_ref = (widget_t *)ctx;
        lk_fullscreen_repaint_cnt = 3;
        widget_invalidate_force(lk_win_ref, NULL);

    }

  return RET_OK ;
}

static ret_t timer_refresh_50_ms(const timer_info_t *info)
{
    (void)info ;

    /* ── Triple-buffer forced repaint (runs at most 3 times after page open) ──
     * Each tick forces the entire link_page window dirty so AWTK repaints the
     * full 1024×600 area.  After 3 consecutive full-screen frames, all 3
     * framebuffer slots contain link_page content and VG dirty-rect-copy
     * can no longer restore stale home_page wallpaper from any buffer. */
    if (lk_fullscreen_repaint_cnt > 0 && lk_win_ref != NULL) {
        widget_invalidate_force(lk_win_ref, NULL);
        lk_fullscreen_repaint_cnt--;
    }

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

    /* Top bar + bottom bar refresh */
    link_refresh_topbar();
    link_refresh_bottombar();
    
    return RET_REPEAT ;
}