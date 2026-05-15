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
#include "hcn_global.h"

static bt_music_info_t g_music_info = { 0 };
static bt_call_t g_call_info        = { 0 };
static bt_data_t g_bt_info          = { 0 };
static bool  g_bluetooth_state = false ;
static bool  g_is_calling      = false ;


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
    
}


ret_t parse_music_data(const bt_music_info_t *_music_info)
{
    if (NULL == _music_info)
        return RET_FAIL ;
    
    char format_buff[TEXT_PARAM_LEN] = {0};

    if (g_music_info.play_state != _music_info->play_state)
    {
        home_refresh_music_state(_music_info->play_state == BT_MUSIC_PLAY_STATE_PLAYING);
        printf("music state changed = %d \n" , _music_info->play_state);
    }

    if (strcmp(g_music_info.artist, _music_info->artist))
    {
        
        memset(format_buff , 0x0 , sizeof(format_buff));
        tk_snprintf(format_buff , sizeof(format_buff)   , "%s",_music_info->artist);

        home_refresh_music_ex_title(format_buff);
        home_refresh_music_title(format_buff);
        printf("g_music_info  artist changed = %s\n" ,format_buff);
    }

    if (strcmp(g_music_info.lyrics, _music_info->lyrics))
    {
        memset(format_buff , 0x0 , sizeof(format_buff));
        tk_snprintf(format_buff , sizeof(format_buff)   , "%s",_music_info->lyrics);

        home_refresh_music_ex_lyric(format_buff);
        home_refresh_music_lyric(format_buff);
        printf("g_music_info  lyrics changed = %s\n" ,format_buff);
    }


    if (memcmp(&g_music_info.music , &_music_info->music  , sizeof(g_music_info.music)) )
    {
        home_refresh_music_bar( _music_info->music.cur_time_music_play , _music_info->music.music_total_time);
        printf("music music_total_time = %d  current time changed = %d \n" ,_music_info->music.music_total_time , _music_info->music.cur_time_music_play);
    }
    

    if(memcmp(&g_music_info.song_art_cover , &_music_info->song_art_cover  , sizeof(g_music_info.song_art_cover)))
    {

        if (RET_OK == gloabl_load_image((uint8_t *)_music_info->song_art_cover.image_buffer , _music_info->song_art_cover.image_len))
        {
            home_refresh_music_ex_image(BLUETOOTH_MUSIC_IMAGE) ;
            home_refresh_music_image(BLUETOOTH_MUSIC_IMAGE);
        }else{
            home_refresh_music_ex_image(BLUETOOTH_DEFAULT_IMAGE) ;
            home_refresh_music_image(BLUETOOTH_DEFAULT_IMAGE);
        }
        printf("g_music_info image changed image_index = %d\n" ,_music_info->song_art_cover.img_index);
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

    home_refresh_music_bar(0,100);

    home_refresh_music_current_time(0);
    home_refresh_music_total_time(0);

    home_refresh_music_ex_image(BLUETOOTH_DEFAULT_IMAGE) ;
    home_refresh_music_image(BLUETOOTH_DEFAULT_IMAGE);
}


void calling_view_update()
{
    if (g_bluetooth_state)
    {
        bool _is_calling = vehicle_buluetooth_is_calling();
        if (g_is_calling != _is_calling)
        {
            g_is_calling = _is_calling ;
            home_refresh_phone_view(g_is_calling ? PHONE_CONNECT : PHONE_NO_CONNECT) ;
        }
        
        const bt_call_t *_call_info = vehicle_get_calling_data();
        if (memcmp(_call_info , &g_call_info , sizeof(bt_call_t)))
        {
            if (RET_OK == parse_calling_data(_call_info))
            {
                memcpy(&g_call_info , _call_info ,sizeof(bt_call_t));
            }    
        }
        
    }
}


ret_t parse_calling_data(const bt_call_t *_call_info)
{
    if (NULL == _call_info)
        return RET_FAIL ;
    
    
    if(g_call_info.btHfpState != _call_info->btHfpState)
    {
        if (_call_info->btHfpState == OUTGOING_CALL)
        {
            home_refresh_phone_state(CALL_OUTGOING);
            refresh_pop_call_state(OUTGOING_CALL) ;
            calling_animation_start();

        }else if (_call_info->btHfpState == INCOMING_CALL)
        {
            home_refresh_phone_state(CALL_INCOMMING);
            refresh_pop_call_state(INCOMING_CALL) ;
            calling_animation_start();

        }else if (_call_info->btHfpState == ACTIVE_CALL)
        {
            home_refresh_phone_state(CALL_CALLING);
            refresh_pop_call_state(ACTIVE_CALL) ;
            // calling_animation_stop();
        }
        
        if (_call_info->btHfpState <= CONNECTED)
        {
            calling_animation_stop();
        }
        
    }

    if (strcmp(g_call_info.btCallNumber1, _call_info->btCallNumber1))
    {
        if (_call_info->btCallPerson1[0])
        {
            printf("has personnal info = %s \n ", _call_info->btCallPerson1) ;
            home_refresh_phone_numName(_call_info->btCallPerson1);
        }
        else 
        {
            printf("No personnal info number = %s \n ", _call_info->btCallNumber1) ;
            home_refresh_phone_numName(_call_info->btCallNumber1);
        }
        
    }

    return RET_OK ;
}


ret_t parse_buletooth_data(const bt_data_t *bt_info)
{
    if (NULL == bt_info)
        return RET_FAIL ;

    if (g_bt_info.btSwitchState != bt_info->btSwitchState)
    {
        refresh_bt_connect_state( bt_info->btSwitchState ? BT_CONNECT_ON_OPTION : BT_CONNECT_OFF_OPTION);
    }
    
    if (g_bt_info.btSignal != bt_info->btSignal)
    {
        home_refresh_GMS_level( (int)(0.6f * bt_info->btSignal));
    }

    return RET_OK ;
}


void bluetooth_data_update()
{
    const bt_data_t *bt_info = vehicle_get_bluetooth_data();
    if (memcmp(bt_info , &g_bt_info , sizeof(bt_data_t)))
    {
        if (RET_OK == parse_buletooth_data(bt_info))
        {
            memcpy(&g_bt_info , bt_info ,sizeof(bt_data_t));
        }    
    }

}


void bluetooth_view_update()
{

    music_view_update() ;

    calling_view_update();

    bluetooth_data_update(); 

    bool bt_state = vehicle_buluetooth_is_connected();
    // home_refresh_signal(ICON_BT , bt_state);   
    if (bt_state != g_bluetooth_state)
    {

        if(false == bt_state )
        {
            //断开连接
            memset(&g_music_info , 0x00 , sizeof(bt_music_info_t));
            home_clean_music_data() ;
            
            //通话
            memset(&g_call_info , 0x00 , sizeof(bt_call_t));
            home_refresh_phone_view(PHONE_NO_CONNECT);
            calling_animation_stop();

            refresh_bt_phone_info("");
        }
        else
        {
            const char* name =  vehicle_get_phone_name();
            if (name[0]) refresh_bt_phone_info(name) ;
            else return ;
        }

        g_bluetooth_state = bt_state ;
    }

}