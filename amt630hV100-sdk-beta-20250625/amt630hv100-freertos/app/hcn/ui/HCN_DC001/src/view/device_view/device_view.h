#ifndef DEVICE_PAGE_H__
#define DEVICE_PAGE_H__

#include "awtk.h"

typedef enum {
    DEVICE_UUID_STATUS,
    DEVICE_UUID,
    DEVICE_BLUETOOTH,
    DEVICE_BLUETOOTH_VER,
    DEVICE_OTA,
    DEVICE_SN,
    DEVICE_VERSION,
    DEVICE_CARBIT,
    DEVICE_COM_NUM_MAX  ,
}device_com;

ret_t device_view_init(widget_t* parent);

ret_t device_refresh_info(device_com component ,const char* data ) ;


#endif