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
    if (key_event_cb) {
        key_event_cb(key_event);
    }
}