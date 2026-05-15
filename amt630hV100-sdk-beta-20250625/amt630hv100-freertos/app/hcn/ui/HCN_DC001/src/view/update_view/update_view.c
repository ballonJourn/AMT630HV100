#include "update_view.h"

const char* update_widget_name[UPDATE_NUM_MAX] = {
    "update_bar" , "lab_state" , "lab_type" , "lab_error" 
} ;

const char* update_state_tr[] = {
    "null" , "updating" , "update_success" , "update_error" , "update_timeout"
};

const char* update_type_str[] = {
    " " ,  "USB-SOC" , "USB-MCU" , "SD-CARD" , "OTA" 
};

const char* update_error_tr[] = {
    "null" ,  "error_crc" , "error_flash" , "error_file_type" , 
};

static widget_t* update_widget[UPDATE_NUM_MAX] = { NULL };

ret_t update_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < UPDATE_NUM_MAX; i++){
        update_widget[i] = widget_lookup(parent, update_widget_name[i], TRUE);
    }
    
    return RET_OK ;
}

ret_t update_refresh_bar(int progress)
{
    progress = tk_min_int(progress , 100);

    if (update_widget[UPDATE_BAR])
    {
        progress_bar_set_value(update_widget[UPDATE_BAR] , progress);
    }
    
    return RET_OK   ;
}

ret_t update_refresh_state(update_state_e state)
{
    if ( state > UPDATE_STATUS_TIMEOUT)
        return RET_FAIL;    
    
    const char *str = locale_info_tr(locale_info(), update_state_tr[state]);

    if (update_widget[UPDATE_STATE])
    {
        widget_set_text_utf8(update_widget[UPDATE_STATE] , str);
    }

    return RET_OK  ;
}

ret_t update_refresh_type(update_type_e type)
{
    if (type > UPDATE_OTA)
        return RET_FAIL;    

    if (update_widget[UPDATE_TYPE])
    {
        widget_set_text_utf8(update_widget[UPDATE_TYPE] ,update_type_str[type]);
    }

    return RET_OK  ;
}

ret_t update_refresh_error(update_error_e error)
{
    if ( error > UPDATE_ERROR_FILE_TYPE )
        return RET_FAIL;    
    
    const char *str = locale_info_tr(locale_info(), update_error_tr[error]);

    if (update_widget[UPDATE_ERROR])
    {
        widget_set_text_utf8(update_widget[UPDATE_ERROR] , str);
    }

    return RET_OK  ;
}