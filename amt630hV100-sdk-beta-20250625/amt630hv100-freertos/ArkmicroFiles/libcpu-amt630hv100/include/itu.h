#ifndef _ITU_H
#define _ITU_H

typedef enum {
	ITU_Y_UV = 0,
	ITU_YUYV,
} ITU_YUV_TYPE;

typedef enum {
	ITU_YUV420 = 0,
	ITU_YUV422,
} ITU_OUT_FMT;

typedef struct {
	int in_width;
	int in_height;
	int out_x;
	int out_y;
	int out_width;
	int out_height;
	int out_format;
	int yuv_type;
	int itu601;
} ItuConfigPara;

int itu_init(void);
int itu_config(ItuConfigPara *para);
void itu_start(void);
void itu_stop(void);

#endif
