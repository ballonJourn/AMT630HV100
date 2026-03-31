#ifndef VEHICLE_OTA__H
#define VEHICLE_OTA__H

#include <stdint.h>
#include <stdbool.h>
#include "ota_manage/hcn_ota.h"

bool vehicle_get_uptate_state() ;

update_info_t* vehicle_get_uptate_info() ;

const char* vehicle_get_ota_ssid();

const char* vehicle_get_ota_password();

#endif