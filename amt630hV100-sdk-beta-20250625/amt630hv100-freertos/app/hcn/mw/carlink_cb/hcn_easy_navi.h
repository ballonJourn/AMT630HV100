/**
*
* @file hcn_easy_navi.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 10:12
* @author och
*
*/
#ifndef __HCN_EASY_NAVI_H__
#define __HCN_EASY_NAVI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>    
#include "carlink_cb/hcn_carlink_cb.h"
#include "config/hcn_config.h"

const hcnNavigationHudInfo *get_easy_navi_info(void);

void parse_easy_navi_info(const hcnNavigationHudInfo *info);
#ifdef HCN_CARLINK_ROAD_PIC_ENABLE
void parse_lane_guidance_pic_info(const road_junction_pic_t *info);
void parse_road_junction_pic_info(const road_junction_pic_t *info);
#endif

int carlink_easy_navi_init(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // HCN_EASY_NAVI_H__