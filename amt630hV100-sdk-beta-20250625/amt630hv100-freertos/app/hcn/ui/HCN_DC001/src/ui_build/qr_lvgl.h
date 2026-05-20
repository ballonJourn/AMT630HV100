/**
 * @file qr_lvgl.h
 * @brief QR 码 LVGL 渲染层 — 公共接口
 *
 * M034: 替代 AWTK 3rd/awtk-widget-qr/src/qr/qr.h
 *
 * 提供与 AWTK qr_set_value() 兼容的 API:
 *   ret_t qr_set_value(widget_t* widget, const char* value);
 *
 * 在 LVGL 模式下, widget 参数是 lv_obj_t* (通过兼容层 typedef),
 * 函数内部用 lv_canvas 渲染 QR 码到 widget 区域内。
 *
 * 调用方只需替换 include 路径:
 *   AWTK: #include "../3rd/awtk-widget-qr/src/qr/qr.h"
 *   LVGL: #include "ui_build/qr_lvgl.h"
 *   或在兼容层中统一重定向
 *
 * @date  2026-05-20
 */

#ifndef QR_LVGL_H__
#define QR_LVGL_H__

#include "view/home_view/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 设置 QR 码内容并渲染
 *
 * @param widget  QR 码容器控件 (lv_obj_t*)
 * @param value   要编码的字符串 (URL 等)
 * @return RET_OK 成功, RET_FAIL 失败
 */
ret_t qr_set_value(widget_t *widget, const char *value);

#ifdef __cplusplus
}
#endif

#endif /* QR_LVGL_H__ */
