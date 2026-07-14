/**
*
* @file hcn_carlink_task.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 12:19
* @author och
*
*/

#include <FreeRTOS.h>
#include "task.h"
#if CARLINK_EC
#include "ECTiny.h"
#endif
#include "carlink_cb/hcn_carlink_task.h"
#include "vehicle_param/vehicle_param.h"
#include "log/hcn_log.h"
#include "carlink_cb/hcn_carlink_cb.h"

#ifdef HCN_CARLINK_WEATHER_ENABLE

#define QUERY_EC_PERIOD  (200) ///< 200MS
#define QUERY_WEATHER_INTERVAL_TIME         (30) ///< 天气查询间隔时间，单位min，30min
#define QUERY_GPS_INTERVAL_TIME             (15) ///< gps海拔查询间隔时间，单位s，10s
#define QUERY_ALTITUDE_ARRAY_SIZE           (5)  ///< 查询海拔时，数组buffer大小
#define ALTITUDE_FLOAT_RANGE                (30) ///< 同一个位置，海拔浮动小于30m
#define QUERY_WEATHER_INTERVAL_PERIOD ((size_t)((QUERY_WEATHER_INTERVAL_TIME *60*1000) / QUERY_EC_PERIOD))
#define QUERY_GPS_INTERVAL_PERIOD  ((size_t)(QUERY_GPS_INTERVAL_TIME*1000)/(QUERY_ALTITUDE_ARRAY_SIZE * QUERY_EC_PERIOD))

static void query_process(void) {
    static int query_count = 0;

    if (vehicle_get_data(VEH_CARLINK_CONNECTED)) {
        if (query_count % QUERY_WEATHER_INTERVAL_PERIOD == 0) {
            if (EC_queryWeather() != 0) {
                hcn_log_error("EC_queryWeather failed!\n");
            } 
        }

        if (query_count % QUERY_GPS_INTERVAL_PERIOD == 0) {
            ///< to do gps altitude query
            ///< int32_t EC_queryGPS(uint32_t* status, ECGPSInfo* gps)
        }

        query_count++;
    } else {
        if (query_count != 0) {
            query_count = 0;
        }
    }
}

static void query_ec_thread(void *param) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(QUERY_EC_PERIOD));
        query_process();
    }
}

void carlink_query_init(void) {
    if (xTaskCreate(query_ec_thread, "query_ec_thread", configMINIMAL_STACK_SIZE,
                    NULL, configMAX_PRIORITIES / 4, NULL) != pdPASS) {
        hcn_log_error("create query_ec_thread fail.\n");
        return;
    }
}

#endif