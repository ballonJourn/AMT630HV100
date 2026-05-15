#ifndef CYCLING_ENERGY__H_
#define CYCLING_ENERGY__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_cycling_energy_com{
    RIDE_5_KM           ,
    RIDE_20_KM          ,
    RIDE_LAST_ELEC      ,
    RIDE_CURRENT_ELEC   ,
    RIDE_AVG_ELEC       ,

    RIDE_LINE_SERIES    ,

    RIDE_NUM_MAX        ,
};

typedef enum {
    RIDE_5KM_OPTION  ,
    RIDE_20KM_OPTION ,

    RIDE_OPTION_MAX  ,
}cycling_engrgy_option_e ;


ret_t set_cycling_energy_view_init(widget_t* parent) ;

void cycling_engrgy_init() ;

void on_cycling_engrgy_deal_short_key(key_id_e key) ;

void cycling_engergy_view_clean_state() ;

void cycling_engergy_view_set_focused_item(cycling_engrgy_option_e focusedIndex) ;

ret_t series_push(widget_t* widget, const void* data, uint32_t nr);

#endif