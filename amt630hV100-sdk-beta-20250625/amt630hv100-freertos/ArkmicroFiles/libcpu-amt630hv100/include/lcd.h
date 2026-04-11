/*
 * ark_lcd.h
 *
 */

#ifndef _LCD_H
#define _LCD_H

#define LCD_OSD_FMT_TYPE_YUV			0
#define LCD_OSD_FMT_TYPE_ARGB888		1
#define LCD_OSD_FMT_TYPE_RGB565			2
#define LCD_OSD_FMT_TYPE_RGB454			3


typedef enum {
	LCD_OSD0 = 0,
	LCD_OSD1,
	LCD_OSD_NUMS,
}LCD_OSD_LAYER;

typedef enum {
	LCD_OSD_FORAMT_Y_UV420 = 0,
	LCD_OSD_FORAMT_Y_U_V420,
	LCD_OSD_FORAMT_Y_UV422,
	LCD_OSD_FORAMT_Y_U_V422,
	LCD_OSD_FORAMT_ARGB888,
	LCD_OSD_FORAMT_RGB565,
	LCD_OSD_FORAMT_RGB454,
	LCD_OSD_FORAMT_NUMS,
}LCD_OSD_FORMAT;

typedef struct {
	int x;
	int y;
	int width;
	int height;
	int format;
	int stride;
	unsigned int yaddr;
	unsigned int uaddr;
	unsigned int vaddr;
} LcdOsdInfo;

#define LCD_VIDEO_LAYER	LCD_OSD0
#define LCD_UI_LAYER	LCD_OSD1


/* osd layer set func */
/************************************************************/
int ark_lcd_set_osd_size(LCD_OSD_LAYER osd, uint32_t width, uint32_t height);
int ark_lcd_set_osd_format(LCD_OSD_LAYER osd, LCD_OSD_FORMAT format);
int ark_lcd_set_osd_yaddr(LCD_OSD_LAYER osd, uint32_t yaddr);
int ark_lcd_set_osd_uaddr(LCD_OSD_LAYER osd, uint32_t yaddr);
int ark_lcd_set_osd_vaddr(LCD_OSD_LAYER osd, uint32_t yaddr);
int ark_lcd_osd_enable(LCD_OSD_LAYER osd, uint8_t enable);
int ark_lcd_get_osd_enable(LCD_OSD_LAYER osd);

/* Interface with default argument value, may be you need them. If you not sure, ingore them */
int ark_lcd_set_osd_possition(LCD_OSD_LAYER osd, uint32_t h, uint32_t v);
int ark_lcd_set_osd_h_offset(LCD_OSD_LAYER osd, uint32_t offset);
int ark_lcd_osd_coeff_enable(LCD_OSD_LAYER osd, uint8_t enable);
int ark_lcd_osd_set_coeff(LCD_OSD_LAYER osd, uint32_t value);
int ark_lcd_set_osd_mult_coef(LCD_OSD_LAYER osd, uint32_t value);


/* after calling the osd layer set func, you should call */
/* this function to flush the parameters for atom option */
int ark_lcd_set_osd_sync(LCD_OSD_LAYER osd);	

/************************************************************/

int ark_lcd_get_osd_size(LCD_OSD_LAYER osd, uint32_t *width, uint32_t *height);
int ark_lcd_get_osd_format(LCD_OSD_LAYER osd, LCD_OSD_FORMAT *format);
int ark_lcd_get_osd_yaddr(LCD_OSD_LAYER osd, uint32_t *yaddr);
uint32_t ark_lcd_get_virt_addr(void);
uint8_t *ark_lcd_get_fb_addr(uint8_t index);

int ark_lcd_enable(uint8_t enable);
int ark_lcd_wait_for_vsync(void);
int ark_lcd_set_osd_info_atomic(LCD_OSD_LAYER osd, LcdOsdInfo *info);
int ark_lcd_get_osd_info_atomic_isactive(LCD_OSD_LAYER osd);

int lcd_init(void);
void lcd_uninit(void);
void Cpulcd_Init(void);

#endif
