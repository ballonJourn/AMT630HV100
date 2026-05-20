/**
 * @file qr_lvgl.c
 * @brief QR 码 LVGL 渲染层 — 替代 AWTK qr widget
 *
 * M034: 复用 3rd/awtk-widget-qr/src/qr/ 下的纯算法文件 (qrencode.c 等),
 *       新增 LVGL lv_canvas 渲染, 提供 qr_set_value() 兼容 API
 *
 * 调用链:
 *   link_view_logic.c → qr_set_value(widget_qr, url_string)
 *   navigation_view.c → qr_set_value(navi_qr, url_string)
 *
 * 原 AWTK qr.c 中 qr_set_value 的语义:
 *   1. 将 value 字符串用 qrencode 编码为 QRcode 矩阵
 *   2. 在 widget 区域内绘制黑白像素
 *
 * LVGL 实现:
 *   1. qr_set_value(lv_obj_t*, text) → QRcode_encodeString()
 *   2. 在 obj 关联的 canvas buffer 上逐像素绘制
 *   3. lv_obj_invalidate → 触发重绘
 *
 * 内存: canvas buffer 使用静态分配 (最大 QR v10 = 57x57 模块,
 *       放大到 239x235 像素, 每像素 2 字节 RGB565 ≈ 112KB)
 *       在嵌入式中可能偏大, 如果内存紧张可降低为 v5 (37x37)
 *
 * @date  2026-05-20
 */

#include "view/home_view/common.h"
#include "lvgl_compat/widget_registry.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* qrencode 纯算法头文件 (无 AWTK 依赖) */
#include "../3rd/awtk-widget-qr/src/qr/qrencode.h"

/* ======================================================================
 * 静态 canvas buffer
 *
 * link_page QR: 239x235, navi QR: 166x162
 * 取最大值 240x240, RGB565 = 240*240*2 = 115200 bytes
 * 为节省内存, 使用 INDEXED_1BIT (1bpp): 240*240/8 = 7200 bytes
 * 但 LVGL 7.x 的 canvas indexed 模式支持有限, 用 TRUE_COLOR 更安全
 *
 * 折中方案: 用较小的 canvas (QR模块数 × scale),
 * 让 LVGL 自动缩放显示
 * ====================================================================== */

/* QR 最大模块数 (version 10 = 57, 通常 URL 用 v3~v6 即 29~41) */
#define QR_MAX_MODULES  64
/* 每个模块的像素 (scale factor) */
#define QR_PIXEL_SCALE   4
/* canvas 最大像素 */
#define QR_CANVAS_MAX   (QR_MAX_MODULES * QR_PIXEL_SCALE)

/*
 * 静态 buffer — 避免动态分配
 * 大小: QR_CANVAS_MAX * QR_CANVAS_MAX * 2 (RGB565) = 256*256*2 = 131072
 * 如果内存紧张, 可减小 QR_PIXEL_SCALE 到 2 (128*128*2 = 32KB)
 */
static lv_color_t s_qr_cbuf[QR_CANVAS_MAX * QR_CANVAS_MAX];

/* 当前绑定的 canvas 对象 (同一时刻只有一个 QR 活跃) */
static lv_obj_t *s_qr_canvas = NULL;

/* ======================================================================
 * 内部: 将 QRcode 矩阵渲染到 canvas
 * ====================================================================== */

static void qr_render_to_canvas(lv_obj_t *canvas, QRcode *qrcode,
                                 lv_coord_t cw, lv_coord_t ch)
{
    if (qrcode == NULL || canvas == NULL) return;

    int modules = qrcode->width;
    /* 计算 scale: 尽量填满 canvas 区域 */
    int scale_w = cw / modules;
    int scale_h = ch / modules;
    int scale = (scale_w < scale_h) ? scale_w : scale_h;
    if (scale < 1) scale = 1;
    if (scale > QR_PIXEL_SCALE) scale = QR_PIXEL_SCALE;

    int qr_px = modules * scale;
    /* 居中偏移 */
    int off_x = (cw - qr_px) / 2;
    int off_y = (ch - qr_px) / 2;

    /* 清空为白色 */
    lv_canvas_fill_bg(canvas, LV_COLOR_WHITE, LV_OPA_COVER);

    /* 绘制黑色模块 */
    for (int y = 0; y < modules; y++) {
        for (int x = 0; x < modules; x++) {
            /* data[y*width+x] 的最低位表示黑(1)/白(0) */
            if (qrcode->data[y * modules + x] & 0x01) {
                /* 画一个 scale×scale 的黑色方块 */
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        lv_canvas_set_px(canvas,
                                          off_x + x * scale + dx,
                                          off_y + y * scale + dy,
                                          LV_COLOR_BLACK);
                    }
                }
            }
        }
    }

    lv_obj_invalidate(canvas);
}

/* ======================================================================
 * 公共 API: qr_set_value — 兼容 AWTK qr_set_value(widget_t*, const char*)
 *
 * 在 LVGL 模式下:
 *   - 第一次调用时, 将 obj 替换为 lv_canvas (或在 obj 上创建 canvas 子控件)
 *   - 编码字符串 → 渲染到 canvas
 *
 * 注意: AWTK 的 qr_set_value 签名是 ret_t qr_set_value(widget_t*, const char*)
 *       兼容层中 widget_t = lv_obj_t, ret_t = lv_res_t
 * ====================================================================== */

ret_t qr_set_value(widget_t *widget, const char *value)
{
    if (widget == NULL || value == NULL || value[0] == '\0') {
        return RET_FAIL;
    }

    printf("[qr_lvgl] qr_set_value: %.40s%s\n",
           value, strlen(value) > 40 ? "..." : "");

    /* 获取 widget 的实际尺寸 */
    lv_coord_t w = lv_obj_get_width(widget);
    lv_coord_t h = lv_obj_get_height(widget);

    /* 限制 canvas 尺寸不超过 buffer */
    if (w > QR_CANVAS_MAX) w = QR_CANVAS_MAX;
    if (h > QR_CANVAS_MAX) h = QR_CANVAS_MAX;

    /*
     * 策略: 在 widget 内部创建一个 canvas 子控件 (如果还没有)
     * 如果 widget 已经有 canvas 子控件, 复用它
     */
    lv_obj_t *canvas = NULL;

    /* 查找已有的 canvas 子控件 */
    lv_obj_t *child = lv_obj_get_child(widget, NULL);
    while (child != NULL) {
        /* LVGL 7.x 没有直接判断类型的好方法, 用 user_data 标记 */
        if (lv_obj_get_user_data(child) == (lv_obj_user_data_t)(uintptr_t)0xC0DE4051) {
            canvas = child;
            break;
        }
        child = lv_obj_get_child(widget, child);
    }

    if (canvas == NULL) {
        /* 首次: 创建 canvas */
        canvas = lv_canvas_create(widget, NULL);
        lv_obj_set_pos(canvas, 0, 0);
        lv_obj_set_size(canvas, w, h);
        lv_canvas_set_buffer(canvas, s_qr_cbuf, w, h, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_user_data(canvas, (lv_obj_user_data_t)(uintptr_t)0xC0DE4051);
        s_qr_canvas = canvas;
        printf("[qr_lvgl] canvas created %dx%d\n", w, h);
    }

    /* 编码 QR */
    QRcode *qrcode = QRcode_encodeString(value, 0, QR_ECLEVEL_M,
                                          QR_MODE_8, 1);
    if (qrcode == NULL) {
        printf("[qr_lvgl] QRcode_encodeString failed\n");
        lv_canvas_fill_bg(canvas, LV_COLOR_WHITE, LV_OPA_COVER);
        return RET_FAIL;
    }

    printf("[qr_lvgl] QR encoded: version=%d, width=%d modules\n",
           qrcode->version, qrcode->width);

    /* 渲染到 canvas */
    qr_render_to_canvas(canvas, qrcode, w, h);

    /* 释放 QR 数据 */
    QRcode_free(qrcode);

    return RET_OK;
}
