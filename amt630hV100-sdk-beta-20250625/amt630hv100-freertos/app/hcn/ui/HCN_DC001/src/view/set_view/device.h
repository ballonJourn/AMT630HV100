#ifndef DEVICE__H_
#define DEVICE__H_

#include "awtk.h"
#include "../view_manager.h"

enum set_device_com{
    DEVICE_SN       ,
    DEVICE_VER      ,
    DEVICE_MCU      ,

    DEVICE_NUM_MAX  ,
};

ret_t set_device_view_init(widget_t* parent) ;

void refresh_sn(const char* str) ;

void refresh_ver(const char* str) ;

void refresh_mcu(const char* str) ;

#endif