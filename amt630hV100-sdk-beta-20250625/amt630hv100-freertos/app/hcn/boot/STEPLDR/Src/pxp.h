#ifndef _PXP_H
#define _PXP_H

typedef enum {
	PXP_SRC_FMT_RGB888 = 0x1, /* 32-bit pixels (unpacked 24-bit format) */
	PXP_SRC_FMT_RGB565 = 0x4, /* 16-bit pixels */
	PXP_SRC_FMT_RGB555 = 0x5, /* 16-bit pixels */
	PXP_SRC_FMT_YUV422 = 0x8, /* 16-bit pixels */
	PXP_SRC_FMT_YUV420 = 0x9, /* 16-bit pixels */
	PXP_SRC_FMT_UYVY1P422 = 0xa, /* 16-bit pixels (1-plane U0,Y0,V0,Y1 interleaved bytes) */
	PXP_SRC_FMT_VYUY1P422 = 0xb, /* 16-bit pixels (1-plane V0,Y0,U0,Y1 interleaved bytes) */
	PXP_SRC_FMT_YUV2P422 = 0xc, /* 16-bit pixels (2-plane UV interleaved bytes) */
	PXP_SRC_FMT_YUV2P420 = 0xd, /* 16-bit pixels */
	PXP_SRC_FMT_YVU2P422 = 0xe, /* 16-bit pixels (2-plane VU interleaved bytes) */
	PXP_SRC_FMT_YVU2P420 = 0xf, /* 16-bit pixels */
} PXP_SRC_FMT;

typedef enum {
	PXP_OUT_FMT_ARGB8888 = 0x0, /* 32-bit pixels */
	PXP_OUT_FMT_RGB888 = 0x1, /* 32-bit pixels (unpacked 24-bit pixel in 32 bit DWORD.) */
	PXP_OUT_FMT_RGB888P = 0x2, /* 24-bit pixels (packed 24-bit format) */
	PXP_OUT_FMT_ARGB1555 = 0x3, /* 16-bit pixels */
	PXP_OUT_FMT_RGB565 = 0x4, /* 16-bit pixels */
	PXP_OUT_FMT_RGB555 = 0x5, /* 16-bit pixels */
	PXP_OUT_FMT_YUV444 = 0x7, /* 32-bit pixels (1-plane XYUV unpacked) */
	PXP_OUT_FMT_UYVY1P422 = 0xa, /* 16-bit pixels (1-plane U0,Y0,V0,Y1 interleaved bytes) */
	PXP_OUT_FMT_VYUY1P422 = 0xb, /* 16-bit pixels (1-plane V0,Y0,U0,Y1 interleaved bytes) */
	PXP_OUT_FMT_YUV2P422 = 0xc, /* 16-bit pixels (2-plane UV interleaved bytes) */
	PXP_OUT_FMT_YUV2P420 = 0xd, /* 16-bit pixels (2-plane VU) */
	PXP_OUT_FMT_YVU2P422 = 0xe, /* 16-bit pixels (2-plane VU interleaved bytes) */
	PXP_OUT_FMT_YVU2P420 = 0xf, /* 16-bit pixels (2-plane VU) */
} PXP_OUT_FMT;

/* clockwise rotate */
typedef enum {
	PXP_ROTATE_0 = 0,
	PXP_ROTATE_90 = 1,
	PXP_ROTATE_180 = 2,
	PXP_ROTATE_270 = 3,
} PXP_ROTATE;

int pxp_scaler_rotate(uint32_t s0buf, uint32_t s0ubuf, uint32_t s0vbuf,
					int s0format, uint32_t s0width, uint32_t s0height,
					uint32_t outbuf, uint32_t outbuf2, int outformat,
					uint32_t outwidth, uint32_t outheight, int outangle);
int pxp_rotate(uint32_t s0buf, uint32_t s0ubuf, uint32_t s0vbuf,
					int s0format, uint32_t s0width, uint32_t s0height,
					uint32_t outbuf, uint32_t outbuf2, int outformat,
					int outangle);

#endif
