#ifndef HOME_INFO_VIEW_H
#define HOME_INFO_VIEW_H
#include "common.h"
#include <stdint.h>

enum home_info_com{
    INFO_TIME       ,
    INFO_DISTANCE   ,
    INFO_NUM_MAX    ,
};

ret_t home_info_view_init(widget_t* parent);

ret_t home_refresh_info_time(uint64_t total_seconds) ;

ret_t home_refresh_info_distance(uint32_t distance);
#endif