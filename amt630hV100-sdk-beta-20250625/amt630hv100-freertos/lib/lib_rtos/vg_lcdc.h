#ifndef _VG_LCDC_H_
#define _VG_LCDC_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 定义VG的FB模式 ()
// FB_COUNT = 2, 为双缓存模式
// FB_COUNT = 1, 为单帧模式
#ifndef FB_COUNT
#define	FB_COUNT		2
//#define	FB_COUNT		1
#endif


// 获取VG可用的framebuffer个数(固定为2个)
unsigned int xm_vg_get_fb_count (void);


// 获取VG帧(GPU帧)的每行数据字节长度
unsigned int xm_vg_get_stride (void);

// 获取OSD帧的每行数据字节长度
unsigned int xm_vg_get_osd_stride (void);

// 获取VG帧(GPU帧/OSD帧)的bpp设置
// 16 (RGB565)
// 32 (ARGB8888)
unsigned int xm_vg_get_bpp (void);

// 获取一个新的VG帧的GPU fb物理地址, 用于GPU后台渲染
//		VG帧是OSD帧内部的开窗区域, 用于GPU绘图使用
// 返回值
//		指向VG帧的原点(左上角)
unsigned int xm_vg_require_gpu_fb (void);


// 释放当前后台渲染(GPU渲染)所使用的VG帧. 
//		该VG帧(GPU)关联的OSD帧可用于LCD显示更新
void xm_vg_release_gpu_fb (void);

// 检查指定的基址是否是有效的GPU framebuffer
// 返回值
//		1		是有效的GPU framebuffer地址
//		0		不是
int xm_vg_is_valid_gpu_fb (unsigned int base);

// 设置VG帧在OSD帧内部的开窗位置
// 	VG帧属于OSD帧的内部开窗
void xm_vg_set_osd_window (	unsigned int x,		// VG帧原点(左上角)相对于OSD帧原点(左上角)的像素偏移
										unsigned int y,
										unsigned int w,		//	VG帧的像素尺寸
										unsigned int h
									);
void xm_vg_set_x (unsigned int x);
void xm_vg_set_y (unsigned int y);
void xm_vg_set_width (unsigned int w);
void xm_vg_set_height (unsigned int h);


// 读取VG帧在OSD帧内部的开窗位置
// 	VG帧属于OSD帧的内部开窗
void xm_vg_get_osd_window (	unsigned int* x,		// 相对于OSD帧原点(左上角)的像素偏移
										unsigned int* y,
										unsigned int* w,		//	VG帧的像素尺寸
										unsigned int* h,
										unsigned int* stride	// VG帧的每行字节长度
									);
// 获取VG帧相对于OSD帧原点(左上角)的像素偏移
unsigned int xm_vg_get_x (void);
unsigned int xm_vg_get_y (void);
// 获取VG帧的像素宽度
unsigned int xm_vg_get_width (void);
// 获取VG帧的像素高度
unsigned int xm_vg_get_height (void);

// 获取当前正在渲染的OSD framebuffer
//   *no == -1 表示获取当前正在渲染的OSD framebuffer
// 返回值
//		当前正在渲染的OSD framebuffer基址
unsigned int xm_vg_get_osd_fb (int *no);

// 获取VG帧填充的背景图基址(与GPU FB相同尺寸/规格)
void* xm_vg_get_gpu_background_image (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif /* _XM_SIGNAL_H_ */
