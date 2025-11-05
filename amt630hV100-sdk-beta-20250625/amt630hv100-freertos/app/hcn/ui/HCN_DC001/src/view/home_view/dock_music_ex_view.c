#include "dock_music_ex_view.h"

extern ret_t stb_load_image(int32_t subtype, const uint8_t* buff, uint32_t buff_size, bitmap_t* image,
                     bool_t require_bgra, bool_t enable_bgr565, bool_t enable_rgb565);


const char* home_dock_music_ex_widget_name[MUSIC_DOCK_NUM_MAX] = {
    "music_image_ex" , "music_title_ex" , "music_lyric_ex" , "music_bar" , 
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

ret_t home_refresh_music_ex_image(char *blueMusicImg , int length)
{
    // 将图片数据添加到资源管理器
    // 验证资源是否存在
    // const asset_info_t* asset = assets_manager_ref(assets_manager(), ASSET_TYPE_IMAGE, BLUETOOTH_MUSIC_IMAGE );
    // if (asset != NULL) {
    //     printf("asset_info_t successed! size: %d\n", asset->size);
    //     assets_manager_unref(assets_manager(), asset);
    // } else {
    //     assets_manager_add_data(assets_manager(), BLUETOOTH_MUSIC_IMAGE , ASSET_TYPE_IMAGE, ASSET_TYPE_IMAGE_PNG, (uint8_t*)blueMusicImg, sizeof(blueMusicImg));
    // }
    
    bitmap_t bmp, tmps;
    // 解码对应的图片
    if (RET_OK == stb_load_image(ASSET_TYPE_IMAGE_PNG, (uint8_t*)blueMusicImg , length , &bmp, 0, 0, 0)) {
        // 释放旧的图片缓存
        if (RET_OK == image_manager_get_bitmap(image_manager(), BLUETOOTH_MUSIC_IMAGE, &tmps)) {
            image_manager_unload_bitmap(image_manager(), &tmps);
        }
        image_manager_add(image_manager(), BLUETOOTH_MUSIC_IMAGE, &bmp);
    }else{
        log_debug("BlueMusicPicData decode error************* \r\n");
    }

    // 设置图片控件的图片
    if (home_dock_music_ex_widget[MUSIC_IMAGE_EX]){
        image_set_image(home_dock_music_ex_widget[MUSIC_IMAGE_EX] , BLUETOOTH_MUSIC_IMAGE ) ;
        widget_invalidate_force(home_dock_music_ex_widget[MUSIC_IMAGE_EX], NULL  ) ;
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
        widget_set_style_str(music_state_widget ,STATE_NORMAL "." STYLE_ID_ICON ,  music_play_state_image_str[MUSIC_NORMAL][is_playing]);
        widget_set_style_str(music_state_widget ,STATE_SELECTE "." STYLE_ID_ICON , music_play_state_image_str[MUSIC_SELECTED][is_playing]);
    }
    
    return RET_OK ;
}

void music_ex_view_set_focused_item(music_ex_focused_e focusedIndex)
{
    int offset = MUSIC_NEXT - MUSIC_FOCUSED_NEXT ;
    for (size_t i = 0; i < MUSIC_FOCUSED_MAX ; i++)
    {
        if (home_dock_music_ex_widget[i + offset])
        {
            if (i == focusedIndex)
                widget_set_state(home_dock_music_ex_widget[i + offset], STATE_SELECTE) ;
            else
                widget_set_state(home_dock_music_ex_widget[i + offset], STATE_NORMAL ) ;

                
            widget_invalidate_force(home_dock_music_ex_widget[i + offset] , NULL)  ;
        }
    }
    
    widget_invalidate_force(home_dock_music_ex_widget[MUSIC_BAR] ,NULL);
    return ;

    // image_set_image();
}

/***********
// 图片名默认为assets_manager_load_file加载的路径
#define IMAGE_NAME "/media/sda1/AWTK.png" 

static ret_t on_unload_button_click(void* ctx, event_t* e) 
{ 
    // 点击卸载图片按钮卸载图片缓存  
    bitmap_t bitmap = {0};  widget_t* win = WIDGET(ctx);  widget_t* image = widget_lookup(win, "image", TRUE);  
    // 卸载图片管理器缓存  
    image_manager_get_bitmap(image_manager(), IMAGE_NAME, &bitmap);  image_manager_unload_bitmap(image_manager(), &bitmap);
    // 卸载资源管理器缓存 
    assets_manager_clear_cache_ex(assets_manager(), ASSET_TYPE_IMAGE, IMAGE_NAME);  widget_invalidate(image, NULL); 
    return RET_OK;
}

static ret_t on_load_button_click(void* ctx, event_t* e) 
{ 
    // 点击加载图片按钮重新加载图片缓存  
    widget_t* win = WIDGET(ctx);  widget_t* image = widget_lookup(win, "image", TRUE);
    // 将新的图片数据添加到资源管理器缓存中  
    asset_info_t* img = assets_manager_load_file(assets_manager(), ASSET_TYPE_IMAGE, IMAGE_NAME);  assets_manager_add(assets_manager(), img);
    image_set_image(image, IMAGE_NAME);
    widget_invalidate(image, NULL);  
    return RET_OK;
}

// 假设 album_cover_data 是蓝牙接收到的图片数据
extern uint8_t album_cover_data[];
extern size_t album_cover_data_size;

void load_album_cover() {
    // 创建一个 image 控件
    widget_t *image = image_create(NULL, 0, 0, 200, 200);
    if (!image) {
        // 处理创建失败的情况
        return;
    }

    // 将图片数据添加到资源管理器
    assets_manager_add_data(assets_manager(), "album_cover", ASSET_TYPE_IMAGE, ASSET_TYPE_IMAGE_PNG, album_cover_data, album_cover_data_size);

    // 设置图片控件的图片
    image_set_image(image, "album_cover");
}
************* */

