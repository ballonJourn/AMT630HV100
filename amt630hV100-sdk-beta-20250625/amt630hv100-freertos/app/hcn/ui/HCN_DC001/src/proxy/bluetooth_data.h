
#ifndef BLUETOOTH_DATA_H
#define BLUETOOTH_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "carlink_cb/hcn_carlink_cb.h"


bool vehicle_buluetooth_is_connected() ;

/// @brief 音乐信息
/// @return 

//播放、暂停
void vehicle_music_playpause() ;


//播放、暂停
void vehicle_music_play() ;

void vehicle_music_pause() ;

void vehicle_music_playpause() ;

//下一曲
void vehicle_music_forward() ;

//上一曲
void vehicle_music_backward() ;

//音乐信息
const bt_music_info_t* vehicle_get_music_data();


///蓝牙数据
const char* vehicle_get_bluetooth_name() ;

const char* vehicle_get_phone_name() ;

#endif