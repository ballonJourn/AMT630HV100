#include <string.h>
#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "lcd.h"
#include "blend2d.h"
#include "ff_stdio.h"

#define REGS_BLEND2D	REGS_BLEND2D_BASE

/* BLEND2D MODULE */
#define BLEND2D_ENABLE								0x000
#define BLEND2D_START								0x004
#define BLEND2D_CTL_ENABLE							0x008
#define BLEND2D_MODE								0x00C
#define BLEND2D_WRITE_WAIT							0x010
#define BLEND2D_LAYER1_BURST_CTL					0x020
#define BLEND2D_LAYER1_CTL							0x024
#define BLEND2D_LAYER1_SIZE							0x028
#define BLEND2D_LAYER1_ADDR							0x02C
#define BLEND2D_LAYER2_BURST_CTL					0x030
#define BLEND2D_LAYER2_CTL							0x034
#define BLEND2D_LAYER2_SIZE							0x038
#define BLEND2D_LAYER2_ADDR							0x03C
#define BLEND2D_COLOR_KEY_MASK_THLD_LAYER1			0x040
#define BLEND2D_COLOR_KEY_MASK_THLD_LAYER2			0x044
#define BLEND2D_COLOR_KEY_MASK_VALUE_LAYER1			0x048
#define BLEND2D_COLOR_KEY_MASK_VALUE_LAYER2			0x04C
#define BLEND2D_LAYER1_WINDOW_POINT					0x050
#define BLEND2D_LAYER2_WINDOW_POINT					0x054
#define BLEND2D_LAYER1_SOURCE_SIZE					0x058
#define BLEND2D_LAYER2_SOURCE_SIZE					0x05C
#define BLEND2D_WRITE_BACK_SOURCE_SIZE				0x060
#define BLEND2D_BLD_SIZE							0x064
#define BLEND2D_BLD_WINDOW_POINT					0x068
#define BLEND2D_BLD_ADDR							0x06C
#define BLEND2D_BLD_CTL								0x070
#define BLEND2D_LAYER1_BACK_COLOR					0x074
#define BLEND2D_LAYER2_BACK_COLOR					0x078
#define BLEND2D_INT_CTL								0x080
#define BLEND2D_INT_CLR								0x084
#define BLEND2D_INT_STA								0x088

void blend2d_start(void)
{
	writel(1, REGS_BLEND2D + BLEND2D_START);		
	udelay(10);
	writel(0, REGS_BLEND2D + BLEND2D_START);
}

void blend2d_clear_int(void)
{
	writel(0, REGS_BLEND2D + BLEND2D_INT_CLR);
}

void blend2d_read_wait_enable(int enable, unsigned int wait_time)
{
	unsigned int reg;

	reg = readl(REGS_BLEND2D + BLEND2D_MODE);
	reg &= ~((0xffff << 16) | (1 <<5));
	wait_time &= 0xffff;
	if(enable)
		reg |= (wait_time << 16) | (1 << 5);
	writel(reg, REGS_BLEND2D + BLEND2D_MODE);
}

void blend2d_write_wait_enable(unsigned int enable, unsigned int wait_time)
{
	unsigned int reg;

	reg = readl(REGS_BLEND2D + BLEND2D_WRITE_WAIT);
	reg &= ~((1 << 12) | 0xfff);
	wait_time &= 0xfff;
	if(enable)
		reg |= (1 << 12) | wait_time;
	writel(reg, REGS_BLEND2D + BLEND2D_WRITE_WAIT);
}

void blend2d_layer_enable(int layer, int enable)
{
	unsigned int reg = readl(REGS_BLEND2D + BLEND2D_CTL_ENABLE);

	if (layer == BLEND2D_LAYER1) {
		if (enable)
			reg |= 1;
		else
			reg &= ~1;		
	} else if (layer == BLEND2D_LAYER2) {
		if (enable)
			reg |= 1 << 1;
		else
			reg &= ~(1 << 1);
	}
	writel(reg, REGS_BLEND2D + BLEND2D_CTL_ENABLE);
}

void blend2d_set_layer_point(int layer, int x, int y)
{
	x &= 0xfff;
	y &= 0xfff;
	if (layer == BLEND2D_LAYER1)
		writel((y << 12) | x, REGS_BLEND2D + BLEND2D_LAYER1_WINDOW_POINT);
	else if (layer == BLEND2D_LAYER2)
		writel((y << 12) | x, REGS_BLEND2D + BLEND2D_LAYER2_WINDOW_POINT);
}

void blend2d_set_layer_size(int layer, int width, int height)
{
	width &= 0xfff;
	height &= 0xfff;
	if (layer == BLEND2D_LAYER1)
		writel((height << 12) | width, REGS_BLEND2D + BLEND2D_LAYER1_SIZE);
	else if (layer == BLEND2D_LAYER2)
		writel((height << 12) | width, REGS_BLEND2D + BLEND2D_LAYER2_SIZE);
}

void blend2d_set_layer_source_size(int layer, int width, int height)
{
	width &= 0xfff;
	height &= 0xfff;
	if (layer == BLEND2D_LAYER1)
		writel((height << 12) | width, REGS_BLEND2D + BLEND2D_LAYER1_SOURCE_SIZE);
	else if (layer == BLEND2D_LAYER2)
		writel((height << 12) | width, REGS_BLEND2D + BLEND2D_LAYER2_SOURCE_SIZE);
}

void blend2d_set_layer_addr(int layer, unsigned int addr)
{
	if (layer == BLEND2D_LAYER1)
		writel(addr, REGS_BLEND2D + BLEND2D_LAYER1_ADDR);
	else if (layer == BLEND2D_LAYER2)
		writel(addr, REGS_BLEND2D + BLEND2D_LAYER2_ADDR);
}

void blend2d_set_layer_format(int layer, int format, int rgb_order)
{
	format &= 0xf;
	rgb_order &= 0x7;
	if (layer == BLEND2D_LAYER1)
		writel(rgb_order << 4 | format, REGS_BLEND2D + BLEND2D_LAYER1_CTL);
	else if (layer == BLEND2D_LAYER2)
		writel(rgb_order << 4 | format, REGS_BLEND2D + BLEND2D_LAYER2_CTL);
}

void blend2d_set_layer_alpha_mode(int layer, int mode)
{
	unsigned int reg;

	if (layer == BLEND2D_LAYER1) {
		reg = readl(REGS_BLEND2D + BLEND2D_LAYER1_BURST_CTL);
		if (mode == BLEND2D_ALPHA_DATA)
			reg &= ~(1 << 15);
		else if (mode == BLEND2D_ALPHA_REG)
			reg |= 1 << 15;
		writel(reg, REGS_BLEND2D + BLEND2D_LAYER1_BURST_CTL);
	} else if (layer == BLEND2D_LAYER2) {
		reg = readl(REGS_BLEND2D + BLEND2D_LAYER2_BURST_CTL);
		if (mode == BLEND2D_ALPHA_DATA)
			reg &= ~(1 << 15);
		else if (mode == BLEND2D_ALPHA_REG)
			reg |= 1 << 15;
		writel(reg, REGS_BLEND2D + BLEND2D_LAYER2_BURST_CTL);
	}
}

void blend2d_set_layer_alpha(int layer, uint8_t alpha)
{
	unsigned int reg;

	if (layer == BLEND2D_LAYER1) {
		reg = readl(REGS_BLEND2D + BLEND2D_LAYER1_BURST_CTL);
		reg &= ~(0xf << 16);
		reg |= alpha << 16;
		writel(reg, REGS_BLEND2D + BLEND2D_LAYER1_BURST_CTL);
	} else if (layer == BLEND2D_LAYER2) {
		reg = readl(REGS_BLEND2D + BLEND2D_LAYER2_BURST_CTL);
		reg &= ~(0xf << 16);
		reg |= alpha << 16;
		writel(reg, REGS_BLEND2D + BLEND2D_LAYER2_BURST_CTL);
	}
}

void blend2d_set_layer_colorkey(int layer, int enable, uint8_t r, uint8_t g, uint8_t b)
{
	unsigned int reg;

	if (layer == BLEND2D_LAYER1) {
		reg = readl(REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER1);
		reg &= ~0x1ffffff;
		reg |= (r << 16) | (g << 8) | b;
		if (enable)
			reg |= 1 << 24;
		writel(reg, REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER1);		
	} else if (layer == BLEND2D_LAYER2) {
		reg = readl(REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER2);
		reg &= ~0x1ffffff;
		reg |= (r << 16) | (g << 8) | b;
		if (enable)
			reg |= 1 << 24;
		writel(reg, REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER2);
	}
}

void blend2d_set_layer_colorkey_thld(int layer, uint8_t r, uint8_t g, uint8_t b)
{
	if (layer == BLEND2D_LAYER1)
		writel((r << 16) | (g << 8) | b, REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_THLD_LAYER1);
	else if (layer == BLEND2D_LAYER2)
		writel((r << 16) | (g << 8) | b, REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_THLD_LAYER2);
}

void blend2d_set_layer_colorkey_backcolor(int layer, int enable, uint8_t r, uint8_t g, uint8_t b)
{
	unsigned int reg;

	if (layer == BLEND2D_LAYER1) {
		reg = readl(REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER1);
		if (enable)
			reg |= 1 << 25;
		else
			reg &= ~(1 << 25);
		writel(reg, REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER1);
		writel((r << 16) | (g << 8) | b, REGS_BLEND2D + BLEND2D_LAYER1_BACK_COLOR);
	} else if (layer == BLEND2D_LAYER2) {
		reg = readl(REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER2);
		if (enable)
			reg |= 1 << 25;
		else
			reg &= ~(1 << 25);
		writel(reg, REGS_BLEND2D + BLEND2D_COLOR_KEY_MASK_VALUE_LAYER2);
		writel((r << 16) | (g << 8) | b, REGS_BLEND2D + BLEND2D_LAYER2_BACK_COLOR);
	}
}

void blend2d_set_blend_point(int x, int y)
{
	x &= 0xfff;
	y &= 0xfff;
	writel((y << 12) | x, REGS_BLEND2D + BLEND2D_BLD_WINDOW_POINT);
}

void blend2d_set_blend_source_size(int width, int height)
{
	width &= 0xfff;
	height &= 0xfff;
	writel((height << 12) | width, REGS_BLEND2D + BLEND2D_WRITE_BACK_SOURCE_SIZE);
}

void blend2d_set_blend_size(int width, int height)
{
	width &= 0xfff;
	height &= 0xfff;
	writel((height << 12) | width, REGS_BLEND2D + BLEND2D_BLD_SIZE);
}

void blend2d_set_blend_mode(int mode)
{
	unsigned int reg = readl(REGS_BLEND2D + BLEND2D_MODE);
	reg &= ~((0xf << 8) | 0x1);
	reg |= (mode << 8) | (0x1 << 4);
	writel(reg, REGS_BLEND2D + BLEND2D_MODE);
}

void blend2d_set_blend_addr(uint32_t addr)
{
	writel(addr, REGS_BLEND2D + BLEND2D_BLD_ADDR);
}

void blend2d_set_blend_format(int format)
{
	unsigned int reg = readl(REGS_BLEND2D + BLEND2D_BLD_CTL);

	format &= 0xf;
	reg &= ~0xf;
	reg |= format;
	writel(reg, REGS_BLEND2D + BLEND2D_BLD_CTL);
}

void blend2d_set_blend_endian(int endian)
{
	unsigned int reg = readl(REGS_BLEND2D + BLEND2D_BLD_CTL);

	if (endian == BLEND2D_ARGB)
		reg |= 1 << 4;
	else if (endian == BLEND2D_RGBA)
		reg &= ~(1 << 4);
	writel(reg, REGS_BLEND2D + BLEND2D_BLD_CTL);
}

void blend2d_set_blend_alpha(uint8_t        alpha)
{
	unsigned int reg = readl(REGS_BLEND2D + BLEND2D_BLD_CTL);

	reg &= ~(0xff << 24);
	reg |= alpha << 24;
	writel(reg, REGS_BLEND2D + BLEND2D_BLD_CTL);
}

void blend2d_set_blend_alpha_mode(int mode)
{
	unsigned int reg = readl(REGS_BLEND2D + BLEND2D_BLD_CTL);

	mode &= 3;
	reg &= ~((0x1ff << 8) | (3 << 5));
	reg |= (256 << 8) | (mode << 5);
	writel(reg, REGS_BLEND2D + BLEND2D_BLD_CTL);
}

void blend2d_fill(uint32_t address, int xpos, int ypos, int width, int height, int source_width, int source_height, 
				uint8_t cr, uint8_t cg, uint8_t cb, int format, uint8_t opa, int alpha_byte)
{
	/* printf("addr 0x%x, %d,%d,%d,%d-%d,%d r=%d, g=%d, b=%d, format=%d, opa=0x%x.\n",
		address, xpos, ypos, width, height, source_width, source_height, cr, cg, cb, format, opa); */
	blend2d_set_layer_format(BLEND2D_LAYER2, format & 0xf, (format >> 8) & 0xf);
	blend2d_set_layer_addr(BLEND2D_LAYER2, address);
	blend2d_set_layer_point(BLEND2D_LAYER2, 0, 0);
	blend2d_set_layer_size(BLEND2D_LAYER2, width, height);
	blend2d_set_layer_source_size(BLEND2D_LAYER2, width, height);
	blend2d_set_layer_colorkey(BLEND2D_LAYER2, 1, 0, 0, 0);
	blend2d_set_layer_colorkey_thld(BLEND2D_LAYER2, 0xff, 0xff, 0xff);
	blend2d_set_layer_colorkey_backcolor(BLEND2D_LAYER2, 1, cr, cg, cb);
	blend2d_set_layer_alpha(BLEND2D_LAYER2, opa);
	blend2d_set_layer_alpha_mode(BLEND2D_LAYER2, BLEND2D_ALPHA_REG);
	blend2d_layer_enable(BLEND2D_LAYER2, 1);

	blend2d_set_layer_format(BLEND2D_LAYER1, format & 0xf, (format >> 8) & 0xf);
	blend2d_set_layer_addr(BLEND2D_LAYER1, address);
	blend2d_set_layer_point(BLEND2D_LAYER1, 0, 0);
	blend2d_set_layer_size(BLEND2D_LAYER1, width, height);
	blend2d_set_layer_source_size(BLEND2D_LAYER1, source_width, source_height);
	blend2d_layer_enable(BLEND2D_LAYER1, 1);

	blend2d_set_blend_format(format & 0xf);
	blend2d_set_blend_addr(address);
	blend2d_set_blend_point(xpos, ypos);
	blend2d_set_blend_size(width, height);
	blend2d_set_blend_source_size(source_width, source_height);
	blend2d_set_blend_mode(BLEND2D_MIX_BLEND);
	blend2d_set_blend_endian(BLEND2D_ARGB);
	blend2d_set_blend_alpha(0xff);
	if (alpha_byte)
		blend2d_set_blend_alpha_mode(BLEND2D_ALPHA_LAYER2);
	else
		blend2d_set_blend_alpha_mode(BLEND2D_ALPHA_BLEND_REG);

	blend2d_read_wait_enable(1, 0x3f);
}

void blend2d_blit(uint32_t dst_addr, int dst_w, int dst_h, int dst_x, int dst_y, int dst_format, int width, int height, 
				uint32_t src_addr, int src_w, int src_h, int src_x, int src_y, int src_format, uint8_t opa, int alpha_byte)
{
	/* printf("dst 0x%x,%dx%d-%d,%d %dx%d src 0x%x,%dx%d-%d,%d, src_format=%d, opa=0x%x, alpha_byte=%d.\n",
		dst_addr, dst_w, dst_h, dst_x, dst_y, width, height,
		src_addr, src_w, src_h, src_x, src_y, src_format, opa, alpha_byte); */
	blend2d_set_layer_format(BLEND2D_LAYER2, src_format & 0xf, (src_format >> 8) & 0xf);
	blend2d_set_layer_addr(BLEND2D_LAYER2, src_addr);
	blend2d_set_layer_point(BLEND2D_LAYER2, src_x, src_y);
	blend2d_set_layer_size(BLEND2D_LAYER2, width, height);
	blend2d_set_layer_source_size(BLEND2D_LAYER2, src_w, src_h);
	blend2d_set_layer_colorkey(BLEND2D_LAYER2, 0, 0, 0, 0);
	blend2d_set_layer_alpha(BLEND2D_LAYER2, opa);
	if (alpha_byte)
		blend2d_set_layer_alpha_mode(BLEND2D_LAYER2, BLEND2D_ALPHA_DATA);
	else
		blend2d_set_layer_alpha_mode(BLEND2D_LAYER2, BLEND2D_ALPHA_REG);
	blend2d_layer_enable(BLEND2D_LAYER2, 1);

	blend2d_set_layer_format(BLEND2D_LAYER1, dst_format & 0xf, (dst_format >> 8) & 0xf);
	blend2d_set_layer_addr(BLEND2D_LAYER1, dst_addr);
	blend2d_set_layer_point(BLEND2D_LAYER1, dst_x, dst_y);
	blend2d_set_layer_size(BLEND2D_LAYER1, width, height);
	blend2d_set_layer_source_size(BLEND2D_LAYER1, dst_w, dst_h);
	blend2d_layer_enable(BLEND2D_LAYER1, 1);

	blend2d_set_blend_format(dst_format & 0xf);
	blend2d_set_blend_addr(dst_addr);
	blend2d_set_blend_point(dst_x, dst_y);
	blend2d_set_blend_size(width, height);
	blend2d_set_blend_source_size(dst_w, dst_h);
	blend2d_set_blend_mode(BLEND2D_MIX_BLEND);
	blend2d_set_blend_endian(BLEND2D_ARGB);
	//blend2d_set_blend_alpha(0xff);
	if (alpha_byte)
		blend2d_set_blend_alpha_mode(BLEND2D_ALPHA_LAYER1);
	else
		blend2d_set_blend_alpha_mode(BLEND2D_ALPHA_LAYER2);

	blend2d_read_wait_enable(1, 0x3f);	
}


static void blend2d_fill_test(void)
{
	unsigned char *buf = pvPortMalloc(1024*600*4);

	memset(buf, 0, 1024*600*4);
	CP15_clean_dcache_for_dma((uint32_t)buf, (uint32_t)buf + 1024*600*4);

	ark_lcd_set_osd_yaddr(LCD_OSD1, (uint32_t)buf);
	ark_lcd_set_osd_sync(LCD_OSD1);

	blend2d_fill((uint32_t)buf, 0, 0, 1024, 600, 1024, 600, 0xff, 0, 0, BLEND2D_FORAMT_ARGB888, 0x20, 0);
	
	blend2d_run();
}

static void blend2d_demo_thread(void *param)
{
	blend2d_fill_test();
	vTaskDelay(portMAX_DELAY);
}
	
int blend2d_demo(void)
{
	/* Create a task to play animation */
	if (xTaskCreate(blend2d_demo_thread, "axiblendemo", configMINIMAL_STACK_SIZE, NULL,
		configMAX_PRIORITIES / 3, NULL) != pdPASS) {
		printf("create axiblend demo task fail.\n");
		return -1;
	}

	vTaskDelay(portMAX_DELAY);

	return 0;	
}

static QueueHandle_t blend2d_done;

static void blend2d_int_handler(void *para)
{
	unsigned int status;

	status = readl(REGS_BLEND2D + BLEND2D_INT_STA);
	blend2d_clear_int();
	if(status & 0x2) {
		printf("blend2d err int 0x%x.\n", status);
	}
	xQueueSendFromISR(blend2d_done, NULL, 0);
}

int blend2d_init(void)
{
	blend2d_done = xQueueCreate(1, 0);

	request_irq(BLEND2D_IRQn, 0, blend2d_int_handler, NULL);

	writel(0x82, REGS_BLEND2D + BLEND2D_LAYER1_BURST_CTL);
	writel(0x82, REGS_BLEND2D + BLEND2D_LAYER2_BURST_CTL);
	writel(0x3, REGS_BLEND2D + BLEND2D_INT_CTL);
	writel(1, REGS_BLEND2D + BLEND2D_ENABLE);

	return 0;
}

int blend2d_run(void)
{
	xQueueReset(blend2d_done);
	blend2d_start();
	if (xQueueReceive(blend2d_done, NULL, pdMS_TO_TICKS(100)) != pdTRUE) {
		printf("blend2d_run timeout.\n");
		return -1;
	}
	return 0;
}
