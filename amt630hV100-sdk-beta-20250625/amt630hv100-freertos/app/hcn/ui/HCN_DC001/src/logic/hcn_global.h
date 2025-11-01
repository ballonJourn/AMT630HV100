#ifndef _HCH_GLOBAL_H__
#define _HCH_GLOBAL_H__

#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include "awtk.h"

#define KM_CONVERT_MILE (0.62137f)

ret_t global_data_init(const timer_info_t *info) ;

ret_t global_refresh_unit(uint8_t value) ;

ret_t global_refresh_language(uint8_t value) ;

// assets_set_global_theme()
#endif