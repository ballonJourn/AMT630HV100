#include "device_view.h"

static widget_t* device_widget[DEVICE_COM_NUM_MAX] = { NULL };

const char *device_widget_name[DEVICE_COM_NUM_MAX] = { 
    "uuid_status" , "uuid" , "bluetooth" , "bluetooth_ver" , "ota" , "sn" , "version" , "carBit"
};

#define REFRESH_DEVICE_LABEL(component , data)  do {                      \
                if(NULL == data)                                          \
                    return ;                                              \
                                                                          \
                if (device_widget[component])                             \
                    widget_set_text_utf8(device_widget[component] , data);\
            }while (0);


ret_t device_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < DEVICE_COM_NUM_MAX; i++){
        device_widget[i] = widget_lookup(parent, device_widget_name[i], TRUE);
    }
    
    return RET_OK ;
}


void device_refresh_uuid_status(char * data)
{
    REFRESH_DEVICE_LABEL(DEVICE_UUID_STATUS , data) ;
}

void device_refresh_uuid(char * data)
{
    REFRESH_DEVICE_LABEL(DEVICE_UUID , data) ;
}

void device_refresh_bluetooth(char * data)
{
    REFRESH_DEVICE_LABEL(DEVICE_UUID , data) ;
}

ret_t device_refresh_info(device_com component ,const char* data )
{
    if(NULL == data)                                          
        return RET_FAIL;                                              

    if (device_widget[component])                             
        widget_set_text_utf8(device_widget[component] , data);

    return RET_OK;  
}