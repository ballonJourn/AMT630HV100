
#ifndef VEHICLE_TIME__H
#define VEHICLE_TIME__H

#include <stdint.h>
#include <stdbool.h>
#include "mw_init/hcn_mw_common.h"

int32_t vehicle_get_time_min();

int32_t vehicle_get_time_hour();

void vehicle_set_time(int hour, int32_t min);

#endif