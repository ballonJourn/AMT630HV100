#ifndef HOME_MUSIC_VIEW_H
#define HOME_MUSIC_VIEW_H
#include "common.h"


enum music_com{
    MUSIC_IMAGE   ,
    MUSIC_TITLE   ,
    MUSIC_LYRIC   ,

    MUSIC_NUM_MAX ,
};


ret_t home_dock_music_view_init(widget_t* parent);

ret_t home_refresh_music_image(char *blueMusicImg ) ;

ret_t home_refresh_music_title(char *title);

ret_t home_refresh_music_lyric(char *lyric);









#endif