#include "device_view.h"

static widget_t* device_widget[DEVICE_COM_NUM_MAX] = { NULL };

const char *device_widget_name[DEVICE_COM_NUM_MAX] = { 
    "uuid_status" , "uuid" , "bluetooth" , "bluetooth_ver" , "ota" , "sn" , "version" , "carBit" ,
    "ota_state" ,"progress_circle" ,"lab_state" , "lab_error"
};

const char *ota_state_str[] = { 
    "OTA_no_start" , "OTA_starting" , "OTA_started"
};

extern const char* update_error_tr[] ;
extern const char* update_state_tr[] ;

ret_t device_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < DEVICE_COM_NUM_MAX; i++){
        device_widget[i] = widget_lookup(parent, device_widget_name[i], TRUE);
    }
    
    return RET_OK ;
}

ret_t device_refresh_info(device_com component ,const char* data )
{
    if(NULL == data)                                          
        return RET_FAIL;                                              

    if (device_widget[component])                             
        widget_set_text_utf8(device_widget[component] , data);

    return RET_OK;  
}

//OTA
ret_t device_refresh_bar(int progress)
{
    progress = tk_min_int(progress , 100);

    if (device_widget[DEVICE_OTA_PROGRESS])
    {
        progress_circle_set_value(device_widget[DEVICE_OTA_PROGRESS] , progress);
    }
    
    return RET_OK   ;
}

// ret_t device_refresh_type(update_type_e type)
// {
//     if (type > UPDATE_OTA)
//         return RET_FAIL;    

//     if (device_widget[device_TYPE])
//     {
//         widget_set_text_utf8(device_widget[device_TYPE] ,update_type_str[type]);
//     }

//     return RET_OK  ;
// }

ret_t device_refresh_state(update_state_e state)
{
    if ( state > UPDATE_STATUS_TIMEOUT)
        return RET_FAIL;    
    
    const char *str = locale_info_tr(locale_info(), update_state_tr[state]);

    if (device_widget[DEVICE_OTA_STATE])
    {
        widget_set_text_utf8(device_widget[DEVICE_OTA_STATE] , str);
    }

    return RET_OK  ;
}

ret_t device_refresh_error(update_error_e error)
{
    if ( error > UPDATE_ERROR_FILE_TYPE )
        return RET_FAIL;    
    
    const char *str = locale_info_tr(locale_info(), update_error_tr[error]);

    if (device_widget[DEVICE_OTA_ERROR])
    {
        widget_set_text_utf8(device_widget[DEVICE_OTA_ERROR] , str);
    }

    return RET_OK  ;
}


ret_t device_refresh_ota_state(ota_state_e state)
{
    if ( state > OTA_START_SUCCESSFUL )
        return RET_FAIL;    
    
    const char *str = locale_info_tr(locale_info(), ota_state_str[state]);

    if (device_widget[DEVICE_OTA_START])
    {
        widget_set_text_utf8(device_widget[DEVICE_OTA_START] , str);
    }

    return RET_OK  ;
}