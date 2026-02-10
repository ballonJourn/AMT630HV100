#include "vehicle_ota.h"
#include "ota_manage/hcn_ota.h"
#include "carlink_cb/hcn_carlink_cb.h"

bool vehicle_get_uptate_state()
{
    update_info_t * update_info = get_current_update_info();
    if (update_info)
    {
        if (update_info->type <= UPDATE_OTA && update_info->type != UPDATE_OTA)
            return update_info->type == UPDATE_NONE ? false : true ;
    }
    return false ;
    
}

update_info_t* vehicle_get_uptate_info()
{
    return get_current_update_info();
}


const char* vehicle_get_ota_ssid()
{
    return hcn_get_ota_ssid();
}

const char* vehicle_get_ota_password()
{
    return hcn_get_ota_ap_pwd();
}