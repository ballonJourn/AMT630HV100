/**
 * File:   image_loader.h
 * Author: AWTK Develop Team
 * Brief:  libpng image loader
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
 * 2018-01-21 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#define LIBPNG_IMAGE_IMPLEMENTATION

#include "tkc/mem.h"

#include "libpng/png.h"
#include "image_loader/image_loader_libpng.h"
#include "base/pixel_pack_unpack.h"



typedef struct tagPNGDATA {
  const uint8_t* buff;
  uint32_t buff_size;
} PNGDATA;

static void png_read_data(png_structp png_ptr, png_bytep data, size_t length)
{
  size_t check = 0;
  png_voidp io_ptr;
  PNGDATA *png_data = (PNGDATA *)png_ptr;

  /* fread() returns 0 on error, so it is OK to store this in a size_t
  * instead of an int, which is what fread() actually returns.
  */
  io_ptr = png_get_io_ptr(png_ptr);
  
  if (io_ptr != NULL)
  {
    png_data = (PNGDATA *)io_ptr;
    if (length > png_data->buff_size)
      length = png_data->buff_size;
    memcpy(data, png_data->buff, length);
    png_data->buff += length;
    png_data->buff_size -= length;
  }
}

void LIBPNG_FREE(void *h, void* ptr) {
  (void)(h);
  tk_free(ptr);
}

void* LIBPNG_MALLOC(void *h, size_t size) {
  (void)(h);
  return tk_alloc(size, __FUNCTION__, __LINE__);
}

ret_t libpng_load_image(int32_t subtype, const uint8_t* buff, uint32_t buff_size, bitmap_t* image,
                     bool_t require_bgra, bool_t enable_bgr565, bool_t enable_rgb565) {
  int w = 0;
  int h = 0;
  int n = 0;
  ret_t ret = RET_FAIL;
  unsigned int stride;
  png_structp read_ptr = NULL;
  bitmap_t *temp_bitmap = NULL;
  png_infop read_info_ptr = NULL;
  png_infop end_info_ptr = NULL;
  PNGDATA png_data;
  png_data.buff = buff;
  png_data.buff_size = buff_size;
  // 当前仅支持png格式
  if (subtype != ASSET_TYPE_IMAGE_PNG)
    return ret;
#ifdef WITH_LCD_MONO
  // 暂时未考虑支持单色显示
  return ret;
#endif

  // 暂时不支持BGRA
  if (require_bgra)
    return ret;
  if (!enable_bgr565 && !enable_rgb565)
    return ret;

  read_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, 
                                     (png_malloc_ptr)LIBPNG_MALLOC,
                                     (png_free_ptr)LIBPNG_FREE
                                      );
  if (read_ptr == NULL)
    return ret;

  png_set_error_fn(read_ptr, NULL, NULL, NULL);
  read_info_ptr = png_create_info_struct(read_ptr);
  end_info_ptr = png_create_info_struct(read_ptr);
  /* Allow application (pngtest) errors and warnings to pass */
  png_set_benign_errors(read_ptr, 1);
  /* Turn off CRC checking while reading */
  png_set_crc_action(read_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

  png_set_read_fn(read_ptr, (png_voidp)&png_data, png_read_data);
  png_set_read_status_fn(read_ptr, NULL);

  png_read_info(read_ptr, read_info_ptr);
  do {
    int interlace_type, compression_type, filter_type;
    png_uint_32 width, height;
    int bit_depth, color_type;
    if (png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
      &color_type, &interlace_type, &compression_type, &filter_type) != 0)
    {
      // 仅支持INTERLACE_NONE
      if (interlace_type != PNG_INTERLACE_NONE)
        break;
      // 仅支持8bit sample
      if (bit_depth != 8)
        break;
	  // 仅支持RGB 3通道或者RGBA 4通道
      if (color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA)
        break;
    }
    else
      break;

	if (color_type == PNG_COLOR_TYPE_RGB)
	{
		png_bytep row_buf = NULL;
		// RGB24 转换到 RGB16
		stride = width * 2;
		if (enable_bgr565)
			temp_bitmap = bitmap_create_ex(width, height, stride, BITMAP_FMT_BGR565);
		else if (enable_rgb565)
			temp_bitmap = bitmap_create_ex(width, height, stride, BITMAP_FMT_RGB565);

		if (!temp_bitmap)
			break;

		// 修改stride, bitmap的line_length不一定与设置的stride相同
		stride = temp_bitmap->line_length;
		int row_length = png_get_rowbytes(read_ptr, read_info_ptr);
		row_buf = TKMEM_ZALLOCN(unsigned char, row_length);
		if (!row_buf)
			break;
		int y;
		uint8_t* bdata = bitmap_lock_buffer_for_write(temp_bitmap);
		uint16_t* d = (uint16_t*)(bdata);

		for (y = 0; y < height; y++)
		{
			d = (uint16_t*)((bdata)+y * temp_bitmap->line_length);
			// 读取一行
			png_read_rows(read_ptr, (png_bytepp)&row_buf, NULL, 1);
			const uint8_t* s = row_buf;
			// 转换为565模式
			if (enable_bgr565)
			{
				for (int i = 0; i < width; i++) {
					uint8_t r = s[0];
					uint8_t g = s[1];
					uint8_t b = s[2];
					*d++ = rgb_to_bgr565(r, g, b);
					s += 3;
				}
			}
			else if (enable_rgb565) {
				for (int i = 0; i < width; i++) {
					uint8_t r = s[0];
					uint8_t g = s[1];
					uint8_t b = s[2];
					*d++ = rgb_to_rgb565(r, g, b);
					s += 3;
				}
			}
		}
		bitmap_unlock_buffer(temp_bitmap);
		TKMEM_FREE(row_buf);

	}
	else if (color_type == PNG_COLOR_TYPE_RGBA) {
		char **row_buf_p;
		int8_t* bdata;
		stride = width * 4;
		temp_bitmap = bitmap_create_ex(width, height, stride, BITMAP_FMT_RGBA8888);
		if (!temp_bitmap)
			break;

		row_buf_p = (char **)TKMEM_ZALLOCN(void *, height);
		if (!row_buf_p)
			break;
		bdata = (int8_t *)bitmap_lock_buffer_for_write(temp_bitmap);
		for (int y = 0; y < height; y++) {
			row_buf_p[y] = (char *)bdata + y * temp_bitmap->line_length;
		}
		png_read_rows(read_ptr, (png_bytepp)row_buf_p, NULL, height);
		bitmap_unlock_buffer(temp_bitmap);
		TKMEM_FREE(row_buf_p);
	}
    

    /* Read rest of file, and get additional chunks in info_ptr - REQUIRED */
    png_read_end(read_ptr, read_info_ptr);

    memcpy(image, temp_bitmap, sizeof(bitmap_t));
    TKMEM_FREE(temp_bitmap);
    temp_bitmap = NULL;

    ret = RET_OK;
  } while (0);

  png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
  if (temp_bitmap)
    bitmap_destroy(temp_bitmap);

  return ret;
}

static ret_t image_loader_libpng_load(image_loader_t* l, const asset_info_t* asset, bitmap_t* image) {
  ret_t ret = RET_OK;
  bool_t require_bgra = FALSE;
  bool_t enable_bgr565 = FALSE;
  bool_t enable_rgb565 = FALSE;
  return_value_if_fail(l != NULL && image != NULL, RET_BAD_PARAMS);

  if (asset->subtype != ASSET_TYPE_IMAGE_JPG && asset->subtype != ASSET_TYPE_IMAGE_PNG &&
      asset->subtype != ASSET_TYPE_IMAGE_GIF && asset->subtype != ASSET_TYPE_IMAGE_BMP) {
    return RET_NOT_IMPL;
  }

#ifdef WITH_BITMAP_BGR565
  enable_bgr565 = TRUE;
#endif /*WITH_BITMAP_BGR565*/

#ifdef WITH_BITMAP_RGB565
  enable_rgb565 = TRUE;
#endif /*WITH_BITMAP_RGB565*/

#ifdef WITH_BITMAP_BGRA
  require_bgra = TRUE;
#endif /*WITH_BITMAP_BGRA*/

  ret = libpng_load_image(asset->subtype, asset->data, asset->size, image, require_bgra, enable_bgr565,
                       enable_rgb565);

#ifdef WITH_BITMAP_PREMULTI_ALPHA
  if (ret == RET_OK) {
    ret = bitmap_premulti_alpha(image);
  }
#endif /*WITH_BITMAP_RGB565*/
  return ret;
}

static const image_loader_t libpng_loader = {.load = image_loader_libpng_load};

image_loader_t* image_loader_libpng() {
  return (image_loader_t*)&libpng_loader;
}
