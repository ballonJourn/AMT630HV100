#include "carlink_switch.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"
#include "logic/hcn_global.h"

const char* set_carlink_widget_name[CARLINK_OPTION_MAX] = {
    "carlink_cp_option" , "carlink_ey_option"
} ;

static const char* carlink_popup_widget_names[] = {
    "carlink_popup_view",
    "carlink_popup_confirm_bg",
    "carlink_popup_cancel_bg",
    "carlink_popup_confirm",
    "carlink_popup_cancel",
    "carlink_popup_title",
};

enum {
    POPUP_W_VIEW = 0,
    POPUP_W_CONFIRM_BG,
    POPUP_W_CANCEL_BG,
    POPUP_W_CONFIRM,
    POPUP_W_CANCEL,
    POPUP_W_TITLE,
    POPUP_W_MAX,
};

static widget_t* set_carlink_widget[CARLINK_OPTION_MAX] = { NULL };
static widget_t* popup_widget[POPUP_W_MAX] = { NULL };

static carlink_option_e option = CARLINK_CP_OPTION ;
static carlink_option_e saved_option = CARLINK_CP_OPTION ;  ///< 进入前的值,用于取消恢复
static carlink_popup_state_e popup_state = CARLINK_POPUP_NONE ;
static int popup_focus = 0 ;  ///< 0=确认  1=取消

ret_t set_carlink_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < CARLINK_OPTION_MAX; i++){
        set_carlink_widget[i] = widget_lookup(parent, set_carlink_widget_name[i], TRUE);
    }

    for (size_t i = 0; i < POPUP_W_MAX; i++){
        popup_widget[i] = widget_lookup(parent, carlink_popup_widget_names[i], TRUE);
    }

    return RET_OK ;
}

/* ---- 弹窗高亮 ---- */
static void hl_popup(int f)
{
    popup_focus = f ;
    if (popup_widget[POPUP_W_CONFIRM_BG])
        widget_set_state(popup_widget[POPUP_W_CONFIRM_BG], (f == 0) ? STATE_SELECTE : STATE_NORMAL);
    if (popup_widget[POPUP_W_CANCEL_BG])
        widget_set_state(popup_widget[POPUP_W_CANCEL_BG],  (f == 1) ? STATE_SELECTE : STATE_NORMAL);
}

static void show_popup(bool visible)
{
    if (popup_widget[POPUP_W_VIEW])
        widget_set_visible(popup_widget[POPUP_W_VIEW], visible);

    popup_state = visible ? CARLINK_POPUP_SHOW : CARLINK_POPUP_NONE ;
}

/* ---- 确认重启 ---- */
static void do_confirm_reboot(void)
{
    printf("carlink_switch: confirm, type=%d, saving & reboot\n", option);

    vehicle_set_param_carlink_type((uint8_t)option);

#if !ON_PC_CACLE
    save_hcn_usr_param();

    extern void wdt_cpu_reboot(void);
    wdt_cpu_reboot();
#endif
}

/* ---- 取消, 恢复选项不修改配置 ---- */
static void do_cancel(void)
{
    printf("carlink_switch: cancel, restore to %d\n", saved_option);

    option = saved_option ;
    carlink_view_set_focused_item(option) ;
    show_popup(false) ;
}

/* ---- SET键处理 ---- */
static void carlink_view_deal_set(void)
{
    if (popup_state == CARLINK_POPUP_SHOW)
    {
        /* 弹窗中按SET */
        if (popup_focus == 0) {
            do_confirm_reboot();
        } else {
            do_cancel();
        }
        return ;
    }

    /* 正常选项页, 按SET进入弹窗确认 */
    if (option == saved_option) {
        /* 没有改变, 不需要弹窗, 直接返回上级 */
        set_current_level(MENU_LEVEL_1);
        carlink_view_clean_state() ;
        return ;
    }

    /* 选项有变化, 弹窗提示重启 */
    popup_focus = 1 ;  ///< 默认焦点在"取消"上,防误触
    show_popup(true) ;
    hl_popup(popup_focus) ;

    return ;
}

/* ---- BACK键处理 ---- */
static void carlink_view_deal_back(void)
{
    if (popup_state == CARLINK_POPUP_SHOW)
    {
        do_cancel();
        return ;
    }

    /* 恢复进入前的值 */
    option = saved_option ;
    carlink_view_set_focused_item(option) ;

    set_current_level(MENU_LEVEL_1);
    carlink_view_clean_state() ;

    return ;
}

/* ---- UP/DOWN键处理 ---- */
static void carlink_view_deal_up(void)
{
    if (popup_state == CARLINK_POPUP_SHOW) {
        popup_focus = 0 ;
        hl_popup(popup_focus) ;
        return ;
    }

    option = (option - 1 + CARLINK_OPTION_MAX) % CARLINK_OPTION_MAX ;
    carlink_view_set_focused_item(option) ;

    return ;
}

static void carlink_view_deal_down(void)
{
    if (popup_state == CARLINK_POPUP_SHOW) {
        popup_focus = 1 ;
        hl_popup(popup_focus) ;
        return ;
    }

    option = (option + 1) % CARLINK_OPTION_MAX ;
    carlink_view_set_focused_item(option) ;

    return ;
}

static short_click_deal short_click[] = {
    [KEY_SHORT_UP]   = carlink_view_deal_up  ,
    [KEY_SHORT_DOWN] = carlink_view_deal_down,
    [KEY_SHORT_SET]  = carlink_view_deal_set ,
    [KEY_SHORT_BACK] = carlink_view_deal_back,
};

void carlink_switch_init(void)
{
    uint8_t value = vehicle_get_param_carlink_type();
    if (value >= CARLINK_OPTION_MAX)
        value = CARLINK_CP_OPTION ;

    option = (carlink_option_e)value ;
    saved_option = option ;

    carlink_view_set_focused_item(option) ;
    show_popup(false) ;

    return ;
}

void on_carlink_switch_deal_short_key(key_id_e key)
{
    printf("on_carlink_switch_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(short_click) / sizeof(short_click_deal)
        && short_click[key]) {
        short_click[key]();
    }

    return ;
}

void carlink_view_set_focused_item(carlink_option_e focusedIndex)
{
    for (size_t i = 0; i < CARLINK_OPTION_MAX ; i++)
    {
        if (set_carlink_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_carlink_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_carlink_widget[i], STATE_NORMAL ) ;

            widget_invalidate_force(set_carlink_widget[i] , NULL)  ;
        }
        else{
            printf(" carlink_view_set_focused_item not find widget \n");
            return ;
        }
    }

    option = focusedIndex ;

    return ;
}

void carlink_view_clean_state(void)
{
    for (size_t i = 0; i < CARLINK_OPTION_MAX ; i++)
    {
        if (set_carlink_widget[i])
        {
            widget_set_state(set_carlink_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_carlink_widget[i] , NULL)  ;
        }
    }

    show_popup(false) ;

    return ;
}
