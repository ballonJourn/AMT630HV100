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
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "base/widget_vtable.h"
#include "base/window_manager.h"

#include "qr.h"
//#include "../3rd/gtest/googletest/include/gtest/gtest.h"
#define IMAGE_QR_MAX_RATIO 0.45f /* 贴图的面积和二维码的面积最大比例 */

#define MIN_SIZE 21

static ret_t qr_ensure_qrcode(widget_t* widget);

ret_t qr_set_value(widget_t* widget, const char* value) {
  qr_t* qr = QR(widget);
  return_value_if_fail(qr != NULL, RET_BAD_PARAMS);
  
  if (tk_str_eq(qr->value, value)) { return RET_OK; }
  qr->value = tk_str_copy(qr->value, value);

  if (qr->qrcode != NULL) {
    QRcode_free(qr->qrcode);
    qr->qrcode = NULL;
  }
  
  ret_t ret = qr_ensure_qrcode(widget);
  return (ret == RET_OK) ? widget_invalidate(widget, NULL) : RET_FAIL;
}


ret_t qr_set_value_h(widget_t* widget,  char* value) {

  qr_t* qr = QR(widget);
  return_value_if_fail(qr != NULL, RET_BAD_PARAMS);
  
  if (tk_str_eq(qr->value, value)) { return RET_OK; }
  qr->value = tk_str_copy(qr->value, value);

  if (qr->qrcode != NULL) {
    QRcode_free(qr->qrcode);
    qr->qrcode = NULL;
  }
  
  ret_t ret = RET_FAIL;
  int32_t size = tk_min(widget->w, widget->h) - 5;
  if (size < MIN_SIZE) {
    ret = RET_FAIL;
  }
  if (qr->qrcode != NULL) {
    ret = RET_OK;
  }
  qr->qrcode = QRcode_encodeString(qr->value, 3, QR_ECLEVEL_H, QR_MODE_8, 1);
  if (qr->qrcode != NULL) {
    ret = RET_OK;
  }
  return (ret == RET_OK) ? widget_invalidate(widget, NULL) : RET_FAIL;
}


static ret_t qr_get_prop(widget_t* widget, const char* name, value_t* v) {
  qr_t* qr = QR(widget);
  return_value_if_fail(qr != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(WIDGET_PROP_VALUE, name) || tk_str_eq(WIDGET_PROP_TEXT, name)) {
    value_set_str(v, qr->value);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t qr_set_prop(widget_t* widget, const char* name, const value_t* v) {
  qr_t* qr = QR(widget);
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(WIDGET_PROP_VALUE, name) || tk_str_eq(WIDGET_PROP_TEXT, name)) {
    qr_set_value(widget, value_str(v));

    if (qr->qrcode != NULL) {
      QRcode_free(qr->qrcode);
      qr->qrcode = NULL;
    }

    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t qr_on_destroy(widget_t* widget) {
  qr_t* qr = QR(widget);
  return_value_if_fail(widget != NULL && qr != NULL, RET_BAD_PARAMS);

  TKMEM_FREE(qr->value);
  if (qr->qrcode != NULL) {
    QRcode_free(qr->qrcode);
    qr->qrcode = NULL;
  }

  return RET_OK;
}



static ret_t qr_ensure_qrcode(widget_t* widget) {
  qr_t* qr = QR(widget);
  int32_t size = tk_min(widget->w, widget->h) - 5;

  if (size < MIN_SIZE) {
    return RET_FAIL;
  }

  if (qr->qrcode != NULL) {
    return RET_OK;
  }

  qr->qrcode = QRcode_encodeString(qr->value, 3, QR_ECLEVEL_L, QR_MODE_8, 1);

  if (qr->qrcode != NULL) {
    return RET_OK;
  }

  return RET_FAIL;
}

static ret_t qr_paint_logo(widget_t* widget, canvas_t* c) {
  vgcanvas_t* vg = canvas_get_vgcanvas(c);
  vgcanvas_save(vg);
  const char* logo_name = NULL;
  bitmap_t logo = {0};
  return_value_if_fail(widget != NULL && vg != NULL, RET_BAD_PARAMS);

  logo_name = style_get_str(widget->astyle, STYLE_ID_BG_IMAGE, NULL);

  if (logo_name != NULL && widget_load_image(widget, logo_name, &logo) == RET_OK) {
    vgcanvas_translate(vg, c->ox + (widget->w - logo.w) / 2.0f,
                       c->oy + (widget->h - logo.h) / 2.0f);
    vgcanvas_draw_image(vg, &logo, 0, 0, logo.w, logo.h, 0, 0, logo.w, logo.h);
  }
  vgcanvas_restore(vg);
  return RET_OK;
}
#define PRINTF printf
void bitmap_dump(bitmap_t* b) {
  rgba_t rgba;
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t w = b->line_length/4;
  uint32_t h = b->h;

  PRINTF("-----------------------------------------------\n");
  for (y = 0; y < h; y++) {
    PRINTF("%02d:", y);
    for (x = 0; x < w; x++) {
      if (bitmap_get_pixel(b, x, y, &rgba) == RET_OK) {
        //PRINTF("%02x%02x%02x%02x ", rgba.r, rgba.g, rgba.b, rgba.a);
//        if (y = h - 1) {
//          PRINTF("%02x%02x%02x%02x ", rgba.r, rgba.g, rgba.b, rgba.a);
//        }
      }
    }
    PRINTF("\n");
  }
}
static ret_t qr_on_paint_self(widget_t* widget, canvas_t* c) {
  qr_t* qr = QR(widget);
  style_t* style = widget->astyle;
  //uint32_t last_time = get_timer(0);
  if (style != NULL) {
    color_t trans = color_init(0, 0, 0, 0);
    uint32_t margin = style_get_int(style, STYLE_ID_MARGIN, 2);
    color_t bg = style_get_color(style, STYLE_ID_BG_COLOR, trans);
    color_t fg = style_get_color(style, STYLE_ID_FG_COLOR, trans);
	//宽高必须为16的倍数
    uint32_t widget_w = tk_min(widget->w, widget->h);
    while (widget_w % 16) {
      widget_w++;
    }
    widget_resize(widget, widget_w, widget_w);
    if (qr_ensure_qrcode(widget) == RET_OK) {
      //rect_t r = { 0, 0, (int)widget->w, (int)widget->h };
      uint32_t w = tk_min(widget->w, widget->h);
      uint32_t h = tk_min(widget->w, widget->h);

      uint8_t* p = qr->qrcode->data;
      uint32_t size = qr->qrcode->width;
      uint32_t pix_size = (tk_min(widget->w, widget->h) - 2 * margin) / size;
      uint32_t x = 0;
      uint32_t y = 0;
      rect_t r_clip = rect_init(widget->x + (int)(((1 - IMAGE_QR_MAX_RATIO) / 2.0f) * widget->w),
                                widget->y + (int)(((1 - IMAGE_QR_MAX_RATIO) / 2.0f) * widget->h),
                                (int)(IMAGE_QR_MAX_RATIO * widget->w), (int)(IMAGE_QR_MAX_RATIO * widget->h));
      return_value_if_fail(pix_size > 1, RET_BAD_PARAMS); 

      bitmap_t* qr_bitmap =
          bitmap_create_ex(w, h, 0, BITMAP_FMT_BGRA8888);
      int line_length = bitmap_get_line_length(qr_bitmap);
      int margin_left = 0;
      int margin_right = 0;
      if ((line_length - pix_size * size * 4) % 8 == 0){
        margin_left = (line_length - pix_size * size * 4) / 8;
        margin_right = (line_length - pix_size * size * 4) / 8;
	  }else{
        margin_left = (line_length - pix_size * size * 4) / 8;
        margin_right = (line_length - pix_size * size * 4) / 8 + 1;
	  }
      return_value_if_fail(qr_bitmap != NULL, RET_BAD_PARAMS);
      uint8_t* img_data = bitmap_lock_buffer_for_write(qr_bitmap);
      uint32_t* tmp_1;
      tmp_1 = (uint32_t*)img_data;
      uint32_t xx = 0;
      uint32_t yy = 0;
      static int total_size = 0;
      for (y = 0; y < margin_left; y++) {
        for (x = 0; x < w; x++) {
            *tmp_1++ = bg.color;
		}
      }
      for (y = 0; y < size; y++) {
        for (yy = 0; yy < pix_size; yy++) {
          for (x = 0; x < margin_left; x++) {
            *tmp_1++ = bg.color;
          }
          for (x = 0; x < size; x++) {
            for (xx = 0; xx < pix_size; xx++) {
              if (p[(y * size) + x] & 0x01) {
                *tmp_1++ = fg.color;
              } else {
                *tmp_1++ = bg.color;
              }
              total_size++;
            }
          }
          for (x = 0; x < margin_right; x++) {
            *tmp_1++ = bg.color;
          }
        }
      }
      for (y = 0; y < margin_right; y++) {
        for (x = 0; x < w; x++) {
          *tmp_1++ = bg.color;
        }
      }
      bitmap_unlock_buffer(qr_bitmap);
      vgcanvas_t* vg = canvas_get_vgcanvas(c);
      //vgcanvas_save(vg);
      //vgcanvas_translate(vg, c->ox, c->oy);
      vgcanvas_draw_image(vg, qr_bitmap, 0, 0, w, h, c->ox, c->oy, w, h);
      //vgcanvas_restore(vg);
      bitmap_destroy(qr_bitmap);
      widget_paint_with_clip(widget, &r_clip, c, qr_paint_logo);
    }
  }
  //printf("\r\n****** use time = %d ms**********\r\n",(get_timer(0)-last_time)/1000);
  return RET_OK;
}
static ret_t qr_on_event(widget_t* widget, event_t* e) {
  qr_t* qr = QR(widget);
  return_value_if_fail(widget != NULL && qr != NULL, RET_BAD_PARAMS);

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
  return_value_if_fail(qr != NULL, NULL);
  return widget;
}

widget_t* qr_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, qr), NULL);

  return widget;
}
