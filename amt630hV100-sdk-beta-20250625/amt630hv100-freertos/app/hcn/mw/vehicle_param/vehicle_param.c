/**
*
* @file vehicle_param.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/05 10:14
* @author och
*
*/

#include <stddef.h>
#include <stdbool.h>
#include "log/hcn_log.h"
#include "vehicle_param/vehicle_param.h"

#define VEH_INVALID_DATA INT32_MIN

static int32_t veh_data[VEH_DATA_END] = {0};

static bool is_valid_id(veh_data_e id) {
    return (id < VEH_DATA_END);
}

int32_t vehicle_get_data(veh_data_e id) {
    if (is_valid_id(id)) {
        return veh_data[id];
    } else {
        hcn_log_error("Attempt to retrieve data for invalid ID: %u", id);
        return VEH_INVALID_DATA;
    }
}

void vehicle_set_data(veh_data_e id, int32_t value) {
    if (!is_valid_id(id)) {
        hcn_log_error("Attempt to set data for invalid ID: %u", id);
        return;
    }

    if (veh_data[id] != value) {
        veh_data[id] = value;
    }
}
