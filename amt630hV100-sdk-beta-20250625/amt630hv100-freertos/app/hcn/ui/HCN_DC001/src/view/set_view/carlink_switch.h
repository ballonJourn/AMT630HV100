#ifndef CARLINK_SWITCH__H_
#define CARLINK_SWITCH__H_

#include "awtk.h"
#include "../view_manager.h"

typedef enum {
    CARLINK_CP_OPTION   ,   ///< CarPlay
    CARLINK_EY_OPTION   ,   ///< 亿连
    CARLINK_OPTION_MAX  ,
} carlink_option_e ;

/**
 * @brief 弹窗状态
 */
typedef enum {
    CARLINK_POPUP_NONE  ,   ///< 无弹窗
    CARLINK_POPUP_SHOW  ,   ///< 弹窗显示中
} carlink_popup_state_e ;

ret_t set_carlink_view_init(widget_t* parent) ;

void carlink_switch_init() ;

void on_carlink_switch_deal_short_key(key_id_e key) ;

void carlink_view_clean_state() ;

void carlink_view_set_focused_item(carlink_option_e focusedIndex) ;

#endif
