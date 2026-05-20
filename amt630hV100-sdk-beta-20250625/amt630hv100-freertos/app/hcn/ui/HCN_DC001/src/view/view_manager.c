/**
 * @file view_manager.c
 * @brief 视图管理器 — LVGL 版本
 *
 * M029: 从 AWTK 迁移
 * 主要变更:
 *   1. HCN_KEY_DISPATCH 宏: top_win_->name → screen_mgr_get_top_name()
 *   2. on_key_event: 不再通过 widget_on 注册到 window_manager
 *      改为由 hcn_key_handle 直接分发 (物理按键是唯一输入源)
 *   3. view_manager_init: widget_lookup → widget_reg_find
 *   4. idle_queue: 已通过 timer_compat 兼容, 无需修改
 *   5. set_dock_view/set_window_page: slide_view/pages 控件操作
 *      暂用兼容层桩函数, M024 XML转写后实现
 */

#include <stdio.h>
#include <stdlib.h>
#include "set_view/set_page_key.h"
#include "home_view/home_view_interface.h"
#include "set_view/set_view_interface.h"
#include "device_view/device_logic.h"
#include "proxy/vehicle_data.h"
#include "common/navigator.h"
#include "view_manager.h"
#include "link_view/link_page_key.h"
#include "proxy/vehicle_time.h"
#include "proxy/bluetooth_data.h"
#include "logic/hcn_selfcheck.h"
#include "logic/mileage_calc.h"
#include "lvgl_compat/screen_manager.h"
#include "lvgl_compat/widget_name_shim.h"

#if !ON_PC_CACLE
#include "key_module/hcn_key_common.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#endif

#define OTA_PAGE_INTERVAL 35

static dock_view_e current_dock = ICON_INFO ;

static menu_level_e current_level = MENU_LEVEL_0 ;

static bool ready_press = true ;

const char* window_name_str[WINDOWS_NUM_MAX] = {
    "pages" , "dock_slider_view"
} ;

static widget_t* window_page[WINDOWS_NUM_MAX] = { NULL };

/**
 * LVGL 版 HCN_KEY_DISPATCH — 使用 screen_mgr_get_top_name()
 * 替代 AWTK 的 top_win_->name 结构体访问
 */
#define HCN_KEY_DISPATCH(keyType)  do { \
    const char* _top_name = screen_mgr_get_top_name(); \
    if (_top_name == NULL) return; \
    if (strcmp(_top_name, HOME_PAGE) == 0) { \
        if ((current_level) == (MENU_LEVEL_0)) { \
            home_page_deal_key_##keyType(); \
        } else if (((current_level) == (MENU_LEVEL_1)) && ((current_dock) == (ICON_MUSIC))) { \
            music_page_deal_key_##keyType(); \
        } else { \
            set_page_deal_key_##keyType(); \
        } \
    } else if (strcmp(_top_name, LINK_PAGE) == 0) { \
        link_page_deal_key_##keyType(); \
    } else if (strcmp(_top_name, DEVICE_PAGE) == 0) { \
        device_page_deal_key_##keyType(); \
    } \
} while(0)


/**
 * on_key_event — 调试/模拟按键处理
 * 在 AWTK 中通过 widget_on(window_manager(), EVT_KEY_DOWN, ...) 注册
 * 在 LVGL 中, 物理按键通过 hcn_key_cb → idle_queue → hcn_key_handle 处理
 * 此函数保留用于 PC 模拟/调试
 */
static ret_t on_key_event(void* ctx, event_t* e)
{
    (void)ctx;
    key_event_t* evt = (key_event_t*)e;
    uint32_t key     = evt->key;

    if (e->type == EVT_KEY_DOWN){
        switch (key)
        {
            case TK_KEY_w:
                deal_key_up_short_press() ;
                break;
            case TK_KEY_s:
                deal_key_down_short_press();
                break;
            case TK_KEY_a:
                deal_key_back_short_press();
                break;
            case TK_KEY_d:
                deal_key_set_short_press() ;
                break;
            default:
                printf("Unhandled key event: %u", key);
                break;
        }
    }
    else if (e->type == EVT_KEY_LONG_PRESS){
        if (key == TK_KEY_d)
        {
            get_demonstration_state() ? demonstration_stop() : demonstration_start();
        }
        else if(key == TK_KEY_w)
        {
            navigator_switch_to(DEVICE_PAGE , false) ;
        }
    }
    return RET_OK;
}

#if !ON_PC_CACLE
static void hcn_key_handle(uint8_t id)
{
    printf( "set_key_cb key = %d \n", id) ;

    if (!ready_press || checkself_get_state() != CHECK_STATE_FINISHED )
        return ;

    switch (id)
    {
        case  MODE_KEY_LONG_PR   :
                get_demonstration_state() ? demonstration_stop() : demonstration_start();
                break;

        case  BACK_KEY_SHORT_PR :
                deal_key_back_short_press() ;
                break;

        case  BACK_KEY_LONG_PR:
                deal_key_back_long_press();
                break;

        case  SET_KEY_SHORT_PR  :
                deal_key_set_short_press()  ;
                break;

        case  UP_KEY_SHORT_PR   :
                deal_key_up_short_press()   ;
                break;

        case  MODE_KEY_SHORT_PR :
                deal_key_down_short_press() ;
                break;

        case  SET_KEY_SUPER_LONG_PR :
                deal_key_super_long_press() ;
                break;
        default:
                break;
    }

    return ;
}

ret_t on_idle_queue(const idle_info_t* idle)
{
    if (NULL == idle)
        return RET_REMOVE;

    uint8_t key = (uint8_t)(uintptr_t)idle->ctx ;

    hcn_key_handle(key) ;

    return RET_REMOVE;
}

static void hcn_key_cb(uint8_t id)
{
    idle_queue(on_idle_queue ,(void*)(uintptr_t)id) ;
    return ;
}
#endif

static uint64_t start_time = 0 ;

ret_t view_manager_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;

    /* widget_lookup 现在走注册表, 需要 M024 注册 "pages" / "dock_slider_view" */
    for (size_t i = 0; i < WINDOWS_NUM_MAX; i++){
        window_page[i] = widget_lookup(parent, window_name_str[i], TRUE);
    }

    /**
     * AWTK: widget_on(window_manager(), EVT_KEY_DOWN, on_key_event, NULL)
     * LVGL: 物理按键通过 hcn_key_cb → idle_queue 处理, 不需要注册到 window_manager
     * on_key_event 仅用于 PC 模拟调试
     */
    /* widget_on(window_manager(), EVT_KEY_DOWN,       on_key_event, NULL); */
    /* widget_on(window_manager(), EVT_KEY_LONG_PRESS, on_key_event, NULL); */
    (void)on_key_event; /* suppress unused warning */

#if !ON_PC_CACLE
    set_key_event_cb(hcn_key_cb);
#endif

    start_time = time_now_s();

    return RET_OK ;
}

void update_page_info()
{
    int min = vehicle_get_time_hour();
    int sec = vehicle_get_time_min();
    refresh_clock(min , sec);
}


ret_t set_dock_view(dock_view_e dock_view)
{
    if (window_page[DOCK_SELECT_VIEW])
    {
        slide_view_set_active_ex(window_page[DOCK_SELECT_VIEW] , dock_view , FALSE ) ;
    }

    set_current_win(dock_view) ;

    home_refresh_dock_icon(dock_view) ;

    set_window_page( dock_view == ICON_SETTING );

    return RET_OK;
}

ret_t set_window_page(window_page_e type)
{
    if (window_page[MAIN_PAGE])
    {
        /* pages_t / PAGES 宏暂用兼容层桩, M024 布局转写后实现 */
        pages_set_active(window_page[MAIN_PAGE] , type);
    }

    return RET_OK;
}


int get_current_win(){
    return  current_dock ;
}

void set_current_win(int cur_dock){
    current_dock = cur_dock ;
}


int get_current_levle(){
    return  current_level ;
}


void set_current_level(int cur_level){
    current_level = cur_level ;
}


void deal_key_set_short_press()
{
    if (vehicle_buluetooth_is_calling())
        vehicle_calling_pick_up();
    else
        HCN_KEY_DISPATCH(set);

    return ;
}


void deal_key_back_short_press()
{
    if (vehicle_buluetooth_is_calling())
        vehicle_calling_hung_up();
    else
        HCN_KEY_DISPATCH(back);

    return ;
}

void deal_key_back_long_press()
{
    if (ICON_INFO == get_current_win()
        && MENU_LEVEL_0 == get_current_levle())
    {
#if !ON_PC_CACLE
        mileage_clear_trip();
#endif
        printf("mileage_clear_trip \n");
    }
}

void deal_key_up_short_press()
{
    HCN_KEY_DISPATCH(up);
    return ;
}


void deal_key_down_short_press()
{
    HCN_KEY_DISPATCH(down);
    return ;
}

void deal_key_super_long_press()
{
    uint64_t currentTime = time_now_s();
    if ( ((currentTime - start_time ) <= OTA_PAGE_INTERVAL )
            &&ICON_INFO == get_current_win()
            && MENU_LEVEL_0 == get_current_levle()  )
    {
        navigator_switch_to(DEVICE_PAGE , false) ;
    }
    else
        printf("Can not open! Timer Interval = [%llu] or Focused Item Not the First One \n" ,currentTime - start_time);

    return  ;
}


void set_ready_press_state(bool state )
{
    ready_press = state ;
    return ;
}

bool get_ready_press_state()
{
    return ready_press ;
}
