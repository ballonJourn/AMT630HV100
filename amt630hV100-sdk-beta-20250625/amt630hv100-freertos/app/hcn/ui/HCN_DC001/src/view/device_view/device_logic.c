#include "device_logic.h"
#include <string.h>
#include <stdlib.h>
#include "device_view.h"
#include "proxy/vehicle_data.h"
#include "proxy/mirror_data.h"
#include "proxy/vehicle_ota.h"
#include "proxy/bluetooth_data.h"
#include "common/navigator.h"

#define REFRESH_INTERVAL_100_MS   (100)

static uint32_t timer_array[REFRESH_TIMER_NUM_MAX] = { 0 } ;

static ret_t timer_refresh_100_ms(const timer_info_t *info) ;

static ret_t on_device_page_changed(void* ctx, event_t* e) ;


ret_t device_init(widget_t *win) 
{
    if (win == NULL) return RET_FAIL ;

    device_view_init(win) ;

    widget_on(win , EVT_WINDOW_CLOSE     , on_device_page_changed ,win);
    widget_on(win , EVT_WINDOW_WILL_OPEN , on_device_page_changed ,win);

    device_timer_init() ;

    return RET_OK ;
}


void device_timer_init()
{
    timer_array[REFRESH_TIMER_100_MS]  = timer_add( timer_refresh_100_ms ,  NULL , REFRESH_INTERVAL_100_MS ) ;

    return ;
}


static ret_t on_device_page_changed(void* ctx, event_t* e)
{
    
    if (e->type == EVT_WINDOW_CLOSE)
    {
        printf("on_device_page_changed EVT_WINDOW_CLOSE\n") ;
        if(timer_array[REFRESH_TIMER_100_MS] != 0 
            && timer_find(timer_array[REFRESH_TIMER_100_MS]))
        {
            timer_remove(timer_array[REFRESH_TIMER_100_MS]) ;
            timer_array[REFRESH_TIMER_100_MS] = 0 ;
            printf("on_device_page_changed timer_remove successed\n");
        }
        vehicle_set_ota_page_state(2);
        // memset(&device_info , 0x00 , sizeof(device_info)) ;
    }
    else if(e->type == EVT_WINDOW_WILL_OPEN)
    {
        printf("on_device_page_changed EVT_WINDOW_WILL_OPEN\n") ;
        vehicle_set_ota_page_state(1);
        timer_refresh_100_ms(NULL);
    
    }

    return RET_OK ;
}

static ret_t timer_refresh_100_ms(const timer_info_t *info)
{
    (void)info ;

    if (vehicle_get_mirror_url() )
    // if(1)
    {
        const char *tr_txt = locale_info_tr(locale_info(), vehicle_get_mirror_activate() ? "Activated" : "Inactive");
        device_refresh_info(DEVICE_UUID_STATUS ,tr_txt ) ;
        
        device_refresh_info(DEVICE_UUID ,vehicle_get_uuid());

        device_refresh_info(DEVICE_BLUETOOTH , vehicle_get_bluetooth_name());
        
        device_refresh_info(DEVICE_BLUETOOTH_VER , vehicle_get_bluetooth_version() );
        

        device_refresh_info(DEVICE_SN ,vehicle_get_uuid());   //序列号用uuid代替

        device_refresh_info(DEVICE_VERSION, veicle_get_data_version());

        device_refresh_info(DEVICE_CARBIT, vehicle_get_carBit_version());
        
        char buff[256] = {0};
        tk_snprintf(buff , sizeof(buff) - 1 , "%s / %s" ,vehicle_get_ota_ssid() , vehicle_get_ota_password() );

        device_refresh_info(DEVICE_OTA , buff) ;

        // device_refresh_info(veicle_get_data_mcu_ver());
        printf("device page info refresh successed \n") ;

        timer_array[REFRESH_TIMER_100_MS] = 0 ;

        return RET_REMOVE ;
    }
    
    
    return RET_REPEAT ;
}



void device_page_deal_key_down ()
{
    // navigator_back_to_home();
}

void device_page_deal_key_up   ()
{
    // navigator_back_to_home();
}

void device_page_deal_key_set  ()
{
    navigator_back_to_home();
}

void device_page_deal_key_back ()
{
    navigator_back_to_home();
}   