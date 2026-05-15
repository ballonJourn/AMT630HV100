#ifndef HB_COMMON_H
#define HB_COMMON_H

#include "awtk.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SPEED_MAX (199)        //最大速度
#define RPM_MAX (199)          //最大转速
#define POWER_MAX (100)        //最大功率
#define ANGLE_MAX (270)        //最大角度
#define ELECTRI_MAX (100)      //最大续航里程 km


#define STATE_NORMAL  "normal"
#define STATE_SELECTE "selected"

#define HOME_PAGE    "home_page"
#define LINK_PAGE    "link_page"
#define SETTING_PAGE "setting_page"
#define DEVICE_PAGE  "device_page"
#define UPDATE_PAGE  "update_page"

#define BLUETOOTH_MUSIC_IMAGE   "buletooth_music_image"
#define BLUETOOTH_DEFAULT_IMAGE "icon_media_cover_n"

typedef enum {
    KM_H     ,
    MPH      ,
    UNIT_MAX ,
}unit_e;

typedef enum {
    DRV_MODE_E   ,
    DRV_MODE_N   ,
    DRV_MODE_S   ,
    DRV_MODE_MAX ,
}drv_mode_e;

typedef enum {
    GEAR_N   ,
    GEAR_D   ,
    GEAR_R   ,
    GEAR_MAX ,
}gear_e;



#endif