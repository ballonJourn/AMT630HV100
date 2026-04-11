/*
 * blend2d.h
 *
 */

#ifndef _BLEND2D_H
#define _BLEND2D_H

typedef enum {
	BLEND2D_RGBA = 0,
	BLEND2D_ARGB,
} BLEND2D_BLEND_ENDIAN;

typedef enum {
	BLEND2D_RGB = 0,
	BLEND2D_RBG,
	BLEND2D_GRB,
	BLEND2D_GBR,
	BLEND2D_BRG,
	BLEND2D_BGR,
} BLEND2D_RGB_ORDER;

typedef enum {
	BLEND2D_FORAMT_RGB565 = 5,
	BLEND2D_FORAMT_ARGB888 = 6,
	BLEND2D_FORAMT_BGR565 = BLEND2D_FORAMT_RGB565 | (5 << 8),
	BLEND2D_FORMAT_ABGR888 = BLEND2D_FORAMT_ARGB888 | (5 << 8),
} BLEND2D_FORMAT;

typedef enum {
	BLEND2D_ALPHA_DATA = 0,
	BLEND2D_ALPHA_REG,
} BLEND2D_LAYER_ALPHA_MODE;

typedef enum {
	BLEND2D_ALPHA_LAYER1 = 0,
	BLEND2D_ALPHA_LAYER2,
	BLEND2D_ALPHA_BLEND_REG,
} BLEND2D_BLEND_ALPHA_MODE;

typedef enum {
	BLEND2D_MIX_BLEND = 0,
	BLEND2D_MIX_LAYER1 = 1,
	BLEND2D_MIX_LAYER2 = 2,
	BLEND2D_MIX_LAYER1_COLORKEY_COVER_TRANSP = 3,
	BLEND2D_MIX_LAYER2_COLORKEY_COVER_TRANSP = 0xc,
	BLEND2D_MIX_LAYER2_COLORKEY_BLEND_COVER = 0xd,
	BLEND2D_MIX_LAYER2_COLORKEY_BLEND_TRANSP = 0xe,
} BLEND2D_BLEND_MIX_MODE;

typedef enum {
	BLEND2D_LAYER1 = 0,
	BLEND2D_LAYER2,
	BLEND2D_NUMS,
} BLEND2D_LAYER;

int blend2d_demo(void);
int blend2d_init(void);
void blend2d_fill(uint32_t address, int xpos, int ypos, int width, int height, int source_width, int source_height, 
				uint8_t cr, uint8_t cg, uint8_t cb, int format, uint8_t opa, int alpha_byte);
void blend2d_blit(uint32_t dst_addr, int dst_w, int dst_h, int dst_x, int dst_y, int dst_format, int width, int height, 
				uint32_t src_addr, int src_w, int src_h, int src_x, int src_y, int src_format, uint8_t opa, int alpha_byte);
int blend2d_run(void);


#endif

