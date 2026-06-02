/**
*
* @file hcn_key_common.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/26 17:28
* @author och
*
*/

#include <stdio.h>
#include "hcn_key_common.h"
#include "vehicle_param/vehicle_param.h"
#include "hcn_cp_keyevent.h"

static key_event_cb_t key_event_cb = NULL;

int set_key_event_cb(key_event_cb_t event_cb) {
    if (!key_event_cb) {
        if (event_cb) {
            key_event_cb = event_cb;
            return 0;
        }
        return -1;
    } else
        return 1;
}

void send_key_event(uint8_t key_event) {
    printf("key cp status:%d %d\r\n", vehicle_get_data(VEH_CARLINK_CP_STATUS), 
    vehicle_get_data(VEH_CARLINK_CP_PHONE_DEV_STATUS));
    if ((vehicle_get_data(VEH_CARLINK_CP_STATUS) == 1)
        && (vehicle_get_data(VEH_CARLINK_CP_PHONE_DEV_STATUS) == 1)) {
         printf("send cp key event directly, event: %d\r\n", key_event);
         send_keyevent_to_cp(key_event);
    } else {
        if (key_event_cb) {
            key_event_cb(key_event);
        }
    }
}