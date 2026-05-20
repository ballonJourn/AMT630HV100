/**
 * @file app_main.c
 * @brief AWTK 入口桩 — LVGL 模式下不编译
 *
 * M022: 此文件是 AWTK 的入口 (awtk_main.inc / assets.inc)
 * 在 LVGL 模式下, 入口由 main_hcn_lvgl.c → application_init() 完成
 * 用 #ifdef AWTK 包裹整个文件使其在 LVGL 模式下不编译
 */

#ifdef AWTK

#include "awtk.h"

BEGIN_C_DECLS
#ifdef AWTK_WEB
#include "assets.inc"
#else
#include "../res/assets.inc"
#endif
END_C_DECLS

extern ret_t application_init(void);
extern ret_t application_exit(void);

#define APP_LCD_ORIENTATION LCD_ORIENTATION_0
#define APP_TYPE APP_SIMULATOR
#define APP_NAME "HCN_DC001"

#include "awtk_main.inc"

#endif /* AWTK */
