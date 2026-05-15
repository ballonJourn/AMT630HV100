
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

//播放
void vehicle_music_play() ;

//暂停
void vehicle_music_pause() ;

//下一曲
void vehicle_music_forward() ;

//上一曲
void vehicle_music_backward() ;

//音乐信息
const bt_music_info_t* vehicle_get_music_data();

///蓝牙名称
const char* vehicle_get_bluetooth_name() ;

///< 获取蓝牙库版本
const char* vehicle_get_bluetooth_version() ;

//获取连接手机名称
const char* vehicle_get_phone_name() ;

//通话数据
const bt_call_t* vehicle_get_calling_data() ;


//蓝牙信息 信号强度、联系人等
const bt_data_t* vehicle_get_bluetooth_data() ;

bool vehicle_buluetooth_is_calling();

const bt_call_t* vehicle_get_calling_data() ;

const bt_data_t* vehicle_get_bluetooth_data() ;


//设置蓝牙开关
void  vehicle_set_bluetooth_state(bool on_off) ;

//挂断
void  vehicle_calling_hung_up() ;

//接听
void  vehicle_calling_pick_up() ;

#endif