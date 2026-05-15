#ifndef UNIT__H_
#define UNIT__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_unit_com{
    UNIT_KM    ,
    UNIT_MILE    ,

    UNIT_NUM_MAX    ,
};

typedef enum {
    UNIT_KM_OPTION  ,
    UNIT_MILE_OPTION  ,

    UNIT_OPTION_MAX      ,
}unit_option_e ;


ret_t set_unit_view_init(widget_t* parent) ;

void unit_init() ;

void on_unit_deal_short_key(key_id_e key) ;

void unit_view_clean_state() ;

void unit_view_set_focused_item(unit_option_e focusedIndex) ;

#endif