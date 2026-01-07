
#include "vehicle_data.h"
#include "view/home_view/common.h"
#include "vehicle_param/vehicle_param.h"
#include "carlink_cb/hcn_easy_navi.h"
#include "carlink_cb/hcn_carlink_cb.h"

bool vehicle_get_mirror_state()
{
#if !ON_PC_CACLE
    int32_t state = vehicle_get_data(VEH_CARLINK_CONNECTED) ;
    return state ? true : false ;
#endif

    return false ; 
}

bool vehicle_get_mirror_url()
{
#if !ON_PC_CACLE
    int32_t state = vehicle_get_data(VEH_CARLINK_URL_STATUS) ;
    return state ? true : false ;
#endif

    return false ; 
}

bool vehicle_get_mirror_navigation()
{
#if !ON_PC_CACLE
    int32_t state = vehicle_get_data(VEH_EASY_NAV_STATUS) ;
    return state ? true : false ;
#endif

    return false ; 
}

bool vehicle_get_mirror_activate()
{
#if !ON_PC_CACLE
    int32_t state = vehicle_get_data(VEH_LICENSE_AUTH_STATUS) ;
    return state ? true : false ;
#endif

    return false ; 
}

const hcnNavigationHudInfo *vehicle_get_mirror_navi_info()
{
   return  get_easy_navi_info();
}

const char* vehicle_get_uuid()
{
    return hcn_ec_get_uuid() ;
}

const char* vehicle_get_carBit_version()
{
    return hcn_ec_get_version() ;
}