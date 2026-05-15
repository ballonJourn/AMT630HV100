#include "device.h"
#include <stdio.h>
#include "proxy/vehicle_argument.h"

const char* set_device_widget_name[DEVICE_NUM_MAX] = {
    "device_sn" , "device_ver"  ,"device_mcu"
} ;

static widget_t* set_device_widget[DEVICE_NUM_MAX] = { NULL };

ret_t set_device_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < DEVICE_NUM_MAX; i++){
        set_device_widget[i] = widget_lookup(parent, set_device_widget_name[i], TRUE);
    }

    return RET_OK ;
}

void refresh_sn(const char* str)
{
    if (str == NULL || set_device_widget[DEVICE_SN] == NULL )
        return ; 

    widget_set_text_utf8(set_device_widget[DEVICE_SN] , str) ;
}

void refresh_ver(const char* str)
{
    if (str == NULL || set_device_widget[DEVICE_VER] == NULL )
        return ; 

    widget_set_text_utf8(set_device_widget[DEVICE_VER] , str) ;
}

void refresh_mcu(const char* str)
{
    if (str == NULL || set_device_widget[DEVICE_MCU] == NULL )
        return ; 

    widget_set_text_utf8(set_device_widget[DEVICE_MCU] , str) ;
}