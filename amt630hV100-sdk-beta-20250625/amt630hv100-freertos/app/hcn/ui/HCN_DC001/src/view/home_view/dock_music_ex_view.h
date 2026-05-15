#ifndef SLIDER_MUSIC_EX_VIEW_H_
#define SLIDER_MUSIC_EX_VIEW_H_
#include "common.h"

enum music_ex_com{
    MUSIC_IMAGE_EX     ,
    MUSIC_TITLE_EX     ,
    MUSIC_LYRIC_EX     ,
    MUSIC_BAR          ,
    MUSIC_PREV         ,
    MUSIC_STATE        ,
    MUSIC_NEXT         ,
    MUSIC_TOTAL        ,
    MUSIC_CURRENT      ,
    MUSIC_DOCK_NUM_MAX ,
};


typedef enum music_ex_focused_com{
    
    MUSIC_FOCUSED_PREV  ,
    MUSIC_FOCUSED_STATE ,
    MUSIC_FOCUSED_NEXT  ,

    MUSIC_FOCUSED_MAX   ,
}music_ex_focused_e;


ret_t home_dock_music_ex_view_init(widget_t* parent);

ret_t home_refresh_music_ex_image(char *blueMusicImg ) ;

ret_t home_refresh_music_ex_title(char *title);

ret_t home_refresh_music_ex_lyric(char *lyric);

ret_t home_refresh_music_bar(int value , int max);

ret_t home_refresh_music_state(bool_t isplay);

ret_t home_refresh_music_total_time(uint32_t total_seconds) ;

ret_t home_refresh_music_current_time(uint32_t total_seconds);

void music_ex_view_set_focused_item(music_ex_focused_e focusedIndex) ;

void music_ex_view_init() ;

#endif