#include "dock_music_ex_view.h"

const char* home_dock_music_ex_widget_name[MUSIC_DOCK_NUM_MAX] = {
    "music_image_ex" , "music_title_ex" , "music_clyric_ex" , "music_bar" , 
    "music_prev", "music_state" , "music_next"
} ;

static widget_t* home_dock_music_ex_widget[MUSIC_DOCK_NUM_MAX] = { NULL };

typedef enum {
    MUSIC_STOP ,        //暂停
    MUSIC_PLAY ,        //播放

    MUSIC_NONE ,
}music_play_e ;

typedef enum {
    MUSIC_NORMAL     ,
    MUSIC_SELECTED   ,

    MUSIC_STETE_NONE ,
}music_state_e ;   

static music_play_e is_playing = MUSIC_STOP ;
static music_state_e state     = MUSIC_NORMAL ;

const char* music_play_state_image_str[MUSIC_STETE_NONE][MUSIC_NONE] = {
    "icon_play_n"  , "icon_pause_n"  ,
    "icon_play_p"  , "icon_pause_p"  ,
};


ret_t home_dock_music_ex_view_init(widget_t* parent)
{

    if(parent == NULL) return RET_FAIL;

    for (size_t i = 0; i < MUSIC_DOCK_NUM_MAX; i++){
        home_dock_music_ex_widget[i] = widget_lookup(parent, home_dock_music_ex_widget_name[i], TRUE);
    }

    return RET_OK ;
}

//首次进入播放页面初始化
void music_ex_view_init()
{
    music_ex_view_set_focused_item(MUSIC_FOCUSED_STATE);
    return ;
}

ret_t home_refresh_music_ex_image(char *image)
{
    if (home_dock_music_ex_widget[MUSIC_IMAGE_EX]){
        image_set_image(home_dock_music_ex_widget[MUSIC_IMAGE_EX] , image) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_music_ex_title(char *title)
{
    if (home_dock_music_ex_widget[MUSIC_TITLE_EX]){
        widget_set_text_utf8(home_dock_music_ex_widget[MUSIC_TITLE_EX] , title) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_music_ex_lyric(char *lyric)
{
    if (home_dock_music_ex_widget[MUSIC_LYRIC_EX]){
        widget_set_text_utf8(home_dock_music_ex_widget[MUSIC_LYRIC_EX] , lyric) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_music_bar(int value ,int max)
{
    widget_t *widget = home_dock_music_ex_widget[MUSIC_BAR] ;
    if (widget){
        progress_bar_set_max(widget , max);
        progress_bar_set_value(widget , value) ;
    }
    
    return RET_OK ;
}

ret_t home_refresh_music_state(bool_t isplay)
{
    value_t v ;
    is_playing = isplay ? MUSIC_PLAY : MUSIC_STOP ;

    widget_t *music_state_widget = home_dock_music_ex_widget[MUSIC_STATE] ;

    if (music_state_widget)
    {
        widget_get_prop(music_state_widget ,WIDGET_PROP_STATE_FOR_STYLE , &v) ;
        // printf("value_t = %s  " , value_str(&v)) ;
        state = (tk_str_cmp(value_str(&v) , STATE_NORMAL) == 0) ? MUSIC_NORMAL : MUSIC_SELECTED ;
        (void)state ;
        widget_set_style_str(music_state_widget ,STATE_NORMAL "." STYLE_ID_ICON , music_play_state_image_str[MUSIC_NORMAL][is_playing]);
        widget_set_style_str(music_state_widget ,STATE_SELECTE "." STYLE_ID_ICON , music_play_state_image_str[MUSIC_SELECTED][is_playing]);
    }
    
    return RET_OK ;
}

void music_ex_view_set_focused_item(music_ex_focused_e focusedIndex)
{
    int offset = MUSIC_NEXT - MUSIC_FOCUSED_NEXT ;
    for (size_t i = 0; i < MUSIC_FOCUSED_MAX ; i++)
    {
        if (i == focusedIndex){
            if (home_dock_music_ex_widget[i + offset])
                widget_set_state(home_dock_music_ex_widget[i + offset], STATE_SELECTE) ;
        }
        else{
            if (home_dock_music_ex_widget[i + offset])
                widget_set_state(home_dock_music_ex_widget[i + offset], STATE_NORMAL ) ;
        }
    }
    
    widget_invalidate_force(home_dock_music_ex_widget[MUSIC_BAR] ,NULL);
    return ;
}
