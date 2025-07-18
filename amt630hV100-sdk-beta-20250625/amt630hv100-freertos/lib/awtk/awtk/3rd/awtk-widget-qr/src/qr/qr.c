/**
 * File:   qr.c
 * Author: AWTK Develop Team
 * Brief:  显示二维码的控件
 *
 * Copyright (c) 2020 - 2020 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2020-06-01 Li XianJing <xianjimli@hotmail.com> created
 * 2023-01-16 Tiniration modified for libqrencode v4.1.1
 * 2023-02-01 Tiniration modified for bitmap cache
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "base/widget_vtable.h"

#include "qr.h"

static ret_t qr_ensure_qrcode(widget_t* widget);

ret_t qr_set_value(widget_t* widget, const char* value) {
    qr_t* qr = QR(widget);
    return_value_if_fail(qr, RET_BAD_PARAMS);
    
    if (!value || tk_str_eq(qr->value, value)) return RET_FAIL;
    if (qr->value) {
        tk_free(qr->value);
        qr->value = NULL;
    }
    qr->value = tk_strdup(value);

    if (qr->qrcode) {
        QRcode_free(qr->qrcode);
        qr->qrcode = NULL;
    }

    if (qr->bmp) {
        bitmap_destroy(qr->bmp);
        qr->bmp = NULL;
    }

    return (qr_ensure_qrcode(widget) == RET_OK) ? widget_invalidate(widget, NULL) : RET_FAIL;
}

static ret_t qr_get_prop(widget_t* widget, const char* name, value_t* v) {
    qr_t* qr = QR(widget);
    return_value_if_fail(qr && name && v, RET_BAD_PARAMS);

    if (tk_str_eq(WIDGET_PROP_VALUE, name) || tk_str_eq(WIDGET_PROP_TEXT, name)) {
        value_set_str(v, qr->value);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

static ret_t qr_set_prop(widget_t* widget, const char* name, const value_t* v) {
    qr_t* qr = QR(widget);
    return_value_if_fail(widget && name && v, RET_BAD_PARAMS);

    if (tk_str_eq(WIDGET_PROP_VALUE, name) || tk_str_eq(WIDGET_PROP_TEXT, name)) {
        if (RET_OK == qr_set_value(widget, value_str(v)) && qr->qrcode) {
            QRcode_free(qr->qrcode);
            qr->qrcode = NULL;
        }

        return RET_OK;
    }

    return RET_NOT_FOUND;
}

static ret_t qr_on_destroy(widget_t* widget) {
    qr_t* qr = QR(widget);
    return_value_if_fail(widget && qr, RET_BAD_PARAMS);

    TKMEM_FREE(qr->value);
    if (qr->qrcode) {
        QRcode_free(qr->qrcode);
        qr->qrcode = NULL;
    }

    if (qr->bmp) {
        bitmap_destroy(qr->bmp);
        qr->bmp = NULL;
    }

    return RET_OK;
}

#define MIN_SIZE 21

static ret_t qr_process_bitmap(widget_t* widget) {
    qr_t* qr = QR(widget);
    style_t* style = widget->astyle;
    int w = qr->qrcode->width;
    int h = qr->qrcode->width;

    return_value_if_fail(style && w && h, RET_FAIL);
    if (qr->bmp) bitmap_destroy(qr->bmp);

    uint32_t margin = style_get_int(style, STYLE_ID_MARGIN, 2);
    uint32_t pix_size = (tk_min(widget->w, widget->h) - 2 * margin) / w;
    int dw = w * pix_size;
    int dh = h * pix_size;
    qr->bmp = bitmap_create_ex(dw, dh, dw * 4, BITMAP_FMT_RGBA8888);
    if (!qr->bmp) {
        log_warn("oom for bitmap_create_ex");
        return RET_FAIL;
    }

    uint32_t* pd = (uint32_t*)bitmap_lock_buffer_for_write(qr->bmp);
    uint8_t* pIn = qr->qrcode->data;
    uint32_t a_w = (qr->bmp->line_length >> 2);    // align the width for 16
    int size = w * h;                              // the total of qrcode pixel data
    for (int i = 0; i < size; ++i) {
        int row_x = (i / h);    // the row index
        int col_y = (i % w);    // the col index
        // white and black, color_order is [argb]
        uint32_t tmp = ((*pIn++ & 0x01)) ? (0xff000000) : (0xffffffff);
        for (int r_j = 0; r_j < pix_size; ++r_j) {
            for (int c_k = 0; c_k < pix_size; ++c_k) {
                // row index * width + col index
                pd[(row_x * pix_size + r_j) * a_w + (c_k + pix_size * col_y)] = tmp;
            }
        }
    }

    bitmap_unlock_buffer(qr->bmp);
    return RET_OK;
}

static ret_t qr_ensure_qrcode(widget_t* widget) {
    qr_t* qr = QR(widget);
    int32_t size = tk_min(widget->w, widget->h) - 5;

    return_value_if_fail(size >= MIN_SIZE, RET_FAIL);
    // if found cache, use the cache
    if (qr->qrcode || qr->bmp) {
        return RET_OK;
    }

    qr->qrcode = QRcode_encodeString(qr->value, 4, QR_ECLEVEL_Q, QR_MODE_8, 1);
    return (qr->qrcode) ? (qr_process_bitmap(widget)) : (RET_FAIL);
}

static ret_t qr_on_paint_self(widget_t* widget, canvas_t* c) {
    if (qr_ensure_qrcode(widget) == RET_OK) {
        bitmap_t* bmp = QR(widget)->bmp;
        rect_t r_wd = {4, 4, (widget->w)-8, (widget->h)-8};
        rect_t r_qd = {0, 0, bmp->w, bmp->h};
        canvas_draw_image(c, bmp, &r_qd, &r_wd);
    }

    return RET_OK;
}

static ret_t qr_on_event(widget_t* widget, event_t* e) {
    qr_t* qr = QR(widget);
    return_value_if_fail(widget && qr, RET_BAD_PARAMS);

    (void)qr;

    return RET_OK;
}

const char* s_qr_properties[] = {WIDGET_PROP_VALUE, NULL};

TK_DECL_VTABLE(qr) = {.size = sizeof(qr_t),
                      .type = WIDGET_TYPE_QR,
                      .clone_properties = s_qr_properties,
                      .persistent_properties = s_qr_properties,
                      .parent = TK_PARENT_VTABLE(widget),
                      .create = qr_create,
                      .on_paint_self = qr_on_paint_self,
                      .set_prop = qr_set_prop,
                      .get_prop = qr_get_prop,
                      .on_event = qr_on_event,
                      .on_destroy = qr_on_destroy};

widget_t* qr_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
    widget_t* widget = widget_create(parent, TK_REF_VTABLE(qr), x, y, w, h);
    qr_t* qr = QR(widget);
    return_value_if_fail(qr, NULL);
    qr->bmp = NULL;
    qr->value = NULL;
    qr->qrcode = NULL;

    return widget;
}

widget_t* qr_cast(widget_t* widget) {
    return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, qr), NULL);

    return widget;
}
