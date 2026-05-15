#ifndef LANGUAGE__H_
#define LANGUAGE__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_language_com{
    LANGUAGE_CHINESE    ,
    LANGUAGE_ENGLISH    ,

    LANGUAGE_NUM_MAX    ,
};

typedef enum {
    LANGUAGE_CHINESE_OPTION  ,
    LANGUAGE_ENGLISH_OPTION  ,

    LANGUAGE_OPTION_MAX      ,
}language_option_e ;


ret_t set_language_view_init(widget_t* parent) ;

void language_init() ;

void on_language_deal_short_key(key_id_e key) ;

void language_view_clean_state() ;

void language_view_set_focused_item(language_option_e focusedIndex) ;



#endif