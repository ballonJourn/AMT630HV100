#ifndef SETTING_CLOCK__H_
#define SETTING_CLOCK__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_clock_com{
    CLOCK_H_1     ,
    CLOCK_H_2     ,
    CLOCK_M_1     ,
    CLOCK_M_2     ,

    CLOCK_MAX     ,      
};

typedef enum {
    CLOCK_H_1_OPTION     ,
    CLOCK_H_2_OPTION     ,
    CLOCK_M_1_OPTION     ,
    CLOCK_M_2_OPTION     ,

    CLOCK_OPTION_NUM_MAX ,
}clock_option_e ;

/* ── 二级子菜单：调整时间 / 调整制式 ── */
typedef enum {
    CLOCK_SUB_ADJUST_TIME   ,
    CLOCK_SUB_ADJUST_FORMAT ,

    CLOCK_SUB_NUM_MAX       ,
}clock_sub_menu_e ;

/* ── 三级子菜单（制式选择）── */
typedef enum {
    CLOCK_FMT_24H  ,
    CLOCK_FMT_12H  ,

    CLOCK_FMT_MAX  ,
}clock_fmt_option_e ;

ret_t set_clock_view_init(widget_t* parent) ;

void clock_init() ;

void refresh_clock(int min ,int sec);

void get_label_clock(int32_t *min , int32_t *sec);

void on_clock_deal_short_key(key_id_e key) ;

void clock_view_clean_state() ;

void clock_view_set_focused_item(clock_option_e focusedIndex) ;


/// @brief 三级页面（调整时间数字）
void clock_option_init() ;

void on_clock_option_deal_short_key(key_id_e key) ;

/// @brief 三级页面（调整制式）
void clock_fmt_init() ;

void on_clock_fmt_deal_short_key(key_id_e key) ;

/// @brief 获取当前时间制式 0=24H 1=12H
uint8_t clock_get_time_format(void) ;

#endif