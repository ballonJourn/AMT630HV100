#ifndef BRIGHTNESS_H__
#define BRIGHTNESS_H__

#include "awtk.h"
#include "../view_manager.h"

// enum set_brightness_com{
//     DISPLAY_AUTO      ,
//     DISPLAY_DAY       ,
//     DISPLAY_NIGHT     ,
//     DISPLAY_NUM_MAX   ,
// };

// typedef enum {
//     DIAPLAY_AUTO_OPTION   ,
//     DIAPLAY_DAY_OPTION    ,
//     DIAPLAY_NIGHT_OPTION  ,
//     DIAPLAY_OPTION_MAX    ,
// }brightness_option_e ;


ret_t set_brightness_view_init(widget_t* parent) ;

void brightness_init() ;

void on_brightness_deal_short_key(key_id_e key) ;

void brightness_view_clean_state() ;

void brightness_view_set_focused_item(int focusedIndex) ;

#endif