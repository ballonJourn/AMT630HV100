#include "music_view.h"

const char* home_dock_music_widget_name[MUSIC_NUM_MAX] = {
    "music_image" , "music_title" , "music_clyric" ,
} ;

static widget_t* home_dock_music_widget[MUSIC_NUM_MAX] = { NULL };


ret_t home_dock_music_view_init(widget_t* parent)
{

    if(parent == NULL) return RET_FAIL;

    for (size_t i = 0; i < MUSIC_NUM_MAX; i++){
        home_dock_music_widget[i] = widget_lookup(parent, home_dock_music_widget_name[i], TRUE);
    }

    return RET_OK ;
}


ret_t home_refresh_music_image(char *blueMusicImg )
{

    // 设置图片控件的图片
    if (home_dock_music_widget[MUSIC_IMAGE]){
        image_set_image(home_dock_music_widget[MUSIC_IMAGE] , blueMusicImg ) ;
        widget_invalidate_force(home_dock_music_widget[MUSIC_IMAGE], NULL  ) ;
    }
    
    return RET_OK ;

}

ret_t home_refresh_music_title(char *title)
{
    if (home_dock_music_widget[MUSIC_TITLE]){
        widget_set_text_utf8(home_dock_music_widget[MUSIC_TITLE] , title) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_music_lyric(char *lyric)
{
    if (home_dock_music_widget[MUSIC_LYRIC]){
        widget_set_text_utf8(home_dock_music_widget[MUSIC_LYRIC] , lyric) ;
    }
    
    return RET_OK ;
}