#include "base/g2d.h"
#include "blend2d.h"
#include "cp15/cp15.h"

#ifdef WITH_ARK_G2D

#define ARK_G2D_FILL_SIZE_LIMIT 1024
#define ARK_G2D_BLIT_SIZE_LIMIT 1024

static ret_t ark_blend2d_run(void)
{
	if (!blend2d_run())
		return RET_OK;
	else
		return RET_FAIL;
}

static void ark_g2d_invalidate_cache(uint32_t address, uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t pxSize)
{
    int y;

    for(y = 0; y < height; y++) {
        CP15_flush_dcache_for_dma(address, address + width * pxSize);
        address += stride;
    }
}

void ark_g2d_clean_cache(uint32_t address, uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t pxSize)
{
    int y;

    for(y = 0; y < height; y++) {
        CP15_clean_dcache_for_dma(address, address + width * pxSize);
        address += stride;
    }
}						


ret_t g2d_fill_rect(bitmap_t* fb, const rect_t* dst, color_t c) 
{
  uint8_t* fb_data = NULL;
  uint32_t start_address;
  int format;
  int pixsize;
  ret_t ret;

  if (c.rgba.a < 0xf0) {
    return RET_NOT_IMPL;
  }

  return_value_if_fail(fb != NULL && fb->buffer != NULL && dst != NULL, RET_BAD_PARAMS);
  return_value_if_fail(fb->format == BITMAP_FMT_BGR565 || fb->format == BITMAP_FMT_BGRA8888,
                       RET_BAD_PARAMS);

  if(dst->w <= 0 || dst->w * dst->h < ARK_G2D_FILL_SIZE_LIMIT)
  	return RET_BAD_PARAMS;

  if (fb->format == BITMAP_FMT_BGR565) {
    if(dst->x & 1 || dst->w & 1)
  	  return RET_BAD_PARAMS;
    pixsize = 2;
    format = BLEND2D_FORAMT_RGB565;
  } else {
    pixsize = 4;
    format = BLEND2D_FORAMT_ARGB888;
  }					   

  fb_data = bitmap_lock_buffer_for_write(fb);

  start_address = (uint32_t)fb_data + pixsize * (fb->w * dst->y + dst->x);

  ark_g2d_invalidate_cache(start_address, dst->w, dst->h, fb->w * pixsize, pixsize);

  blend2d_fill((uint32_t)fb_data, dst->x, dst->y, dst->w, dst->h, fb->w, fb->h,
		   c.rgba.r, c.rgba.g, c.rgba.b, format, c.rgba.a, 1);

  ret = ark_blend2d_run();

  bitmap_unlock_buffer(fb);

  return ret;
}

ret_t g2d_copy_image(bitmap_t* fb, bitmap_t* img, const rect_t* src, xy_t x, xy_t y) {
  uint8_t* fb_data = NULL, *img_data = NULL;
  uint32_t start_address;
  int format, img_format;
  int pixsize, img_pixsize;
  ret_t ret;

  return_value_if_fail(fb != NULL && fb->buffer != NULL, RET_BAD_PARAMS);
  return_value_if_fail(img != NULL && img->buffer != NULL && src != NULL, RET_BAD_PARAMS);
  return_value_if_fail(fb->format == BITMAP_FMT_BGR565 || fb->format == BITMAP_FMT_BGRA8888,
                       RET_BAD_PARAMS);
  return_value_if_fail(img->format == BITMAP_FMT_BGR565 || img->format == BITMAP_FMT_BGRA8888,
                       RET_BAD_PARAMS);

  if (src->w <= 0 || src->w * src->h < ARK_G2D_BLIT_SIZE_LIMIT)
  	return RET_BAD_PARAMS;	   

  if (fb->format == BITMAP_FMT_BGR565) {
  	if (x & 1 || src->w & 1)
	  return RET_BAD_PARAMS;	
    pixsize = 2;
    format = BLEND2D_FORAMT_RGB565;
  } else {
    pixsize = 4;
    format = BLEND2D_FORAMT_ARGB888;
  }

  if (img->format == BITMAP_FMT_BGR565) {
  	if (src->x & 1 || src->w & 1)
	  return RET_BAD_PARAMS;
    img_pixsize = 2;
    img_format = BLEND2D_FORAMT_RGB565;
  } else {
    img_pixsize = 4;
    img_format = BLEND2D_FORAMT_ARGB888;
  }

  fb_data = bitmap_lock_buffer_for_write(fb);
  img_data = bitmap_lock_buffer_for_write(img);  

  start_address = (uint32_t)fb_data + pixsize * (fb->w * y + x);
  ark_g2d_invalidate_cache(start_address, src->w, src->h, fb->w * pixsize, pixsize);

  start_address = (uint32_t)img_data + img_pixsize * (img->w * src->y + src->x);
  ark_g2d_clean_cache(start_address, src->w, src->h, img->w * img_pixsize, img_pixsize);

  blend2d_blit((uint32_t)fb_data, fb->w, fb->h, x, y, format, src->w, src->h,
		   (uint32_t)img_data, img->w, img->h, src->x, src->y, img_format, 0xff, 0);

  ret = ark_blend2d_run();

  bitmap_unlock_buffer(fb);
  bitmap_unlock_buffer(img);

  return ret;
}

ret_t g2d_rotate_image(bitmap_t* fb, bitmap_t* img, const rect_t* src, lcd_orientation_t o) {
  (void)fb;
  (void)img;
  (void)src;
  (void)o;

  return RET_NOT_IMPL;
}

ret_t g2d_blend_image(bitmap_t* fb, bitmap_t* img, const rect_t* dst, const rect_t* src,
                      uint8_t global_alpha) {
  uint8_t* fb_data = NULL, *img_data = NULL;
  uint32_t start_address;
  int format, img_format;
  int pixsize, img_pixsize;
  ret_t ret;  

  return_value_if_fail(global_alpha > 0xf0, RET_NOT_IMPL); /*not support global_alpha*/
  return_value_if_fail(fb != NULL && fb->buffer != NULL, RET_BAD_PARAMS);
  return_value_if_fail(img != NULL && img->buffer != NULL && src != NULL && dst != NULL,
                       RET_BAD_PARAMS);
  return_value_if_fail(fb->format == BITMAP_FMT_BGR565 || fb->format == BITMAP_FMT_BGRA8888,
                       RET_BAD_PARAMS);
  return_value_if_fail(img->format == BITMAP_FMT_BGR565 || img->format == BITMAP_FMT_BGRA8888 ||
                       img->format == BITMAP_FMT_RGBA8888, RET_BAD_PARAMS);
  return_value_if_fail(src->w == dst->w && src->h == dst->h, RET_NOT_IMPL); /*not support scale*/

  if (src->w <= 0 || src->w * src->h < ARK_G2D_BLIT_SIZE_LIMIT)
  	return RET_BAD_PARAMS;

  if (fb->format == BITMAP_FMT_BGR565) {
  	if (dst->x & 1 || dst->w & 1)
	  return RET_BAD_PARAMS;
    pixsize = 2;
    format = BLEND2D_FORAMT_RGB565;
  } else {
    pixsize = 4;
    format = BLEND2D_FORAMT_ARGB888;
  }

  if (img->format == BITMAP_FMT_BGR565) {
  	if (src->x & 1 || src->w & 1)
	  return RET_BAD_PARAMS;
    img_pixsize = 2;
    img_format = BLEND2D_FORAMT_RGB565;
  } else if (img->format == BITMAP_FMT_RGBA8888) {
    img_pixsize = 4;
    img_format = BLEND2D_FORMAT_ABGR888;
  } else {
    img_pixsize = 4;
    img_format = BLEND2D_FORAMT_ARGB888;
  }

  fb_data = bitmap_lock_buffer_for_write(fb);
  img_data = bitmap_lock_buffer_for_write(img);  

  start_address = (uint32_t)fb_data + pixsize * (fb->w * dst->y + dst->x);
  ark_g2d_invalidate_cache(start_address, dst->w, dst->h, fb->w * pixsize, pixsize);

  start_address = (uint32_t)img_data + img_pixsize * (img->w * src->y + src->x);
  ark_g2d_clean_cache(start_address, src->w, src->h, img->w * img_pixsize, img_pixsize);

  blend2d_blit((uint32_t)fb_data, fb->w, fb->h, dst->x, dst->y, format, src->w, src->h,
		   (uint32_t)img_data, img->w, img->h, src->x, src->y, img_format, 0xff, 1);

  ret = ark_blend2d_run();

  bitmap_unlock_buffer(fb);
  bitmap_unlock_buffer(img);

  return ret;
}
#endif /*WITH_ARK_G2D*/
