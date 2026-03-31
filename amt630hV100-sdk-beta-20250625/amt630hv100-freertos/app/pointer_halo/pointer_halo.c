#include <vg/openvg.h>
#include <vg/vgu.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "vg_font.h"

#include "romfile.h"
#include "vg_driver.h"

#ifdef VG_DRIVER
#if !defined(VG_ONLY) && !defined(AWTK)

extern void *	XM_heap_malloc	(int size);
extern void		XM_heap_free	(void *mem);
extern unsigned int xm_vg_get_bpp (void);
extern unsigned int xm_vg_get_osd_fb (int *no);

#define SCREEN_WIDTH	VG_W
#define SCREEN_HEIGHT   VG_H

// 514, 306为VG区域的中心点坐标
#define	VG_OSD_X		VG_X
#define	VG_OSD_Y		VG_Y
#define	PT_CENTRE_X		VG_W/2
#define	PT_CENTRE_Y		VG_H/2


// 光晕底图aRGB8888
static unsigned int *halo;

static	VGImage maskImage;
static	VGPaint maskPaint;

static VGPath innerPath;
static VGPath outerPath;
static VGPath circlePath;
static VGPaint colorPaint;


// 绘制表盘的光晕效果
// startAngle   光晕对应的开始角度 (笛卡尔坐标系, 顺时针方向)
// angleExtent  光晕的展开角度
static void drawHalo(float startAngle, float  angleExtent )
{
	VGPath  path;
	
	// 设置图像填充模式
	vgPaintPattern(maskPaint, maskImage);
	// 2.2 第一个对应于指针扇区(从指针起始指向角度(0读数)延伸到当前指针读数)的pie行path
	//     该path使用完整光晕的背景底板进行绘制,即得到仅包含有效区域的光晕
	// 定义起始角度 (笛卡尔坐标系)
	vgSetPaint( maskPaint, VG_FILL_PATH ); 
	// 光晕图像与path使用相同的空间尺寸(VG_W, VG_H)
	// 定位到光晕图像的中心点
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();
	vgTranslate( -VG_W/2, -VG_H/2 );
	// 定位到path的中心点
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgTranslate( VG_W/2, VG_H/2 );

	vgSeti ( VG_FILL_RULE, VG_NON_ZERO );
	// 创建一个扇区路径
	path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	//printf ("startAngle=%f, angleExtent=%f\n", startAngle, angleExtent);
	vguArc( path, 0.0f, 0.0f, VG_W, VG_H, startAngle, angleExtent, VGU_ARC_PIE  );
	// 删去以下属性可确保资源使用完毕后立刻被释放
	vgRemovePathCapabilities(path, VG_PATH_CAPABILITY_APPEND_FROM | VG_PATH_CAPABILITY_APPEND_TO |
                                               VG_PATH_CAPABILITY_MODIFY | VG_PATH_CAPABILITY_TRANSFORM_FROM |
                                               VG_PATH_CAPABILITY_TRANSFORM_TO | VG_PATH_CAPABILITY_INTERPOLATE_FROM |
                                               VG_PATH_CAPABILITY_INTERPOLATE_TO);
	// 使用图像填充扇区
	vgDrawPath( path, VG_FILL_PATH );
	// 马上释放资源(资源占有内存资源较大), 否则后续资源分配易失败
	vgClearPath (path, VG_PATH_CAPABILITY_ALL);
	vgDestroyPath( path );
	
	vgSetPaint (VG_INVALID_HANDLE, VG_FILL_PATH);
	vgPaintPattern(maskPaint, VG_INVALID_HANDLE);
}






//size: 490.000000 x 490.000000
static const VGubyte pointer_2_polygon_0_commands[] = {
        VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat pointer_2_polygon_0_coordinates[] = {
        243.998993f, 333.000000f, 241.500000f, 309.584015f, 245.050995f, 7.000000f, 247.951004f, 7.000000f,
        251.500000f, 309.584015f, 249.001007f, 333.000000f
};



static const VGubyte pointer_2_path_1_commands[] = {
        VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_HLINE_TO, VG_LINE_TO, VG_LINE_TO, VG_HLINE_TO, VG_CLOSE_PATH, VG_MOVE_TO,
        VG_HLINE_TO, VG_LINE_TO, VG_LINE_TO, VG_HLINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH

};

static const VGfloat pointer_2_path_1_coordinates[] = {
        247.951004f, 7.000000f, 251.500000f, 309.584015f, 249.001007f, 333.000000f, 243.999008f, 241.500015f,
        309.583984f, 245.050995f, 7.000000f, 247.951004f, 247.951004f, 4.000000f, 245.051010f, 238.500000f,
        309.549011f, 241.015991f, 333.319031f, 249.000992f, 254.482986f, 309.903015f, 250.951004f, 6.965000f,
        247.951004f, 4.000000f
};

static const VGubyte pointer_2_circle_2_commands[] = {
        VG_MOVE_TO, VG_LCCWARC_TO_ABS, VG_SCCWARC_TO_ABS, VG_CLOSE_PATH
};

static const VGfloat pointer_2_circle_2_coordinates[] = {
        246.500000f, 271.998993f, 27.000000f, 27.000000f, 0.000000f, 273.500000f, 244.998993f, 27.000000f,
        27.000000f, 0.000000f, 246.500000f, 271.998993f
};

static VGfloat  fill_linear_gradient_innerPath[] = {
	247.951004f, 7.000000f, 251.500000f, 309.584015f + 24,
};

static VGfloat rampStops_innerPath[30] = {
	0.1857, 0.898f, 0.263f, 0.133f, 1.0f,		//<stop  offset="0.1857" style="stop-color:#E54322"/>
	0.4307, 0.898f, 0.012f, 0.129f, 1.0f,		//<stop  offset="0.4307" style="stop-color:#E50321"/>
	0.9125, 0.898f, 0.122f, 0.122f, 1.0f,		//<stop  offset="0.9125" style="stop-color:#E51F1F"/>
	0.9253, 0.369f, 0.024f,	0.037f, 1.0f,		//<stop  offset="0.9253" style="stop-color:#5E060C"/>
	0.9458, 0.129f, 0.016f, 0.043f, 1.0f,		//<stop  offset="0.9458" style="stop-color:#21040B"/>
	0.9852, 0.247f, 0.000f, 0.031f, 1.0f,		//<stop  offset="0.9852" style="stop-color:#3F0008"/>
};

static VGfloat  fill_linear_gradient_outerPath[] = {
	247.951004f, 7.000000f, 251.500000f, 309.584015f + 24,
};

static VGfloat rampStops_outerPath[40] = {
	0.0040, 0.541f, 0.541f, 0.541f, 1.0f,		//<stop  offset="0.004" style="stop-color:#8A8A8A"/>
	0.1669, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.1669" style="stop-color:#808080"/>
	0.4818, 0.722f, 0.722f, 0.722f, 1.0f,		//<stop  offset="0.4818" style="stop-color:#B8B8B8"/>
	0.9031, 0.800f, 0.800f, 0.800f, 1.0f,		//<stop  offset="0.9031" style="stop-color:#CCCCCC"/>
	0.9174, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.9174" style="stop-color:#808080"/>
	0.9423, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.9423" style="stop-color:#808080"/>
	0.9592, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.9592" style="stop-color:#808080"/>
	0.9852, 0.541f, 0.541f, 0.541f, 1.0f,		//<stop  offset="0.9852" style="stop-color:#8A8A8A"/>
};


static void genPointerPaths (void)
{
	innerPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(innerPath, 7, pointer_2_polygon_0_commands, pointer_2_polygon_0_coordinates);
	outerPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(outerPath, 17, pointer_2_path_1_commands, pointer_2_path_1_coordinates);	
	circlePath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(circlePath, 4, pointer_2_circle_2_commands, pointer_2_circle_2_coordinates);	
	colorPaint = vgCreatePaint();
	vgSetParameteri(colorPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	
#if 1
	// 2.1 定义一个完整光晕的背景底板(带有alpha效果)
	if(maskImage == VG_INVALID_HANDLE)
		maskImage = vgCreateImage( VG_sRGBA_8888, SCREEN_WIDTH, SCREEN_HEIGHT, VG_IMAGE_QUALITY_BETTER );
	//vgImageSubData( maskImage, halo + SCREEN_WIDTH * (SCREEN_HEIGHT - 1), -SCREEN_WIDTH*4, VG_sARGB_8888, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	//vgImageSubData( maskImage, halo + 490 * (490 - 1), -490*4, VG_sRGBA_8888, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	vgImageSubData( maskImage, halo + VG_W * (VG_H - 1), -VG_W*4, VG_sARGB_8888, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	
	
#endif
	maskPaint = vgCreatePaint();
	vgSetParameteri (maskPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
	vgSetParameteri (maskPaint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_PAD);
		
}

#define	POFF_X	+1
#define	POFF_Y	+2

static void drawCursors (float  angleExtent)
{
	VGfloat col[4];
	VGPaint fill;
	VGPaint stroke;
	col[0] = 0xc4/255.0f; 
	col[1] = 0x05/255.0f; 
	col[2] = 0x05/255.0f; 
	col[3] = 1.00f;
	vgSetParameterfv(colorPaint, VG_PAINT_COLOR, 4, col);

	fill = vgCreatePaint();
	vgSetPaint( fill, VG_FILL_PATH );
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();
	//vgTranslate(-PT_CENTRE_X, -PT_CENTRE_Y);
	vgSetParameteri( fill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT );
	vgSetParameterfv( fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient_innerPath );
	vgSetParameterfv( fill, VG_PAINT_COLOR_RAMP_STOPS, 30, rampStops_innerPath );
	vgSetParameteri( fill, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD );
	
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgTranslate(PT_CENTRE_X - POFF_X, PT_CENTRE_Y - POFF_Y);
	vgRotate(angleExtent);
	vgTranslate(-(PT_CENTRE_X - POFF_X), -(PT_CENTRE_Y - POFF_Y));
	
	
	vgDrawPath(innerPath, VG_FILL_PATH);
	vgSetPaint( VG_INVALID_HANDLE, VG_FILL_PATH );
	vgDestroyPaint (fill);
	
	vgSeti ( VG_FILL_RULE, VG_NON_ZERO );
	fill = vgCreatePaint();
	vgSetPaint( fill, VG_FILL_PATH|VG_STROKE_PATH );
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();

	vgSetParameteri( fill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT );
	vgSetParameterfv( fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient_outerPath );
	vgSetParameterfv( fill, VG_PAINT_COLOR_RAMP_STOPS, 40, rampStops_outerPath );
	vgSetParameteri( fill, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD );
	//vgSetPaint(colorPaint, VG_FILL_PATH | VG_STROKE_PATH);
	
	vgDrawPath(outerPath, VG_FILL_PATH|VG_STROKE_PATH);
	//vgSetPaint(colorPaint, VG_FILL_PATH | VG_STROKE_PATH);
	vgSetPaint( VG_INVALID_HANDLE, VG_FILL_PATH|VG_STROKE_PATH );
	vgDestroyPaint (fill);
	
	// 绘制指针上的圆盘
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgTranslate(0, -2);
	//	VGPaint fill;
	col[0] = 0x2b/255.0f; 
	col[1] = 0x2a/255.0f; 
	col[2] = 0x2e/255.0f; 
	col[3] = 1.00f;
	vgSetParameterfv(colorPaint, VG_PAINT_COLOR, 4, col);
	vgSetPaint(colorPaint, VG_FILL_PATH);
	stroke = vgCreatePaint();
	vgSetPaint( stroke, VG_STROKE_PATH );
	col[0] = 0xf8/255.0f; 
	col[1] = 0x40/255.0f; 
	col[2] = 0x00/255.0f; 
	col[3] = 1.00f;
	vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, col);
	vgSetf(VG_STROKE_LINE_WIDTH, 4);
	
	vgDrawPath(circlePath, VG_FILL_PATH|VG_STROKE_PATH);
	vgSetPaint( VG_INVALID_HANDLE, VG_STROKE_PATH );
	vgSetPaint( VG_INVALID_HANDLE, VG_FILL_PATH );
	
	vgSetf(VG_STROKE_LINE_WIDTH, 1);
	vgDestroyPaint (stroke);
	
}

extern Font arial_font;
VGfloat textLineWidth(const Font* font,
                      const char* str) ;

int xm_vg_get_offset_x (void);
int xm_vg_get_offset_y (void);

// font_w 字形的像素宽度
// font_h 字形的像素高度
// speed 显示的字符串文本
// centre_x centre_y 为效果图中对应的速度文本其显示区域的中心点坐标(相对于效果图左上角原点的偏移)
static void DrawSpeed(char *speed, int font_w, int font_h, int centre_x, int centre_y, unsigned int color) 
{
	VGfloat col[4];
	int str_size;
	VGfloat glyphOrigin[2] = { 0.0f, 0.0f };
	float speed_scale_x,  speed_scale_y;
	
	// 将效果图中的坐标值转换为VG区域中的坐标值
	int vg_offset_x = centre_x - xm_vg_get_offset_x();
	int vg_offset_y = centre_y - xm_vg_get_offset_y();
	vg_offset_y = xm_vg_get_height() - 1 - vg_offset_y;
	
	str_size = strlen (speed);
	
	// 计算数字字符的逻辑宽度/高度(等宽字体)
	vgTextSize (&arial_font, "2", &speed_scale_x, &speed_scale_y);
	// 字符串像素宽度
	float str_width = font_w * str_size;
	// 字符串像素高度
	float str_height = font_h / speed_scale_y;

	// 设置车速显示时的文字颜色 (白色)
	col[0] = ((color >> 0) & 0xff) / 255.0; 	// R
	col[1] = ((color >> 8) & 0xff) / 255.0; 	// G
	col[2] = ((color >> 16) & 0xff) / 255.0; 	// B
	col[3] = 1.00f;
	vgSetParameterfv(colorPaint, VG_PAINT_COLOR, 4, col);
	vgSetPaint( colorPaint, VG_FILL_PATH|VG_STROKE_PATH );
	
	vgSeti(VG_FILL_RULE, VG_NON_ZERO);
	vgSetfv(VG_GLYPH_ORIGIN, 2, glyphOrigin);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
	vgLoadIdentity();
	// 定位显示的原点
	vgTranslate(vg_offset_x - str_width / 2, vg_offset_y - str_height/2 );
	// 计算字形的放大倍数
	vgScale(font_w / speed_scale_x , font_h / speed_scale_y);
	// arial_font为车速显示使用的字形数据
	vgTextOut (&arial_font, speed, VG_FILL_PATH);
	
	vgSetPaint( VG_INVALID_HANDLE, VG_FILL_PATH|VG_STROKE_PATH );
}


#define	RGB(r,g,b)	((r) | ((g) << 8) | ((b) << 16))


	float startAngle = 212.0f;



static void draw_speed_level (int speed_level)
{
	char text[8];
	
	sprintf (text, "%d", speed_level);
	DrawSpeed (text, 42, 42, 514, 498, RGB(255,255,255)); 
	
}

// 绘制表盘上的刻度值 (0, 30, ... 330, 360)
static void draw_scale_value (void)
{
	// 字体大小
	int fw, fh;
	fw = 16;
	fh = 20;
	// 以下坐标为LCD屏幕坐标
	DrawSpeed ( "0", fw, fh, 328 + 16, 398 + 8, RGB(255,255,255)); 
	DrawSpeed ("30", fw, fh, 306 + 16, 322 + 18, RGB(255,255,255)); 
	DrawSpeed ("60", fw, fh, 306 + 16, 254 + 14, RGB(255,255,255)); 
	DrawSpeed ("90", fw, fh, 329 + 16, 195 + 10, RGB(255,255,255)); 
	DrawSpeed ("120", fw, fh, 375 + 16, 146 + 10, RGB(255,255,255)); 
	DrawSpeed ("150", fw, fh, 419 + 20, 111 + 14, RGB(255,255,255)); 
	DrawSpeed ("180", fw, fh, 494 + 16, 98 + 6, RGB(255,255,255)); 
	DrawSpeed ("210", fw, fh, 562 + 16, 111 + 14, RGB(255,255,255)); 
	DrawSpeed ("240", fw, fh, 613 + 20, 146 + 10, RGB(255,0,0)); 
	DrawSpeed ("270", fw, fh, 646 + 20, 192 + 10, RGB(255,0,0)); 
	DrawSpeed ("300", fw, fh, 660 + 32, 259 + 14, RGB(255,0,0)); 
	DrawSpeed ("330", fw, fh, 661 + 32, 332 + 14, RGB(255,0,0)); 
	DrawSpeed ("360", fw, fh, 641 + 36, 393 + 18, RGB(255,0,0)); 

}

extern void* xm_vg_get_gpu_background_image (void);

void do_paint (float angleExtent, int speed)
{
	//VGImage image;

	// 1 绘制指针仪表的背景图 (仅覆盖VG区域)
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	void *bkImage = xm_vg_get_gpu_background_image ();
	//image = vgCreateImage( VG_sRGB_565, SCREEN_WIDTH, SCREEN_HEIGHT, VG_IMAGE_QUALITY_BETTER );
	//vgDrawImage( image );
	//vgDestroyImage( image );
	vgScale (1.0, 1.0);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgWritePixels (bkImage, -SCREEN_WIDTH * VG_BPP / 8, VG_sRGB_565, 0, 0, 
		SCREEN_WIDTH, SCREEN_HEIGHT);
	
	// 2 叠加光晕效果
	drawHalo (startAngle, angleExtent);

	//vgDestroyPaint (maskPaint);
	//vgDestroyImage (maskImage);
	
	// 3 绘制刻度盘上的刻度值
	draw_scale_value ();
	
	// 4 绘制指针
	drawCursors (startAngle + angleExtent + 90);
	
	// 5 绘制表盘底部的刻度
	draw_speed_level (speed);
	
}

/* speed: 0~360 */
static void draw_dial(int speed)
{
	float angleExtent = -2.0;
	
	// 将速度表读数映射为表盘角度
	angleExtent = -2.0 + (-242.0f - (-2.0)) * speed / 360.0;
	if(angleExtent > (-2.0))
		angleExtent = -2.0;
	else if(angleExtent < (-242.0f))
		angleExtent = -242.0f;
			
	do_paint (angleExtent, speed);
	
	vgFinish();
}

static RomFile *halofile = NULL;
int pointer_halo_init (int width, int height)
{
	halofile = RomFileOpen("halo.rgb");
	//XM_printf ("pointer_halo_init\n");
	if (!halofile) {
		printf("open romfile halo.rgb fail.\n");
		return -1;
	}
	RomFileRead(halofile, NULL, halofile->size);
	halo = (unsigned int *)halofile->buf;
#ifndef ROMFILE_USE_SMALL_MEM
	RomFileClose(halofile);
#endif
	vgFontInit ();
	genPointerPaths ();
	return 0;
}

// 退出并使用资源
int pointer_halo_exit (void)
{
	//XM_printf ("pointer_halo_exit\n");
	if(maskPaint != VG_INVALID_HANDLE)
	{
		vgDestroyPaint (maskPaint);
		maskPaint = VG_INVALID_HANDLE;
	}
	/*
	if(maskImage != VG_INVALID_HANDLE)
	{
		vgDestroyImage (maskImage);
		maskImage = VG_INVALID_HANDLE;
	}*/
	if(colorPaint != VG_INVALID_HANDLE)
	{
		vgDestroyPaint (colorPaint);
		colorPaint = VG_INVALID_HANDLE;
	}
	
	if(circlePath != VG_INVALID_HANDLE)
	{
		vgRemovePathCapabilities(circlePath, VG_PATH_CAPABILITY_APPEND_FROM | VG_PATH_CAPABILITY_APPEND_TO |
                                               VG_PATH_CAPABILITY_MODIFY | VG_PATH_CAPABILITY_TRANSFORM_FROM |
                                               VG_PATH_CAPABILITY_TRANSFORM_TO | VG_PATH_CAPABILITY_INTERPOLATE_FROM |
                                               VG_PATH_CAPABILITY_INTERPOLATE_TO);
		vgClearPath (circlePath, VG_PATH_CAPABILITY_ALL);
		vgDestroyPath( circlePath );
		circlePath = VG_INVALID_HANDLE;
	}
	if(outerPath != VG_INVALID_HANDLE)
	{
		vgRemovePathCapabilities(outerPath, VG_PATH_CAPABILITY_APPEND_FROM | VG_PATH_CAPABILITY_APPEND_TO |
                                               VG_PATH_CAPABILITY_MODIFY | VG_PATH_CAPABILITY_TRANSFORM_FROM |
                                               VG_PATH_CAPABILITY_TRANSFORM_TO | VG_PATH_CAPABILITY_INTERPOLATE_FROM |
                                               VG_PATH_CAPABILITY_INTERPOLATE_TO);
		vgClearPath (outerPath, VG_PATH_CAPABILITY_ALL);
		vgDestroyPath( outerPath );
		outerPath = VG_INVALID_HANDLE;
	}	
	if(innerPath != VG_INVALID_HANDLE)
	{
		vgRemovePathCapabilities(innerPath, VG_PATH_CAPABILITY_APPEND_FROM | VG_PATH_CAPABILITY_APPEND_TO |
                                               VG_PATH_CAPABILITY_MODIFY | VG_PATH_CAPABILITY_TRANSFORM_FROM |
                                               VG_PATH_CAPABILITY_TRANSFORM_TO | VG_PATH_CAPABILITY_INTERPOLATE_FROM |
                                               VG_PATH_CAPABILITY_INTERPOLATE_TO);
		vgClearPath (innerPath, VG_PATH_CAPABILITY_ALL);
		vgDestroyPath( innerPath );
		innerPath = VG_INVALID_HANDLE;
	}	
		
	vgFontExit();

#ifdef ROMFILE_USE_SMALL_MEM
	RomFileClose(halofile);
#endif

	return 0;
}

int pointer_halo_draw (int speed)
{
	// 使用VG绘制中间的表盘区域
	draw_dial (speed);
	
	return 0;
}

#endif
#endif
