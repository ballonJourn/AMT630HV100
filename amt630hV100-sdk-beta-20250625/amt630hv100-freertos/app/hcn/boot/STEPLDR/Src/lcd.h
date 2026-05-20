/*
 * ark_lcd.h
 *
 */

#ifndef _LCD_H
#define _LCD_H

typedef enum {
	LCD_OSD0 = 0,
	LCD_OSD1,
	LCD_OSD_NUMS,
}LCD_OSD_LAYER;

typedef enum {
	LCD_OSD_FORAMT_YUV420 = 0,
	LCD_OSD_FORAMT_ARGB888,
	LCD_OSD_FORAMT_RGB565,
	LCD_OSD_FORAMT_RGB454,
	LCD_OSD_FORAMT_NUMS,
}LCD_OSD_FORMAT;

typedef enum {
	LCD_OSD_Y_U_V420 = 0,
	LCD_OSD_Y_UV420,
}LCD_OSD_YUV420_MODE;

#define LCD_UI_LAYER	LCD_OSD1


/* osd layer set func */
/************************************************************/
int ark_lcd_set_osd_size(LCD_OSD_LAYER osd, uint32_t width, uint32_t height);
int ark_lcd_set_osd_format(LCD_OSD_LAYER osd, LCD_OSD_FORMAT format);
int ark_lcd_set_osd_yaddr(LCD_OSD_LAYER osd, uint32_t yaddr);
int ark_lcd_set_osd_uaddr(LCD_OSD_LAYER osd, uint32_t yaddr);
int ark_lcd_set_osd_vaddr(LCD_OSD_LAYER osd, uint32_t yaddr);
int ark_lcd_osd_enable(LCD_OSD_LAYER osd, uint8_t enable);

/* Interface with default argument value, may be you need them. If you not sure, ingore them */
int ark_lcd_set_osd_possition(LCD_OSD_LAYER osd, uint32_t h, uint32_t v);
int ark_lcd_set_osd_h_offset(LCD_OSD_LAYER osd, uint32_t offset);
int ark_lcd_osd_coeff_enable(LCD_OSD_LAYER osd, uint8_t enable);
int ark_lcd_osd_set_coeff(LCD_OSD_LAYER osd, uint32_t value);
int ark_lcd_set_osd_mult_coef(LCD_OSD_LAYER osd, uint32_t value);
int ark_lcd_set_osd_yuv420_mode(LCD_OSD_LAYER osd, LCD_OSD_YUV420_MODE mode);

/* after calling the osd layer set func, you should call */
/* this function to flush the parameters for atom option */
int ark_lcd_set_osd_sync(LCD_OSD_LAYER osd);	

/************************************************************/

int ark_lcd_get_osd_size(LCD_OSD_LAYER osd, uint32_t *width, uint32_t *height);
int ark_lcd_get_osd_format(LCD_OSD_LAYER osd, LCD_OSD_FORMAT *format);
int ark_lcd_get_osd_yaddr(LCD_OSD_LAYER osd, uint32_t *yaddr);
uint32_t ark_lcd_get_virt_addr(void);

int ark_lcd_enable(uint8_t enable);
int ark_lcd_wait_for_vsync(void);

int lcd_init(void);
void lcd_uninit(void);

#endif
