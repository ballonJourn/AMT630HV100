#ifndef DISPLAY__H_
#define DISPLAY__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_display_com{
    DISPLAY_AUTO      ,
    DISPLAY_DAY       ,
    DISPLAY_NIGHT     ,
    DISPLAY_NUM_MAX   ,
};

typedef enum {
    DIAPLAY_AUTO_OPTION   ,
    DIAPLAY_DAY_OPTION    ,
    DIAPLAY_NIGHT_OPTION  ,
    DIAPLAY_OPTION_MAX    ,
}display_option_e ;


ret_t set_display_view_init(widget_t* parent) ;

void display_init() ;

void on_display_deal_short_key(key_id_e key) ;

void display_view_clean_state() ;

void display_view_set_focused_item(display_option_e focusedIndex) ;

#endif