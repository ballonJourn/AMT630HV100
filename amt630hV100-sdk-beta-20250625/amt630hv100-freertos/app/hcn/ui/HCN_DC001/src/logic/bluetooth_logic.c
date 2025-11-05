#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "awtk.h"
#include "buletooth_logic.h"
#include "view/home_view/home_view_interface.h"
#include "proxy/vehicle_data.h"
#include "carlink_cb/hcn_easy_navi.h"
#include "proxy/bluetooth_data.h"
#include "view/set_view/bt_connect.h"

static bt_music_info_t g_music_info = { 0 };
static bool  g_bluetooth_state = false ;

void music_view_update() 
{
    if (g_bluetooth_state)
    {
        const bt_music_info_t *_music_info = vehicle_get_music_data();
        if (memcmp(_music_info , &g_music_info , sizeof(bt_music_info_t)))
        {
            if (RET_OK == parse_music_data(_music_info))
            {
                memcpy(&g_music_info , _music_info ,sizeof(bt_music_info_t));
            }    
        }
        
    }
    
    bool bt_state = vehicle_buluetooth_is_connected();
    if (bt_state != g_bluetooth_state)
    {
        if(false == bt_state )
        {
            memset(&g_music_info , 0x00 , sizeof(bt_music_info_t));
            home_clean_music_data() ;
            refresh_bt_phone_info(" "); 
        }
        else
        {
            refresh_bt_phone_info(vehicle_get_phone_name()) ;
        }

        g_bluetooth_state = bt_state ;
    }
    
}


ret_t parse_music_data(const bt_music_info_t *_music_info)
{
    if (NULL == _music_info)
        return RET_FAIL ;
    
    char format_buff[128] = {0};

    if (g_music_info.play_state != _music_info->play_state)
    {
        home_refresh_music_state(_music_info->play_state == BT_MUSIC_PLAY_STATE_PLAYING);
        printf("music state = %d \n" , _music_info->play_state);
    }

    if (strcmp(g_music_info.artist, _music_info->artist))
    {
        memset(format_buff , 0x0 , sizeof(format_buff));
        tk_snprintf(format_buff , sizeof(format_buff) - 1  , "%s",_music_info->artist);
        home_refresh_music_ex_title(format_buff);
        home_refresh_music_title(format_buff);
        printf("g_music_info  artist = %s\n" ,_music_info->artist);
    }

    if (strcmp(g_music_info.lyrics, _music_info->lyrics))
    {
        memset(format_buff , 0x0 , sizeof(format_buff));
        tk_snprintf(format_buff , sizeof(format_buff) - 1  , "%s",_music_info->lyrics);
        home_refresh_music_ex_lyric(format_buff);
        home_refresh_music_lyric(format_buff);
        printf("g_music_info  lyrics = %s\n" ,_music_info->lyrics);
    }


    if (memcmp(&g_music_info.music , &_music_info->music  , sizeof(g_music_info.music)) )
    {
        home_refresh_music_bar( _music_info->music.cur_time_music_play , _music_info->music.music_total_time);
        printf("music music_total_time = %d  current time = %d \n" ,_music_info->music.music_total_time , _music_info->music.cur_time_music_play);
    }
    
    return RET_OK ;
}


void home_clean_music_data()
{
    char buff[256] = {0};
    const char *tr_txt = locale_info_tr(locale_info(), "no_music");
    tk_snprintf(buff , sizeof(buff) - 1 , "%s" , tr_txt) ;
    //char *text = tr_txt ;
    home_refresh_music_ex_title(buff);
    home_refresh_music_title(buff);

    home_refresh_music_ex_lyric(" ");
    home_refresh_music_lyric(" ");

    home_refresh_music_state(false);
}