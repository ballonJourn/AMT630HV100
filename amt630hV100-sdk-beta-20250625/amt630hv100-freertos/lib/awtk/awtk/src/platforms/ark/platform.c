/**
 * File:   platform_default.c
 * Author: AWTK Develop Team
 * Brief:  default platform
 *
 * Copyright (c) 2018 - 2021  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-02-21 Li XianJing <xianjimli@hotmail.com> created
 *
 */


#include <time.h>
#include <stdio.h>
#include "base/timer.h"
#include "tkc/platform.h"
#include "tkc/date_time.h"
#include "tkc/mem.h"

#ifdef HMI_AWTK
#include "FreeRTOS_POSIX.h"
#include <rtos.h>
#include <xm_base.h>



static ret_t date_time_get_now_impl(date_time_t* dt) {
  XMSYSTEMTIME wtm;

  memset(dt, 0, sizeof(date_time_t));
  XM_GetLocalTime(&wtm);

  dt->second = wtm.wSecond;
  dt->minute = wtm.wMinute;
  dt->hour = wtm.wHour;
  dt->day = wtm.wDay;
  dt->wday = wtm.wDayOfWeek;
  dt->month = wtm.wMonth;
  dt->year = wtm.wYear;

  return RET_OK;
}

static ret_t date_time_set_now_impl(date_time_t* dt) {
  XMSYSTEMTIME wtm;
  memset(&wtm, 0x00, sizeof(wtm));

  wtm.wMinute = dt->minute;
  wtm.wSecond = dt->second;
  wtm.wHour = dt->hour;
  wtm.wDay = dt->day;
  wtm.wMonth = dt->month;
  wtm.wYear = dt->year;

  if (XM_SetLocalTime(&wtm)) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

uint64_t get_time_ms64() {
  return xTaskGetTickCount();
}

#ifdef HAS_GET_TIME_US64
uint64_t get_time_us64() {
  return get_time_ms64() * 1000;
}
#endif



static const date_time_vtable_t s_date_time_vtable = {
    date_time_get_now_impl,
    date_time_set_now_impl,
    NULL,
    NULL,
};

void sleep_ms(uint32_t ms) {
	OS_Delay(ms);
}

int random(void)
{
	return rand();
}

#ifndef HAS_STD_MALLOC
#ifndef AWTK_HEAP_SIZE
#define AWTK_HEAP_SIZE  (6 * 1024 * 1024)
#endif
static uint32_t s_heap_mem[AWTK_HEAP_SIZE/4];
#endif /*HAS_STD_MALLOC*/

ret_t platform_prepare(void) {
 

#ifndef HAS_STD_MALLOC
  tk_mem_init(s_heap_mem, sizeof(s_heap_mem));
#endif /*HAS_STD_MALLOC*/

  date_time_global_init_ex(&s_date_time_vtable);
  srand(0);

  return RET_OK;
}

#endif
