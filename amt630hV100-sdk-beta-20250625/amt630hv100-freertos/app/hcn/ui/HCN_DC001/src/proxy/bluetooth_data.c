#include "bluetooth_data.h"
#include "vehicle_data.h"
#include "vehicle_param/vehicle_param.h"

bool vehicle_buluetooth_is_connected()
{
#if !ON_PC_CACLE
    int32_t value = vehicle_get_data(VEH_BT_CONNECTED_STATUS) ;
    return value ? true : false ;
#endif

    return false ; 
}

//播放、暂停
void vehicle_music_play() 
{
    hcn_send_music_cmd(BT_MUSIC_CMD_PLAY);
}

void vehicle_music_pause() 
{
    hcn_send_music_cmd(BT_MUSIC_CMD_PAUSE);
}

void vehicle_music_playpause() 
{
    const bt_music_info_t *music_info = hcn_bt_get_music_data();
    if (music_info)
    {
        if (music_info->play_state == BT_MUSIC_PLAY_STATE_PLAYING){
            hcn_send_music_cmd(BT_MUSIC_CMD_PAUSE);
        }else if (music_info->play_state == BT_MUSIC_PLAY_STATE_STOPED ||
                  music_info->play_state == BT_MUSIC_PLAY_STATE_PAUSED){
            hcn_send_music_cmd(BT_MUSIC_CMD_PLAY);
        }else{

        }
    }
    return ;
}


//下一曲
void vehicle_music_forward()
{
    hcn_send_music_cmd(BT_MUSIC_CMD_FORWARD);
}

//上一曲
void vehicle_music_backward()
{
    hcn_send_music_cmd(BT_MUSIC_CMD_BACKWARD);
}

const bt_music_info_t* vehicle_get_music_data()
{
    return hcn_bt_get_music_data();
}


/// @brief 蓝牙名称
/// @return 
const char* vehicle_get_bluetooth_name() 
{
    return hcn_bt_get_name();
}

const char* vehicle_get_phone_name() 
{

    if (vehicle_buluetooth_is_connected())
    {
        const bt_data_t *bt_data =  hcn_bt_get_data();  
        if (bt_data)
            return bt_data->btConnectDevName ;
    }

    return " ";
}
/// ================================= ////////////////////////

bool vehicle_buluetooth_is_calling()
{
    return hcn_bt_is_Call();
}

const bt_call_t* vehicle_get_calling_data() 
{
    return hcn_bt_get_call();   
}

const bt_data_t* vehicle_get_bluetooth_data() 
{
    return hcn_bt_get_data();   
}

void  vehicle_set_bluetooth_state(bool on_off)
{
    hcn_bt_switch_state(on_off) ;
    return ;
} 

void  vehicle_calling_hung_up()
{
    hcn_bt_hung_up();
    return ;
}

void  vehicle_calling_pick_up()
{
    hcn_bt_pick_up();
    return ;
} 