#ifndef BLUETOOTH_LOGIC_H
#define BLUETOOTH_LOGIC_H

#include "carlink_cb/hcn_carlink_cb.h"

void bluetooth_view_update() ;

void music_view_update() ;

void calling_view_update();

ret_t parse_music_data(const bt_music_info_t *music_info);

ret_t parse_calling_data(const bt_call_t *call_info);

void home_clean_music_data();

#endif