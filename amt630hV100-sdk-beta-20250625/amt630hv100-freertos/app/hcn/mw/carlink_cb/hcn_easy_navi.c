/**
*
* @file hcn_easy_navi.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 10:11
* @author och
*
*/

#include <FreeRTOS.h>
#include <string.h>
#include "log/hcn_log.h"
#include "carlink_cb/hcn_easy_navi.h"
#include "vehicle_param/vehicle_param.h"

static hcnNavigationHudInfo easy_navi_info;

void parse_easy_navi_info(const hcnNavigationHudInfo *info) {
    if (info == NULL) {
        return;
    }

    int navi_status = info->status ? 0 : 1;
    memset(&easy_navi_info, 0, sizeof(easy_navi_info));

    easy_navi_info.status = info->status;
    easy_navi_info.naviIcon = info->naviIcon;
    easy_navi_info.destinationRemainingDistance = info->destinationRemainingDistance;
    easy_navi_info.roadRemainingDistance = info->roadRemainingDistance;
    easy_navi_info.signalIntensity = info->signalIntensity;
    snprintf(easy_navi_info.currentRoad,sizeof(easy_navi_info.currentRoad), 
            "%s", info->currentRoad);
    snprintf(easy_navi_info.nextRoad, sizeof(easy_navi_info.nextRoad), 
            "%s", info->nextRoad);

    hcn_log_info("parse_easy_navi_info status:%d, naviIcon:%d, destRemainDist:%d, roadRemainDist:%d, signalIntensity:%d, currentRoad:%s, nextRoad:%s\n",
        easy_navi_info.status,
        easy_navi_info.naviIcon,
        easy_navi_info.destinationRemainingDistance,
        easy_navi_info.roadRemainingDistance,
        easy_navi_info.signalIntensity,
        easy_navi_info.currentRoad,
        easy_navi_info.nextRoad);        
    vehicle_set_data(VEH_EASY_NAV_STATUS, navi_status);		
}

hcnNavigationHudInfo *get_easy_navi_info(void) {
    return &easy_navi_info;
} 
