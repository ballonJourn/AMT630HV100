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
#include "view/set_view/bt_connect.h"

static hcnNavigationHudInfo g_navigation_info = { 0 };
static bool g_mirror_state      = false ;  
static bool g_mirror_navigation = false ;
static bool g_mirror_url        = false ;

#if ON_PC_CACLE == 0
extern int get_qr_text_buf(char *buf, int len) ;
#endif

void navigation_view_update()
{

    bool  _mirror_state = vehicle_get_mirror_state();
    if (g_mirror_state != _mirror_state)
    {
        g_mirror_state = _mirror_state ;

        if (false == _mirror_state)
        {
            home_refresh_nav_view( QR_VIEW )  ;
            // g_mirror_navigation = false       ;
        }
        else
        {
            home_refresh_nav_view( TIPS_VIEW )  ;
        }
        
        // printf("mirror_state      changed = %s \n" , _mirror_state ? "open" : "close");
    }
    
    if (g_mirror_state)
    {
        bool  _mirror_navigation = vehicle_get_mirror_navigation();
        if (g_mirror_navigation != _mirror_navigation)
        {
            if ( false == _mirror_navigation)
                home_refresh_nav_view(TIPS_VIEW)  ;
            else
                home_refresh_nav_view(NAVI_VIEW)  ;

            g_mirror_navigation = _mirror_navigation ;

            // printf("mirror_navigation  changed = %s \n" , _mirror_state ? "open" : "close");
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

void update_qr()
{
    bool _mirror_url = vehicle_get_mirror_url() ;
    if (g_mirror_url != _mirror_url)
    {
        g_mirror_url = _mirror_url ;
        if (_mirror_url)
        {
            #if ON_PC_CACLE == 0
                char buff[ 256 ] = { 0 };
                get_qr_text_buf(buff , sizeof(buff)) ;
                if (tk_strlen(buff))
                    home_refresh_qr(buff);
            #endif

            refresh_bt_name(vehicle_get_bluetooth_name());
            refresh_sn(vehicle_get_uuid());
            refresh_mcu(veicle_get_data_mcu_ver());
            printf("vehicle_get_mirror_url successed to refresh qr\n") ;
        }
        
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