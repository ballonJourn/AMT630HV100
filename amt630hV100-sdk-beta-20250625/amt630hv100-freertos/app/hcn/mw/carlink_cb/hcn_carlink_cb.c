/**
*
* @file hcn_carlink_cb.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/10 10:45
* @author och
*
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "rtc.h"
#include "carlink_cb/hcn_carlink_cb.h"
#include "carlink_cb/hcn_carlink_provide.h"
#include "storage_param1/hcn_usr_param.h"
#include "log/hcn_log.h"
#include "cJSON.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ECTiny.h"
#include "config/hcn_config.h"
#include "vehicle_param/vehicle_param.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#include "carlink_cb/hcn_easy_navi.h"
#include "carlink_cb/hcn_carlink_task.h"
#include "carlink_cb/hcn_carlink_phone_model.h"

static IhcnCallBack *gHcnCallback = NULL;
 
static void sync_carlink_data_time(void) {
    char time_zone[30];
    char datetime[30];

    vTaskDelay(pdMS_TO_TICKS(100));
    if (EC_queryTime(0, 0, time_zone, sizeof(time_zone), 
                    datetime, sizeof(datetime)) == 0) {
        hcn_log_info("\r\nEC_queryTime dateTime:%s\r\n", datetime);

        ///< format: "18.10.2025 09:54:45"
        char year_str[5] = {datetime[6], datetime[7],
                            datetime[8], datetime[9],'\0'};
        char mon_str[3] = {datetime[3], datetime[4], '\0'};
        char day_str[3] = {datetime[0], datetime[1], '\0'};
        char hour_str[3] = {datetime[11], datetime[12], '\0'};
        char min_str[3] = {datetime[14], datetime[15], '\0'};
        char sec_str[3] = {datetime[17], datetime[18], '\0'};

        int year_t = atoi(year_str);
        int mon_t = atoi(mon_str);  
        int day_t = atoi(day_str);
        int hour_t = atoi(hour_str);
        int min_t = atoi(min_str);
        int sec_t = atoi(sec_str);

        SystemTime_t sys_time;
        memset(&sys_time, 0, sizeof(SystemTime_t)); 
        sys_time.tm_year = year_t;
        sys_time.tm_mon = mon_t;
        sys_time.tm_mday = day_t;
        sys_time.tm_hour = hour_t;
        sys_time.tm_min = min_t;
        sys_time.tm_sec = sec_t;   

        hcn_log_info("Internal time set to: %04d/%02d/%02d %02d:%02d:%02d\n",
                     sys_time.tm_year, sys_time.tm_mon, sys_time.tm_mday,
                     sys_time.tm_hour, sys_time.tm_min, sys_time.tm_sec);
                                        
#ifdef HCN_UART_COMM_ENABLE
        send_mcu_set_time(sys_time);                
#else   
        vSetLocalTime(&sys_time);
#endif
    } else {
        hcn_log_error("EC_queryTime failed!\n");
    }
}

static void parse_weather_json(const char *data) {
    if (data) {
        cJSON *json = NULL;
        cJSON *weather = NULL;
        cJSON *weatherIcon = NULL;
        cJSON *temperature = NULL;
        
        json = cJSON_Parse(data);
        if (NULL == json) {
            hcn_log_error("weather cJSON_Parse error:%s\n",cJSON_GetErrorPtr());
            return ;
        }
        
        weather  = cJSON_GetObjectItem(json, "weather");
        weatherIcon = cJSON_GetObjectItem(json, "weatherIcon");
        temperature = cJSON_GetObjectItem(json, "temperature");
        
        int weathe_icon_value = weatherIcon->valueint;
        int temp_value = atoi(temperature->valuestring);
        
        char weather_status[20] = {0};
        snprintf(weather_status, sizeof(weather_status), "%s", weather->valuestring);

        vehicle_set_data(VEH_QUETY_WEATHER_STATUS, 1);
        vehicle_set_data(VEH_WEATHER_TYPE, weathe_icon_value);
        vehicle_set_data(VEH_ENV_TEMP, temp_value);

        hcn_log_info("weather:%s, icon:%d, temp:%d\n", weather_status, weathe_icon_value, temp_value);

        cJSON_Delete(json);
    }
}

static void onHcnLinkConnect(void) {
    hcn_log_info("\r\nonHcnLinkConnect\r\n");

    ///< 通知EC夜间模式状态
    //EC_uploadNightModeStatus(gDayNight);
    sync_carlink_data_time();
}

static void onHcnVideoStatus(bool status) {
    if (status) {
        vehicle_set_data(VEH_CARLINK_CONNECTED, 1);
#ifdef HCN_CARLINK_ROAD_PIC_ENABLE
        EC_enableDownloadPhoneAppHud(EC_APP_HUD_SUPPORT_FUNCTION_LANE_GUIDANCE_PICTURE |
                                     EC_APP_HUD_SUPPORT_FUNCTION_ROAD_JUNCTION_PICTURE);
#else
        EC_enableDownloadPhoneAppHud(EC_APP_HUD_SUPPORT_FUNCTION_DEFAULT); 
#endif
    } else {
        vehicle_set_data(VEH_CARLINK_CONNECTED, 0);
        vehicle_set_data(VEH_QUETY_WEATHER_STATUS, 0);

        EC_disableDownloadPhoneAppHud(); 
    }
}

static void onHcnLicenseStatus(bool status) {
    hcn_log_info("onHcnLicenseStatus status:%d\n", status);

    uint8_t uuid_status = 0;
    if (get_hcn_usr_param(HCN_PARAM_UUID_REGISTER, &uuid_status)) {
        if (status) {
            if (uuid_status == 0) {
                uuid_status = 1;
                set_hcn_usr_param(HCN_PARAM_UUID_REGISTER, &uuid_status);
                save_hcn_usr_param();
            } 
        } else {
            if (uuid_status == 1) {
                uuid_status = 0;
                set_hcn_usr_param(HCN_PARAM_UUID_REGISTER, &uuid_status);
                save_hcn_usr_param();
            }
        }

        vehicle_set_data(VEH_LICENSE_AUTH_STATUS, (int)status);
    } else {
        hcn_log_error("get uuid register status failed!\n");
    }
}

static void onHcnWeatherReceived(const char *weather_json) {
    if (weather_json) {
        parse_weather_json(weather_json);
    }
}

static void onHcnEasyNavigation(const hcnNavigationHudInfo * naviData) {
    if (naviData) {
        parse_easy_navi_info(naviData);
    }
}

static void onHcnPhoneAppHUDLaneGuidancePicture(
    const road_junction_pic_t * data) {
    if (data) {
#ifdef HCN_CARLINK_ROAD_PIC_ENABLE
        parse_lane_guidance_pic_info(data);
#endif
    }
}

static void onHcnPhoneAppHUDRoadJunctionPicture(
    const road_junction_pic_t* data) {
    if (data) {
#ifdef HCN_CARLINK_ROAD_PIC_ENABLE
        parse_road_junction_pic_info(data);
#endif
    }
}

static void onHcnPhoneModel(const char * phoneInfo) {
    if (phoneInfo) {
        parse_phone_model_info(phoneInfo);
    }
}

static IhcnCallBack *register_hcn_callback(void) {
    IhcnCallBack *callback = (IhcnCallBack*)malloc(sizeof(IhcnCallBack));
    memset(callback, 0, sizeof(IhcnCallBack));

    callback->onHcnVideoStatus = onHcnVideoStatus;
    callback->onHcnLicenseStatus = onHcnLicenseStatus;
    callback->onHcnLinkConnect = onHcnLinkConnect;
    callback->onHcnWeatherReceived = onHcnWeatherReceived;
    callback->onHcnEasyNavigation = onHcnEasyNavigation;
    callback->onHcnPhoneAppHUDLaneGuidancePicture = \
                onHcnPhoneAppHUDLaneGuidancePicture;
    callback->onHcnPhoneAppHUDRoadJunctionPicture = \
                onHcnPhoneAppHUDRoadJunctionPicture;
    callback->onHcnPhoneModel = onHcnPhoneModel;
    
    return callback;
}

void unregister_hcn_callback(IhcnCallBack *callback) {
    if (callback) {
        free(callback);
        callback = NULL;
    }
}

void carlink_cb_init(void) {
    static bool inited = false;

    if (inited) {
        hcn_log_info("carlink callback has inited!\n");
        return;
    }

    if (!inited) {  
        HcnLibConfig hcn_cfg;
        memset(&hcn_cfg, 0, sizeof(HcnLibConfig));

        gHcnCallback = register_hcn_callback();
        hcn_initialize(&hcn_cfg, gHcnCallback);

#ifdef HCN_CARLINK_WEATHER_ENABLE
        carlink_query_init();
#endif
        carlink_easy_navi_init();
        
        inited = true;
    }
}

