#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "awtk.h"
#include "navigation_view_logic.h"
#include "proxy/mirror_data.h"
#include "view/home_view/navigation_view.h"
#include "view/set_view/device.h"
#include "carlink_cb/hcn_easy_navi.h"
#include "proxy/vehicle_data.h"
#include "proxy/bluetooth_data.h"
#include "proxy/vehicle_argument.h"
#include "view/set_view/bt_connect.h"
#include "vehicle_param/vehicle_param.h"

static hcnNavigationHudInfo g_navigation_info = { 0 };
static bool g_mirror_state      = false ;  
static bool g_mirror_navigation = false ;
static bool g_mirror_url        = false ;

#if ON_PC_CACLE == 0
extern int get_qr_text_buf(char *buf, int len) ;
#endif

void navigation_view_invalidate(void)
{
    /* 
     *     slide_view 设成了 QR_VIEW（因检测到瞬间断连），invalidate
     *     只重置 static 缓存但不改变 widget 状态，必须显式重刷。 */
    g_mirror_state      = false ;
    g_mirror_navigation = false ;
    memset(&g_navigation_info, 0, sizeof(g_navigation_info)) ;

    /* 立即求值并刷新 */
    navigation_view_update() ;
}

void navigation_view_update()
{

    bool  _mirror_state = vehicle_get_mirror_state();
    if (g_mirror_state != _mirror_state)
    {
        g_mirror_state = _mirror_state ;

        if (false == _mirror_state)
        {
            if (vehicle_get_param_carlink_type() == 0) {
                home_refresh_cp_dock_tip(vehicle_get_bluetooth_name()) ;
                home_refresh_nav_view( CP_TIP_VIEW ) ;
            } else {
                home_refresh_nav_view( QR_VIEW )  ;
            }
            // g_mirror_navigation = false       ;
        }
        else
        {
            home_refresh_nav_view( TIPS_VIEW )  ;
        }
    }
    
    if (g_mirror_state)
    {
        bool  _mirror_navigation = vehicle_get_mirror_navigation();
        if (g_mirror_navigation != _mirror_navigation)
        {
            if ( false == _mirror_navigation) {
                home_refresh_nav_view(TIPS_VIEW)  ;
            } else {
                home_refresh_nav_view(NAVI_VIEW)  ;
            }

            g_mirror_navigation = _mirror_navigation ;
        }


        if (g_mirror_navigation)
        {
            const hcnNavigationHudInfo *_navigation_info = vehicle_get_mirror_navi_info() ;

            if ( memcmp(_navigation_info , &g_navigation_info , sizeof(hcnNavigationHudInfo)) )
            {
                if(RET_OK == parse_navigation_data(_navigation_info))
                    memcpy(&g_navigation_info , _navigation_info , sizeof(hcnNavigationHudInfo)) ;
            }
            
        }
        
    }

    update_qr();

    return ;

}

extern bool carlink_ble_mac_addr_is_ready();

void update_qr()
{
    bool _mirror_url = false ;
    if (vehicle_get_param_carlink_type() == 0) {
        _mirror_url = carlink_ble_mac_addr_is_ready() ;
    } else if (vehicle_get_param_carlink_type() == 1) {
        _mirror_url = vehicle_get_mirror_url() ;
    }
    if (g_mirror_url != _mirror_url)
    {
        g_mirror_url = _mirror_url ;
        if (_mirror_url)
        {
            #if ON_PC_CACLE == 0
                if (vehicle_get_param_carlink_type() == 1) {
                    char buff[ 256 ] = { 0 };
                    get_qr_text_buf(buff , sizeof(buff)) ;
                    if (tk_strlen(buff))
                        home_refresh_qr(buff);
                } else {
                    home_refresh_cp_dock_tip(vehicle_get_bluetooth_name()) ;
                }
            #endif

            refresh_bt_name(vehicle_get_bluetooth_name());
            refresh_sn(vehicle_get_uuid());
            refresh_mcu(veicle_get_data_mcu_ver());
            printf("vehicle_get_mirror_url successed to refresh qr\n") ;
        }
        
    }

    /* CarPlay模式：蓝牙改名完成后一次性补刷dock小窗 */
    if (g_mirror_url && vehicle_get_param_carlink_type() == 0) {
        #if ON_PC_CACLE == 0
        if (vehicle_get_data(VEH_BT_NAME_READY) == 1) {
            vehicle_set_data(VEH_BT_NAME_READY, 0);
            home_refresh_cp_dock_tip(vehicle_get_bluetooth_name()) ;
            printf("cp dock tip refreshed on bt name ready\n") ;
        }
        #endif
    }

    return ;
}



ret_t parse_navigation_data(const hcnNavigationHudInfo *_navigation_info)
{   
    if (_navigation_info == NULL) return RET_FAIL ;
    
    char format_buff[128] = {0};

    if (g_navigation_info.naviIcon != _navigation_info->naviIcon)
    {
        tk_snprintf(format_buff , sizeof(format_buff) , "icon_nav_%d", _navigation_info->naviIcon);
        home_refresh_nav_image(format_buff);

        // printf("naviIcon  changed = %d\n" , _navigation_info->naviIcon);
    }
    
    if (g_navigation_info.roadRemainingDistance != _navigation_info->roadRemainingDistance)
    {
        memset(format_buff , 0x0 , sizeof(format_buff));

        if (_navigation_info->roadRemainingDistance > 1000 )
            tk_snprintf(format_buff , sizeof(format_buff) , "%.1f km", (float)((int)(_navigation_info->roadRemainingDistance / 100.0))/10 );
        else
            tk_snprintf(format_buff , sizeof(format_buff) , "%d m", _navigation_info->roadRemainingDistance);
        
        home_refresh_nav_distance(format_buff);

        // printf("roadRemainingDistance  changed = %d\n" , _navigation_info->roadRemainingDistance);
    }
    
    if (strcmp(g_navigation_info.nextRoad, _navigation_info->nextRoad))
    {
        memset(format_buff , 0x0 , sizeof(format_buff));
        tk_snprintf(format_buff , sizeof(format_buff) , "%s", _navigation_info->nextRoad);
        home_refresh_nav_road(format_buff);

        // printf("nextRoad  changed = %s\n" ,_navigation_info->nextRoad);
    }

    return RET_OK ;
}