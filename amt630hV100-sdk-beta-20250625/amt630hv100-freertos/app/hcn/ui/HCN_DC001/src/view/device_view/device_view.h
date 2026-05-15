#ifndef DEVICE_PAGE_H__
#define DEVICE_PAGE_H__

#include "awtk.h"
#include "ota_manage/hcn_ota.h"

typedef enum {
    DEVICE_UUID_STATUS,
    DEVICE_UUID,
    DEVICE_BLUETOOTH,
    DEVICE_BLUETOOTH_VER,
    DEVICE_OTA,
    DEVICE_SN,
    DEVICE_VERSION,
    DEVICE_CARBIT,
    //OTA
    DEVICE_OTA_START ,
    DEVICE_OTA_PROGRESS,
    DEVICE_OTA_STATE ,
    DEVICE_OTA_ERROR ,

    DEVICE_COM_NUM_MAX  ,
}device_com;

typedef enum {
    OTA_NO_START ,
    OTA_STARTING ,
    OTA_START_SUCCESSFUL ,
}ota_state_e;

ret_t device_view_init(widget_t* parent);

ret_t device_refresh_info(device_com component ,const char* data ) ;

//OTA
ret_t device_refresh_bar(int progress) ;

ret_t device_refresh_state(update_state_e state) ;

ret_t device_refresh_error(update_error_e error) ;

ret_t device_refresh_ota_state(ota_state_e state) ;

#endif