
#ifndef CARLINL_DATA_H
#define CARLINL_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "carlink_cb/hcn_easy_navi.h"

/// @brief 手机投屏是否连接
/// @return 
bool vehicle_get_mirror_state();


/// @brief url地址是否就绪
/// @return 
bool vehicle_get_mirror_url() ;


/// @brief 是否开启导航
/// @return 
bool vehicle_get_mirror_navigation() ;


/// @brief 是否激活
/// @return 
bool vehicle_get_mirror_activate() ;


/// @brief uuid
/// @return 
const char* vehicle_get_uuid() ;


/// @brief 亿连版本信息
/// @return 
const char* vehicle_get_carBit_version();


/// @brief 获取简易导航信息
/// @return 
const hcnNavigationHudInfo *vehicle_get_mirror_navi_info() ;

#endif