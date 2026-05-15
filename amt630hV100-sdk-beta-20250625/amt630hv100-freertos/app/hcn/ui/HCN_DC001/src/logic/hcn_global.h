#ifndef _HCH_GLOBAL_H__
#define _HCH_GLOBAL_H__

#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "awtk.h"

#define APP_MESSAGE_CONTENT_INFO_LEN    (128 )
#define INTERCEPT_artitle_LENGTH      ((size_t)(8 * 3) ) // 8
#define INTERCEPT_lycrile_LENGTH      ((size_t)(18 * 3)) //18

#define KM_CONVERT_MILE (0.62137f)

ret_t global_data_init(const timer_info_t *info) ;

ret_t global_refresh_unit(uint8_t value) ;

ret_t global_refresh_language(uint8_t value) ;

ret_t global_refresh_display(uint8_t value) ;

void global_refresh_mileage();

bool truncate_utf8_string(char* str , int intercept_length) ;

ret_t gloabl_load_image(uint8_t *buff , uint32_t length) ;

#endif