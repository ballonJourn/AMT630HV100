#ifndef BLUETOOTH_LOGIC_H
#define BLUETOOTH_LOGIC_H

#include "carlink_cb/hcn_carlink_cb.h"


void music_view_update() ;

ret_t parse_music_data(const bt_music_info_t *_music_info);

void home_clean_music_data();
#endif