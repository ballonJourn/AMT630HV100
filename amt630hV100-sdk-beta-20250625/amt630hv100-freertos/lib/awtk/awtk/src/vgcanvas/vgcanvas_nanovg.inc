#include "nanovg.h"
#include "tkc/mem.h"
#include "tkc/utf8.h"
#include "base/events.h"
#include "base/vgcanvas.h"
#include "base/system_info.h"
#include "base/image_manager.h"
#include "base/font_manager.h"
#include "base/assets_manager.h"
#include "base/vgcanvas_asset_manager.h"

static int vgcanvas_nanovg_ensure_image(vgcanvas_nanovg_t* canvas, bitmap_t* img);
static int vgcanvas_nanovg_ensure_sub_image(vgcanvas_nanovg_t* canvas, bitmap_t* img, int x, int y, int w, int h);
static void vgcanvas_nanovg_free_sub_image(vgcanvas_nanovg_t* canvas, int image);

static ret_t vgcanvas_nanovg_destroy(vgcanvas_t* vgcanvas);
static ret_t vgcanvas_nanovg_end_frame(vgcanvas_t* vgcanvas);
static ret_t vgcanvas_nanovg_set_antialias(vgcanvas_t* vgcanvas, bool_t value);
static ret_t vgcanvas_nanovg_destroy_fbo(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo);
static ret_t vgcanvas_nanovg_reinit(vgcanvas_t* vg, uint32_t w, uint32_t h, uint32_t stride,
                                    bitmap_format_t format, void* data);
static ret_t vgcanvas_nanovg_begin_frame(vgcanvas_t* vgcanvas, const dirty_rects_t* dirty_rects);
static ret_t vgcanvas_nanovg_create_fbo(vgcanvas_t* vgcanvas, uint32_t w, uint32_t h,
                                        bool_t custom_draw_model, framebuffer_object_t* fbo);
static ret_t vgcanvas_nanovg_bind_fbo(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo);
static ret_t vgcanvas_nanovg_unbind_fbo(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo);
static ret_t vgcanvas_nanovg_fbo_to_bitmap(vgcanvas_t* vgcanvas, framebuffer_object_t* fbo,
                                           bitmap_t* img, const rect_t* r);

static ret_t vgcanvas_nanovg_on_asset_events(void* ctx, event_t* e) {
  assets_event_t* evt = (assets_event_t*)e;

  if (evt->type == ASSET_TYPE_FONT) {
    asset_info_t* info = evt->asset_info;
    if (e->type == EVT_ASSET_MANAGER_CLEAR_CACHE) {
      vgcanvas_asset_manager_remove_font(vgcanvas_asset_manager(), ctx, NULL);
    } else if (e->type == EVT_ASSET_MANAGER_UNLOAD_ASSET) {
      vgcanvas_asset_manager_remove_font(vgcanvas_asset_manager(), ctx, info->name);
    }
  }

  return RET_OK;
}

static ret_t vgcanvas_nanovg_init(vgcanvas_t* vgcanvas) {
  (void)vgcanvas;
  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_assets_manager(vgcanvas_t* vg, assets_manager_t* assets_manager) {
  return_value_if_fail(vg != NULL, RET_BAD_PARAMS);

  if (vg->assets_manager != NULL && vg->assets_manager != assets_manager) {
    emitter_off_by_ctx(EMITTER(vg->assets_manager), vg);
  }
  if (assets_manager != NULL && vg->assets_manager != assets_manager) {
    emitter_t* emitter = EMITTER(assets_manager);
    emitter_on(emitter, EVT_ASSET_MANAGER_CLEAR_CACHE, vgcanvas_nanovg_on_asset_events, vg);
    emitter_on(emitter, EVT_ASSET_MANAGER_UNLOAD_ASSET, vgcanvas_nanovg_on_asset_events, vg);
  }
  vg->assets_manager = assets_manager;
  return RET_OK;
}

static ret_t vgcanvas_nanovg_deinit(vgcanvas_t* vgcanvas) {
  emitter_t* emitter = EMITTER(assets_manager());
  emitter_off_by_ctx(emitter, vgcanvas);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_reset(vgcanvas_t* vgcanvas) {
  /*TODO: unload fonts*/
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  nvgDeleteFontByName(vg, NULL);
  vgcanvas_asset_manager_remove_font(vgcanvas_asset_manager(), vgcanvas, NULL);
  return RET_OK;
}

static ret_t vgcanvas_nanovg_reset_curr_state(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  nvgReset(vg);
  return RET_OK;
}

static ret_t vgcanvas_nanovg_flush(vgcanvas_t* vgcanvas) {
#if defined( WITH_NANOVG_GPU) || defined(WITH_VG_GPU)
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  nvgEndFrame(vg);
#endif
  return RET_OK;
}

static NVGcolor toNVGcolor(color_t c);
static ret_t vgcanvas_nanovg_clear_rect(vgcanvas_t* vgcanvas, float_t x, float_t y, float_t w,
                                        float_t h, color_t c) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  color_t fill_color = vgcanvas->fill_color;
 #ifdef WITH_NANOVG_VG
  if(nvgFastClear (vg, x, y, w, h, toNVGcolor(c)) == 0)
	return RET_OK;
 #endif
  vgcanvas_set_fill_color(vgcanvas, c);
  nvgBeginPath(vg);
  nvgRect(vg, x, y, w, h);
  nvgClosePath(vg);
  nvgFill(vg);
  vgcanvas_set_fill_color(vgcanvas, fill_color);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_begin_path(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgBeginPath(vg);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_move_to(vgcanvas_t* vgcanvas, float_t x, float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgMoveTo(vg, x, y);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_line_to(vgcanvas_t* vgcanvas, float_t x, float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgLineTo(vg, x, y);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_quad_to(vgcanvas_t* vgcanvas, float_t cpx, float_t cpy, float_t x,
                                     float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgQuadTo(vg, cpx, cpy, x, y);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_bezier_to(vgcanvas_t* vgcanvas, float_t cp1x, float_t cp1y,
                                       float_t cp2x, float_t cp2y, float_t x, float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgBezierTo(vg, cp1x, cp1y, cp2x, cp2y, x, y);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_arc_to(vgcanvas_t* vgcanvas, float_t x1, float_t y1, float_t x2,
                                    float_t y2, float_t r) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgArcTo(vg, x1, y1, x2, y2, r);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_arc(vgcanvas_t* vgcanvas, float_t x, float_t y, float_t r,
                                 float_t start, float_t end, bool_t ccw) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgArc(vg, x, y, r, start, end, ccw ? NVG_CCW : NVG_CW);

  return RET_OK;
}

static bool_t vgcanvas_nanovg_is_point_in_path(vgcanvas_t* vgcanvas, float_t x, float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  (void)vg;
  (void)x;
  (void)y;

  /*TODO*/

  return FALSE;
}

static ret_t vgcanvas_nanovg_rotate(vgcanvas_t* vgcanvas, float_t rad) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgRotate(vg, rad);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_scale(vgcanvas_t* vgcanvas, float_t x, float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgScale(vg, x, y);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_translate(vgcanvas_t* vgcanvas, float_t x, float_t y) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgTranslate(vg, x, y);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_transform(vgcanvas_t* vgcanvas, float_t a, float_t b, float_t c,
                                       float_t d, float_t e, float_t f) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgTransform(vg, a, b, c, d, e, f);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_transform(vgcanvas_t* vgcanvas, float_t a, float_t b, float_t c,
                                           float_t d, float_t e, float_t f) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgResetTransform(vg);
  nvgTransform(vg, a, b, c, d, e, f);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_rounded_rect(vgcanvas_t* vgcanvas, float_t x, float_t y, float_t w,
                                          float_t h, float_t r) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgRoundedRect(vg, x, y, w, h, r);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_ellipse(vgcanvas_t* vgcanvas, float_t x, float_t y, float_t rx,
                                     float_t ry) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgEllipse(vg, x, y, rx, ry);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_close_path(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgClosePath(vg);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_path_winding(vgcanvas_t* vgcanvas, bool_t dir) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgPathWinding(vg, dir);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_intersect_clip_rect(vgcanvas_t* vgcanvas, float_t* x, float_t* y,
                                                 float_t* w, float_t* h) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgIntersectScissor_ex(vg, x, y, w, h);
  nvgGetCurrScissor(vg, &vgcanvas->clip_rect.x, &vgcanvas->clip_rect.y, &vgcanvas->clip_rect.w, &vgcanvas->clip_rect.h);
  return RET_OK;
}

#if defined(WITH_NANOVG_GPU) || defined(WITH_VG_GPU)
#define CMP_FLOAT_QE(a, b, CMP_ACCURACY) \
  (-CMP_ACCURACY <= ((a) - (b)) && ((a) - (b)) <= CMP_ACCURACY)

static bool_t vgcanvas_nanovg_is_system_orientation_0_or_180(float_t xform[6]) {
  float_t sin_180 = sinf(TK_D2R(180));
  system_info_t* info = system_info();

  if (info->lcd_orientation == LCD_ORIENTATION_0 && xform[0] == 1.0f && xform[1] == 0.0f &&
      xform[2] == 0.0f && xform[3] == 1.0f) {
    return TRUE;
  } else if (info->lcd_orientation == LCD_ORIENTATION_180 && xform[0] == -1.0f &&
             CMP_FLOAT_QE(xform[1], sin_180, 0.0001f) && CMP_FLOAT_QE(xform[2], sin_180, 0.0001f) &&
             xform[3] == -1.0f) {
    return TRUE;
  }
  return FALSE;
}

static bool_t vgcanvas_nanovg_is_system_orientation_90_or_270(float_t xform[6]) {
  float_t cos_90 = cosf(TK_D2R(90));
  system_info_t* info = system_info();

  if (CMP_FLOAT_QE(xform[0], cos_90, 0.0001f) && CMP_FLOAT_QE(xform[3], cos_90, 0.0001f)) {
    if (info->lcd_orientation == LCD_ORIENTATION_90 && xform[1] == 1.0f && xform[2] == -1.0f) {
      return TRUE;
    } else if (info->lcd_orientation == LCD_ORIENTATION_270 && xform[1] == -1.0f &&
               xform[2] == 1.0f) {
      return TRUE;
    }
  }
  return FALSE;
}

#endif

static ret_t vgcanvas_nanovg_clip_rect(vgcanvas_t* vgcanvas, float_t x, float_t y, float_t w,
                                       float_t h) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

#if defined(WITH_NANOVG_GPU) || defined(WITH_VG_GPU)
  rect_t r1;
  rect_t cilp_rect;
  float_t xform[6];
  nvgGetStateXfrom(vg, xform);

  /* 判断如果 vg 没有旋转和缩放的坐标系，直接使用 rect_intersect 函数做交集提高效率 */
  if (vgcanvas_nanovg_is_system_orientation_0_or_180(xform) ||
      vgcanvas_nanovg_is_system_orientation_90_or_270(xform)) {
    bool_t is_int = tk_abs(vgcanvas->ratio - (uint32_t)vgcanvas->ratio) < 0.001f;
    r1 = rect_init(x, y, w, h);
#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT) && defined(DIRTY_RECTS_CLIP_SUPPORT)
	extern void nvgScissors(NVGcontext* ctx, int* dirty_rect, int dirty_size, int *max);
	extern int nvgGetCurrScissors (NVGcontext* ctx, float *scissor_rects, int cb_scissor_rects);
	dirty_rects_t cilp_rects;
	rect_t rects[TK_MAX_DIRTY_RECT_NR];
	int nr;

	int i;
	nr = 0;
	rect_intersect_dirty_rects ((const dirty_rects_t*)&vgcanvas->dirty_rects, (const rect_t*)&r1, &cilp_rects);
	for (i = 0; i < cilp_rects.nr; i ++)
	{
		cilp_rect = cilp_rects.rects[i];
		if (is_int) {
		  x = cilp_rect.x;
		  y = cilp_rect.y;
		  w = cilp_rect.w;
		  h = cilp_rect.h;
		} else {
		  x = ((uint32_t)(cilp_rect.x * vgcanvas->ratio)) / vgcanvas->ratio;
		  y = ((uint32_t)(cilp_rect.y * vgcanvas->ratio)) / vgcanvas->ratio;
		  w = ((uint32_t)(cilp_rect.w * vgcanvas->ratio)) / vgcanvas->ratio;
		  h = ((uint32_t)(cilp_rect.h * vgcanvas->ratio)) / vgcanvas->ratio;
		}
		rects[nr].x = x;
		rects[nr].y = y;
		rects[nr].w = w;
		rects[nr].h = h;
		nr ++;
	}
	nvgScissors(vg, (int *)rects, nr, (int *)&cilp_rects.max);
	vgcanvas->cb_clip_rects = nvgGetCurrScissors(vg, (float *)&vgcanvas->clip_rects[0].x, 32);
#else
    cilp_rect = rect_intersect((const rect_t*)&vgcanvas->dirty_rect, (const rect_t*)&r1);
    if (is_int) {
      x = cilp_rect.x;
      y = cilp_rect.y;
      w = cilp_rect.w;
      h = cilp_rect.h;
    } else {
      x = ((uint32_t)(cilp_rect.x * vgcanvas->ratio)) / vgcanvas->ratio;
      y = ((uint32_t)(cilp_rect.y * vgcanvas->ratio)) / vgcanvas->ratio;
      w = ((uint32_t)(cilp_rect.w * vgcanvas->ratio)) / vgcanvas->ratio;
      h = ((uint32_t)(cilp_rect.h * vgcanvas->ratio)) / vgcanvas->ratio;
    }
    nvgScissor(vg, x, y, w, h);
#endif

  } else {
    nvgIntersectScissorForOtherRect(
        vg, x, y, w, h, (float_t)vgcanvas->dirty_rect.x, (float_t)vgcanvas->dirty_rect.y,
        (float_t)vgcanvas->dirty_rect.w, (float_t)vgcanvas->dirty_rect.h);
  }
#else
  nvgScissor(vg, x, y, w, h);
#endif
  nvgGetCurrScissor(vg, &vgcanvas->clip_rect.x, &vgcanvas->clip_rect.y, &vgcanvas->clip_rect.w, &vgcanvas->clip_rect.h);
  return RET_OK;
}

static bool_t vgcanvas_nanovg_is_rectf_in_clip_rect(vgcanvas_t* vgcanvas, float_t left, float_t top, float_t right, float_t bottom) {
#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT) && defined(DIRTY_RECTS_CLIP_SUPPORT)
  int i;
  for (i = 0; i < vgcanvas->cb_clip_rects; i ++ )
  {
	float clip_rect_x, clip_rect_y, clip_rect_w, clip_rect_h;
	clip_rect_x = vgcanvas->clip_rects[i].x;
	clip_rect_y = vgcanvas->clip_rects[i].y;
	clip_rect_w = vgcanvas->clip_rects[i].w;
	clip_rect_h = vgcanvas->clip_rects[i].h;
	float_t clip_left = clip_rect_x;
	float_t clip_right = clip_rect_x + clip_rect_w - 1;
	float_t clip_top = clip_rect_y;
	float_t clip_bottom = clip_rect_y + clip_rect_h - 1;
	if (left > clip_right || right < clip_left || top > clip_bottom || bottom < clip_top) {
		// 不相交
	}
	else
	{
		return TRUE;
	}
  }
  return FALSE;
#else
  float_t clip_left = vgcanvas->clip_rect.x;
  float_t clip_right = vgcanvas->clip_rect.x + vgcanvas->clip_rect.w - 1;
  float_t clip_top = vgcanvas->clip_rect.y;
  float_t clip_bottom = vgcanvas->clip_rect.y + vgcanvas->clip_rect.h - 1;
  if (left > clip_right || right < clip_left || top > clip_bottom || bottom < clip_top) {
    return FALSE;
  }
  return TRUE;
#endif
}


const rectf_t* vgcanvas_nanovg_get_clip_rect(vgcanvas_t* vgcanvas) {
  return &(vgcanvas->clip_rect);
}


#ifdef DIRTY_RECTS_CLIP_SUPPORT
static ret_t vgcanvas_nanovg_clip_dirty_rects (vgcanvas_t* vgcanvas, const dirty_rects_t* dirty_rects) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

#if defined(VGCANVAS_DIRTY_RECTS_SUPPORT)
  memcpy (&vgcanvas->dirty_rects, dirty_rects, sizeof(dirty_rects_t));
  vgcanvas->dirty_rect.x = dirty_rects->max.x;
  vgcanvas->dirty_rect.y = dirty_rects->max.y;
  vgcanvas->dirty_rect.w = dirty_rects->max.w;
  vgcanvas->dirty_rect.h = dirty_rects->max.h;
#endif

  return RET_OK;
}
#endif

static ret_t vgcanvas_nanovg_fill(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgFill(vg);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_stroke(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgStroke(vg);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_paint(vgcanvas_t* vgcanvas, bool_t stroke, bitmap_t* img) {
  int iw = img->w;
  int ih = img->h;
  NVGpaint imgPaint;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  int id = vgcanvas_nanovg_ensure_image(canvas, img);
  return_value_if_fail(id >= 0, RET_BAD_PARAMS);

  imgPaint = nvgImagePattern(vg, 0, 0, iw, ih, 0, id, 1);

  if (stroke) {
    nvgStrokePaint(vg, imgPaint);
    nvgStroke(vg);
  } else {
    nvgFillPaint(vg, imgPaint);
    nvgClosePath(vg);
    nvgFill(vg);
  }

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_font_size(vgcanvas_t* vgcanvas, float_t size) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  nvgFontSize(vg, size);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_font(vgcanvas_t* vgcanvas, const char* name) {
  int font_id = 0;
  ret_t reuslt = RET_FAIL;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  return_value_if_fail(name && *name, RET_BAD_PARAMS);

  font_id = tk_pointer_to_int(vgcanvas_asset_manager_get_font_ctx(vgcanvas_asset_manager(), vgcanvas, name, &reuslt));

  if (reuslt != RET_OK) {
    assets_manager_t* am = vgcanvas->assets_manager ? vgcanvas->assets_manager : assets_manager();
    const asset_info_t* r = assets_manager_ref(am, ASSET_TYPE_FONT, name);

    if (r == NULL || r->subtype != ASSET_TYPE_FONT_TTF) {
      font_id = nvgFindFont(vg, name);
      if (font_id >= 0) {
        canvas->font_id = font_id;
        return RET_OK;
      }

      if (r == NULL) {
        font_t* font = font_manager_get_font(font_manager(), NULL, TK_DEFAULT_FONT_SIZE);
        if (font != NULL) {
          r = assets_manager_ref(assets_manager(), ASSET_TYPE_FONT, font->name);
        }
      }
    }

    if (r != NULL && r->subtype == ASSET_TYPE_FONT_TTF) {
	#ifdef USE_TTF_FONT_DATA_CACHE
	  if( r->size > 8 && *(unsigned int *)(r->data) == 0xAA5555AA &&  *(unsigned int *)(r->data + 4) == 0x454C4946)	// 基于文件cache
		font_id = nvgCreateFont(vg, name, (unsigned char*)(r->data + 8));
	  else
		font_id = nvgCreateFontMem(vg, name, (unsigned char*)r->data, r->size, 0);	    
	#else
      font_id = nvgCreateFontMem(vg, name, (unsigned char*)r->data, r->size, 0);
	#endif
      if (font_id < 0) {
        log_warn("load font %s failed\n", name);
      }
    }

    if (r != NULL) {
      assets_manager_unref(am, r);
    }
    vgcanvas_asset_manager_add_font(vgcanvas_asset_manager(), vgcanvas, name, tk_pointer_from_int(font_id));
  }

  canvas->font_id = font_id;
  nvgFontFaceId(vg, font_id);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_text_align(vgcanvas_t* vgcanvas, const char* text_align) {
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;

  if (text_align[0] == 'r') {
    canvas->text_align_h = NVG_ALIGN_RIGHT;
  } else if (text_align[0] == 'c') {
    canvas->text_align_h = NVG_ALIGN_CENTER;
  } else {
    canvas->text_align_h = NVG_ALIGN_LEFT;
  }

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_text_baseline(vgcanvas_t* vgcanvas, const char* text_baseline) {
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;

  if (text_baseline[0] == 'b') {
    canvas->text_align_v = NVG_ALIGN_BOTTOM;
  } else if (text_baseline[0] == 'm') {
    canvas->text_align_v = NVG_ALIGN_MIDDLE;
  } else {
    canvas->text_align_v = NVG_ALIGN_TOP;
  }

  return RET_OK;
}

static ret_t vgcanvas_nanovg_fill_text(vgcanvas_t* vgcanvas, const char* text, float_t x, float_t y,
                                       float_t max_width) {
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgTextAlign(vg, canvas->text_align_v | canvas->text_align_h);
  nvgText(vg, x, y, text, text + strlen(text));

  return RET_OK;
}

static float_t vgcanvas_nanovg_measure_text(vgcanvas_t* vgcanvas, const char* text) {
  float bounds[4];
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgTextAlign(vg, canvas->text_align_v | canvas->text_align_h);

  return nvgTextBounds(vg, 0, 0, text, text + strlen(text), bounds);
}

#if defined( WITH_NANOVG_GPU) || defined(WITH_VG_GPU)
static ret_t vgcanvas_nanovg_get_text_metrics(vgcanvas_t* vgcanvas, float_t* ascent,
                                              float_t* descent, float_t* line_hight) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  float a = 0;
  float d = 0;
  float h = 0;

  nvgTextMetrics(vg, &a, &d, &h);
  if (ascent != NULL) {
    *ascent = a;
  }

  if (descent != NULL) {
    *descent = d;
  }

  if (line_hight != NULL) {
    *line_hight = h;
  }

  return RET_OK;
}
#else
#define vgcanvas_nanovg_get_text_metrics NULL
#endif /*WITH_NANOVG_GPU*/


#ifdef WITH_VG_GPU

#ifdef VG_DRAW_IMAGE_DIRECT
extern void nvgDrawImageDirect (NVGcontext* ctx, char *image, int x, int y, int w, int h, int stride, int image_w, int image_h, int is_rgb565);

static ret_t vgcanvas_nanovg_draw_image_direct (vgcanvas_t* vgcanvas, bitmap_t* img, float_t sx, float_t sy,
                                        float_t sw, float_t sh, float_t dx, float_t dy, float_t dw,
                                        float_t dh) {
  float scaleX, scaleY;
  int32_t f = 0;
  uint8_t* img_data = NULL;
  if(dw == 0.0f || sw == 0.0f)
	return RET_OK;
  if(dw == 0.0f && sw == 0.0f || dh == 0.0f && sh == 0.0f)
  {
	scaleX = 1.0f;
	scaleY = 1.0f;
  }
  else
  {
	scaleX = (float)dw / sw;
	scaleY = (float)dh / sh;
  }
  switch (img->format) {
    case BITMAP_FMT_RGBA8888: {
		f = VG_sABGR_8888;
		break;
	}

	case BITMAP_FMT_BGRA8888: {
		f = VG_sARGB_8888;
		break;
	}
	case BITMAP_FMT_BGR565: {
		f = VG_sRGB_565;
		//f = VG_sBGR_565;
		break;
	}
	case BITMAP_FMT_RGB565: {
		//f = VG_sBGR_565;
		f = VG_sRGB_565;
		break;
	}
  }

  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  (void)(canvas);
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  img_data =  bitmap_lock_buffer_for_read(img);
  nvgSave(vg);
  nvgTranslate(vg, dx, dy );
  nvgScale(vg, scaleX, scaleY);
  nvgDrawImageDirect (vg, (char *)img_data, sx, sy, sw, sh, img->line_length, img->w, img->h, f);
  nvgRestore(vg);
	
  bitmap_unlock_buffer(img);
  return RET_OK;
}

#endif

extern void vgcanvas_nanovg_free_image(vgcanvas_nanovg_t* canvas, bitmap_t* img) ;

static ret_t vgcanvas_nanovg_draw_image(vgcanvas_t* vgcanvas, bitmap_t* img, float_t sx, float_t sy,
                                        float_t sw, float_t sh, float_t dx, float_t dy, float_t dw,
                                        float_t dh) {
  int iw = img->w;
  int ih = img->h;
  NVGpaint imgPaint;
  uint8_t* img_data = NULL;
  float scaleX, scaleY;
  (void)(img_data);
  if(dw == 0.0f || sw == 0.0f)
	 return RET_OK;
  if(dw == 0.0f && sw == 0.0f || dh == 0.0f && sh == 0.0f)
  {
	scaleX = 1.0f;
	scaleY = 1.0f;
  }
  else
  {
	scaleX = (float)dw / sw;
	scaleY = (float)dh / sh;
  }

  #ifdef VG_DRAW_IMAGE_DIRECT
  // 20220101 强制在所有情况下使用direct drwa image模式, 有发现图像显示异常再进行修复.
  //   使用direct draw image模式可以明显改善内存的使用
  //if(sx == 0 && sy == 0)
    return vgcanvas_nanovg_draw_image_direct (vgcanvas, img, sx, sy, sw, sh, dx, dy, dw, dh);
  #endif

  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  // id为texture对象的句柄
  int id = vgcanvas_nanovg_ensure_image(canvas, img);
  return_value_if_fail(id >= 0, RET_BAD_PARAMS);

  imgPaint = nvgImagePattern(vg, 0, 0, iw, ih, 0, id, 1);
  nvgSave(vg);
  nvgBeginPath(vg);
  nvgTranslate(vg, dx - (sx * scaleX), dy - (sy * scaleY));
  nvgScale(vg, scaleX, scaleY);
  nvgRect(vg, sx, sy, sw, sh);
  nvgFillPaint(vg, imgPaint);
  nvgClosePath(vg);
  nvgFill(vg);
  nvgBeginPath(vg);
  nvgRestore(vg);
  
  // 20211031 img将需要double内存.尽量在使用后将texture对象释放, 避免内存分配溢出现象.
  vgcanvas_nanovg_free_image (canvas, img);
  image_manager_update_specific(img->image_manager, img);

  return RET_OK;
}

#else

// NANOVG
static ret_t vgcanvas_nanovg_draw_image(vgcanvas_t* vgcanvas, bitmap_t* img, float_t sx, float_t sy,
                                        float_t sw, float_t sh, float_t dx, float_t dy, float_t dw,
                                        float_t dh) {
  int iw = img->w;
  int ih = img->h;
  NVGpaint imgPaint;
  float scaleX, scaleY;
  if(dw == 0.0f || sw == 0.0f)
	return RET_OK;
  if(dw == 0.0f && sw == 0.0f || dh == 0.0f && sh == 0.0f)
  {
	scaleX = 1.0f;
	scaleY = 1.0f;
  }
  else
  {
	scaleX = (float)dw / sw;
	scaleY = (float)dh / sh;
  }
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  int id = vgcanvas_nanovg_ensure_image(canvas, img);
  return_value_if_fail(id >= 0, RET_BAD_PARAMS);

  imgPaint = nvgImagePattern(vg, 0, 0, iw, ih, 0, id, 1);
  nvgSave(vg);
  nvgBeginPath(vg);
  nvgTranslate(vg, dx - (sx * scaleX), dy - (sy * scaleY));
  nvgScale(vg, scaleX, scaleY);
  nvgRect(vg, sx, sy, sw, sh);
  nvgFillPaint(vg, imgPaint);
  nvgClosePath(vg);
  nvgFill(vg);
  nvgBeginPath(vg);
  nvgRestore(vg);

  return RET_OK;
}


#endif	// #ifdef WITH_VG_GPU

#ifdef WITH_VG_GPU
static ret_t vgcanvas_nanovg_draw_image_repeat(vgcanvas_t* vgcanvas, bitmap_t* img, float_t sx, float_t sy,
                                        float_t sw, float_t sh, float_t dx, float_t dy, float_t dw, float_t dh,
													 float_t dst_w, float_t dst_h) {
  NVGpaint imgPaint;
  float scaleX, scaleY;  // 源图像到目标区域的缩放系数
  if(dw == 0.0f || sw == 0.0f)
	return RET_OK;
  if(dw == 0.0f && sw == 0.0f || dh == 0.0f && sh == 0.0f)
  {
	scaleX = 1.0f;
	scaleY = 1.0f;
  }
  else
  {
	scaleX = (float)dst_w / sw;
	scaleY = (float)dst_h / sh;
  }

  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  int id = vgcanvas_nanovg_ensure_sub_image(canvas, img, (int)sx, (int)sy, (int)sw, (int)sh);
  return_value_if_fail(id >= 0, RET_BAD_PARAMS);

  imgPaint = nvgImagePatternRepeat(vg, 0, 0, sw, sh, 0, id, 1);
  nvgSave(vg);
  nvgBeginPath(vg);
  nvgTranslate(vg, dx, dy);
  nvgScale(vg, scaleX, scaleY);
  nvgRect(vg, 0, 0, dw/scaleX, dh/scaleY);
  nvgFillPaint(vg, imgPaint);
  nvgClosePath(vg);
  nvgFill(vg);
  nvgBeginPath(vg);
  nvgRestore(vg);
  vgcanvas_nanovg_free_sub_image (canvas, id);
  
  return RET_OK;
}
#endif


static ret_t vgcanvas_nanovg_set_antialias(vgcanvas_t* vgcanvas, bool_t value) {
#if defined( WITH_NANOVG_GPU) || defined(WITH_VG_GPU)
  vgcanvas_nanovg_t* canvas = (vgcanvas_nanovg_t*)vgcanvas;
  NVGcontext* vg = canvas->vg;
  nvgShapeAntiAlias(vg, value);
#else
  (void)vgcanvas;
  (void)value;
  /*always*/
#endif

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_line_width(vgcanvas_t* vgcanvas, float_t value) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgStrokeWidth(vg, value);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_global_alpha(vgcanvas_t* vgcanvas, float_t value) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  vgcanvas->global_alpha = value;
  nvgGlobalAlpha(vg, value);

  return RET_OK;
}

static NVGcolor toNVGcolor(color_t c) {
  return nvgRGBA(c.rgba.r, c.rgba.g, c.rgba.b, c.rgba.a);
}

static ret_t vgcanvas_nanovg_set_fill_color(vgcanvas_t* vgcanvas, color_t c) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgFillColor(vg, toNVGcolor(c));

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_fill_linear_gradient(vgcanvas_t* vgcanvas, float_t sx, float_t sy,
                                                      float_t ex, float_t ey, color_t icolor,
                                                      color_t ocolor) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  NVGpaint paint = nvgLinearGradient(vg, sx, sy, ex, ey, toNVGcolor(icolor), toNVGcolor(ocolor));
  nvgFillPaint(vg, paint);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_fill_radial_gradient(vgcanvas_t* vgcanvas, float_t cx, float_t cy,
                                                      float_t inr, float_t outr, color_t icolor,
                                                      color_t ocolor) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  NVGpaint paint = nvgRadialGradient(vg, cx, cy, inr, outr, toNVGcolor(icolor), toNVGcolor(ocolor));
  nvgFillPaint(vg, paint);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_stroke_color(vgcanvas_t* vgcanvas, color_t c) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgStrokeColor(vg, nvgRGBA(c.rgba.r, c.rgba.g, c.rgba.b, c.rgba.a));

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_stroke_linear_gradient(vgcanvas_t* vgcanvas, float_t sx,
                                                        float_t sy, float_t ex, float_t ey,
                                                        color_t icolor, color_t ocolor) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  NVGpaint paint = nvgLinearGradient(vg, sx, sy, ex, ey, toNVGcolor(icolor), toNVGcolor(ocolor));
  nvgStrokePaint(vg, paint);

  return RET_OK;
}

// 20211230 增加VG风格的线性渐变色
static ret_t vgcanvas_nanovg_set_stroke_vg_linear_gradient(vgcanvas_t* vgcanvas, float_t sx,
                                                        float_t sy, float_t ex, float_t ey,
                                                        float_t * ramp_stops, int32_t ramp_stops_count) {
#if defined(WITH_VG_GPU)
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  NVGpaint paint = nvgLinearGradientEx(vg, sx, sy, ex, ey, ramp_stops, ramp_stops_count);
  nvgStrokePaint(vg, paint);
#endif
  return RET_OK;

}

static ret_t vgcanvas_nanovg_set_stroke_radial_gradient(vgcanvas_t* vgcanvas, float_t cx,
                                                        float_t cy, float_t inr, float_t outr,
                                                        color_t icolor, color_t ocolor) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;
  NVGpaint paint = nvgRadialGradient(vg, cx, cy, inr, outr, toNVGcolor(icolor), toNVGcolor(ocolor));
  nvgStrokePaint(vg, paint);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_line_cap(vgcanvas_t* vgcanvas, const char* value) {
  int line_cap = NVG_BUTT;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  if (*value == 'r') {
    line_cap = NVG_ROUND;
  } else if (*value == 's') {
    line_cap = NVG_SQUARE;
  } else if (*value == 'b') {
    line_cap = NVG_BUTT;
  }

  nvgLineCap(vg, line_cap);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_line_join(vgcanvas_t* vgcanvas, const char* value) {
  int line_join = NVG_MITER;
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  if (*value == 'r') {
    line_join = NVG_ROUND;
  } else if (*value == 'b') {
    line_join = NVG_BEVEL;
  }

  nvgLineJoin(vg, line_join);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_set_miter_limit(vgcanvas_t* vgcanvas, float_t value) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgMiterLimit(vg, value);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_save(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgSave(vg);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_restore(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgRestore(vg);

  return RET_OK;
}

static ret_t vgcanvas_nanovg_clear_cache(vgcanvas_t* vgcanvas) {
  NVGcontext* vg = ((vgcanvas_nanovg_t*)vgcanvas)->vg;

  nvgClearCache(vg);
  nvgDeleteFontByName(vg, NULL);
  vgcanvas_asset_manager_remove_font(vgcanvas_asset_manager(), vgcanvas, NULL);
  vgcanvas_asset_manager_remove_image(vgcanvas_asset_manager(), vgcanvas, NULL);
  
  return RET_OK;
}

static const vgcanvas_vtable_t vt = {
    .reinit = vgcanvas_nanovg_reinit,
    .begin_frame = vgcanvas_nanovg_begin_frame,
    .set_assets_manager = vgcanvas_nanovg_set_assets_manager,
    .reset = vgcanvas_nanovg_reset,
    .reset_curr_state = vgcanvas_nanovg_reset_curr_state,
    .flush = vgcanvas_nanovg_flush,
    .clear_rect = vgcanvas_nanovg_clear_rect,
    .begin_path = vgcanvas_nanovg_begin_path,
    .move_to = vgcanvas_nanovg_move_to,
    .line_to = vgcanvas_nanovg_line_to,
    .arc = vgcanvas_nanovg_arc,
    .arc_to = vgcanvas_nanovg_arc_to,
    .bezier_to = vgcanvas_nanovg_bezier_to,
    .quad_to = vgcanvas_nanovg_quad_to,
    .is_point_in_path = vgcanvas_nanovg_is_point_in_path,
    .ellipse = vgcanvas_nanovg_ellipse,
    .rounded_rect = vgcanvas_nanovg_rounded_rect,
    .close_path = vgcanvas_nanovg_close_path,
    .scale = vgcanvas_nanovg_scale,
    .rotate = vgcanvas_nanovg_rotate,
    .translate = vgcanvas_nanovg_translate,
    .transform = vgcanvas_nanovg_transform,
    .set_transform = vgcanvas_nanovg_set_transform,
    .clip_rect = vgcanvas_nanovg_clip_rect,
    .get_clip_rect = vgcanvas_nanovg_get_clip_rect,
    .is_rectf_in_clip_rect = vgcanvas_nanovg_is_rectf_in_clip_rect,
	
#ifdef DIRTY_RECTS_CLIP_SUPPORT
	.clip_dirty_rects = vgcanvas_nanovg_clip_dirty_rects,
#endif
    .intersect_clip_rect = vgcanvas_nanovg_intersect_clip_rect,
    .path_winding = vgcanvas_nanovg_path_winding,
    .fill = vgcanvas_nanovg_fill,
    .stroke = vgcanvas_nanovg_stroke,
    .paint = vgcanvas_nanovg_paint,
    .set_font = vgcanvas_nanovg_set_font,
    .set_font_size = vgcanvas_nanovg_set_font_size,
    .set_text_align = vgcanvas_nanovg_set_text_align,
    .set_text_baseline = vgcanvas_nanovg_set_text_baseline,
    .fill_text = vgcanvas_nanovg_fill_text,
    .measure_text = vgcanvas_nanovg_measure_text,
    .draw_image = vgcanvas_nanovg_draw_image,
#ifdef WITH_VG_GPU
    .draw_image_repeat = vgcanvas_nanovg_draw_image_repeat,
#endif
    .set_antialias = vgcanvas_nanovg_set_antialias,
    .set_global_alpha = vgcanvas_nanovg_set_global_alpha,
    .set_line_width = vgcanvas_nanovg_set_line_width,
    .set_fill_color = vgcanvas_nanovg_set_fill_color,
    .set_fill_linear_gradient = vgcanvas_nanovg_set_fill_linear_gradient,
    .set_fill_radial_gradient = vgcanvas_nanovg_set_fill_radial_gradient,
    .set_stroke_color = vgcanvas_nanovg_set_stroke_color,
    .set_stroke_linear_gradient = vgcanvas_nanovg_set_stroke_linear_gradient,
    // 20211230 增加VG风格的线性渐变色
    .set_stroke_vg_linear_gradient = vgcanvas_nanovg_set_stroke_vg_linear_gradient,
    .set_stroke_radial_gradient = vgcanvas_nanovg_set_stroke_radial_gradient,
    .set_line_join = vgcanvas_nanovg_set_line_join,
    .set_line_cap = vgcanvas_nanovg_set_line_cap,
    .set_miter_limit = vgcanvas_nanovg_set_miter_limit,
    .get_text_metrics = vgcanvas_nanovg_get_text_metrics,
    .save = vgcanvas_nanovg_save,
    .restore = vgcanvas_nanovg_restore,
    .end_frame = vgcanvas_nanovg_end_frame,
    .create_fbo = vgcanvas_nanovg_create_fbo,
    .bind_fbo = vgcanvas_nanovg_bind_fbo,
    .destroy_fbo = vgcanvas_nanovg_destroy_fbo,
    .unbind_fbo = vgcanvas_nanovg_unbind_fbo,
    .fbo_to_bitmap = vgcanvas_nanovg_fbo_to_bitmap,
    .clear_cache = vgcanvas_nanovg_clear_cache,
    .destroy = vgcanvas_nanovg_destroy};
