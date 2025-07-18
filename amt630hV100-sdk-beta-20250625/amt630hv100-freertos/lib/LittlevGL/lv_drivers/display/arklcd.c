/**
 * @file arklcd.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "FreeRTOS.h"
#include "chip.h"
#include "arklcd.h"


#if USE_ARKLCD
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      STRUCTURES
 **********************/

struct bsd_fb_var_info{
    uint32_t xoffset;
    uint32_t yoffset;
    uint32_t xres;
    uint32_t yres;
    int bits_per_pixel;
 };

struct bsd_fb_fix_info{
    long int line_length;
    long int smem_len;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static struct bsd_fb_var_info vinfo;
static struct bsd_fb_fix_info finfo;
static char *fbp = 0;
static long int screensize = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void arklcd_init(void)
{
	unsigned int width, height;
	LCD_OSD_FORMAT format;

	ark_lcd_get_osd_size(LCD_UI_LAYER, &width, &height);
	ark_lcd_get_osd_format(LCD_UI_LAYER, &format);

	vinfo.xres = width;
	vinfo.yres = height;
	switch (format) {
		case LCD_OSD_FORAMT_RGB565:
		case LCD_OSD_FORAMT_Y_UV422:
		case LCD_OSD_FORAMT_Y_U_V422:
			vinfo.bits_per_pixel = 16;
			break;
		case LCD_OSD_FORAMT_Y_UV420:
		case LCD_OSD_FORAMT_Y_U_V420:
			vinfo.bits_per_pixel = 12;
			break;
		case LCD_OSD_FORAMT_ARGB888:
		default:
			vinfo.bits_per_pixel = 32;
			break;
	}
	vinfo.xoffset = 0;
	vinfo.yoffset = 0;

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	// Figure out the size of the screen in bytes
	finfo.line_length = vinfo.xres * vinfo.bits_per_pixel / 8;
	finfo.smem_len = screensize =  finfo.line_length * vinfo.yres;

    fbp = (void*)ark_lcd_get_virt_addr();

#if ARKLCD_DOUBLE_BUFFERED
	static lv_disp_buf_t disp_buf;
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
	lv_disp_buf_init(&disp_buf, (lv_color_t*)fbp + vinfo.xres * vinfo.yres * 2,
		(lv_color_t*)fbp + vinfo.xres * vinfo.yres * 2,
		LV_HOR_RES_MAX * LV_VER_RES_MAX);
#else
	lv_disp_buf_init(&disp_buf, (lv_color_t*)fbp + vinfo.xres * vinfo.yres, fbp,
		LV_HOR_RES_MAX * LV_VER_RES_MAX);
#endif
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.buffer = &disp_buf;
	disp_drv.flush_cb = arklcd_flush;
	lv_disp_drv_register(&disp_drv);
#endif
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixel to copy to the `area` part of the screen
 */
void arklcd_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
	static int rotbuf_index = 1;
	uint32_t srcfmt;
	uint32_t dstfmt;
	uint32_t dstaddr;
	uint32_t width = LCD_WIDTH;
	uint32_t height = LCD_HEIGHT;
	int ret;
#endif

	if(fbp == NULL ||
			area->x2 < 0 ||
			area->y2 < 0 ||
			area->x1 > (int32_t)vinfo.xres - 1 ||
			area->y1 > (int32_t)vinfo.yres - 1) {
		lv_disp_flush_ready(drv);
		return;
	}

#if ARKLCD_DOUBLE_BUFFERED
	CP15_clean_dcache_for_dma((uint32_t)color_p, (uint32_t)color_p + screensize);
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
#if LCD_BPP == 32
	srcfmt = PXP_SRC_FMT_ARGB8888;
	dstfmt = PXP_OUT_FMT_ARGB8888;
#else
	srcfmt = PXP_SRC_FMT_RGB565;
	dstfmt = PXP_OUT_FMT_RGB565;
#endif

#if LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_90 || LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_270
	width = LCD_HEIGHT;
	height = LCD_WIDTH;
#endif

	dstaddr = (uint32_t)fbp + FB_SIZE * rotbuf_index;

	//uint32_t stime = get_timer(0);
#if 0
	ret = pxp_scaler_rotate((uint32_t)color_p, 0, 0, srcfmt, width, height,
			dstaddr, 0, dstfmt, LCD_WIDTH, LCD_HEIGHT, LCD_ROTATE_ANGLE);
#else
	ret = pxp_rotate((uint32_t)color_p, 0, 0, srcfmt, width, height,
			dstaddr, 0, dstfmt, LCD_ROTATE_ANGLE);
#endif
	//stime = get_timer(stime);
	//printf("rotate %d us.\n", stime);
	if(ret != 0) {
		lv_disp_flush_ready(drv);
		return;
	}

	ark_lcd_set_osd_yaddr(LCD_UI_LAYER, dstaddr);
	ark_lcd_set_osd_sync(LCD_UI_LAYER);
	rotbuf_index = !rotbuf_index;
#else
	ark_lcd_set_osd_yaddr(LCD_UI_LAYER, (unsigned int)color_p);
	ark_lcd_set_osd_sync(LCD_UI_LAYER);
#endif

	/* wait vsync */
	ark_lcd_wait_for_vsync();
#ifdef REVERSE_UI
	/* 界面切换到正常UI界面时再关闭倒车层显示防止闪屏 */
	extern int is_carback_scr_active(void);
	extern int get_animation_status(void);
	if (!is_carback_scr_active() && !get_animation_status()) {
		ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
		ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
	}
#endif
#else
	/*Truncate the area to the screen*/
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > (int32_t)vinfo.xres - 1 ? (int32_t)vinfo.xres - 1 : area->x2;
	int32_t act_y2 = area->y2 > (int32_t)vinfo.yres - 1 ? (int32_t)vinfo.yres - 1 : area->y2;

	lv_coord_t w = (act_x2 - act_x1 + 1);
	long int location = 0;
	long int byte_location = 0;
	unsigned char bit_location = 0;

	/*32 or 24 bit per pixel*/
	if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
		uint32_t * fbp32 = (uint32_t *)fbp;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 4;
			memcpy(&fbp32[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 4);
			color_p += w;
		}
	}
	/*16 bit per pixel*/
	else if(vinfo.bits_per_pixel == 16) {
		uint16_t * fbp16 = (uint16_t *)fbp;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
			memcpy(&fbp16[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 2);
			color_p += w;
		}
	}
	/*8 bit per pixel*/
	else if(vinfo.bits_per_pixel == 8) {
		uint8_t * fbp8 = (uint8_t *)fbp;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length;
			memcpy(&fbp8[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1));
			color_p += w;
		}
	}
	/*1 bit per pixel*/
	else if(vinfo.bits_per_pixel == 1) {
		uint8_t * fbp8 = (uint8_t *)fbp;
		int32_t x;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			for(x = act_x1; x <= act_x2; x++) {
				location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * vinfo.xres;
				byte_location = location / 8; /* find the byte we need to change */
				bit_location = location % 8; /* inside the byte found, find the bit we need to change */
				fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
				fbp8[byte_location] |= ((uint8_t)(color_p->full)) << bit_location;
				color_p++;
			}

			color_p += area->x2 - act_x2;
		}
	} else {
		/*Not supported bit per pixel*/
	}
#endif

    drv->buffer->flushing = 0;
    drv->buffer->flushing_last = 0;
}

void fbdev_get_sizes(uint32_t *width, uint32_t *height)
{
	if (width)
		*width = vinfo.xres;

	if (height)
		*height = vinfo.yres;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
#endif
