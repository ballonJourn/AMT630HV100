#include <vg/openvg.h>
#include <vg/vgu.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "vg_font.h"
#include "vg_image.h"


#ifdef SINGLE_POINTER_HALO

#define SCREEN_WIDTH	1024
#define SCREEN_HEIGHT   600

#define	VG_W			1024
#define	VG_H			600
#define	VG_OSD_X		0
#define	VG_OSD_Y		0
#define	OSD_W			1024
#define	OSD_H			600

// 光晕底图的尺寸定义(.\bin\halo.RGB为496*496 ARGB格式)
#define	HALO_W	496
#define	HALO_H	496

#ifdef BG_RGB565
#define	VG_BPP		16
#else
#define	VG_BPP		32
#endif

#define ROM_IMAGE_BG_RGB16_RGB565					"bg_RGB16.RGB565"
#define ROM_IMAGE_HALO_496X496_ARGB8888				"halo_496x496.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_0_2X_ARGB8888		"rotate_speed_0_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_1_2X_ARGB8888		"rotate_speed_1_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_2_2X_ARGB8888		"rotate_speed_2_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_3_2X_ARGB8888		"rotate_speed_3_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_4_2X_ARGB8888		"rotate_speed_4_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_5_2X_ARGB8888		"rotate_speed_5_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_6_2X_ARGB8888		"rotate_speed_6_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_7_2X_ARGB8888		"rotate_speed_7_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_8_2X_ARGB8888		"rotate_speed_8_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_9_2X_ARGB8888		"rotate_speed_9_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_10_2X_ARGB8888		"rotate_speed_10_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_11_2X_ARGB8888		"rotate_speed_11_2x.ARGB8888"
#define ROM_IMAGE_ROTATE_SPEED_12_2X_ARGB8888		"rotate_speed_12_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_0_2X_ARGB8888			"water_temp_0_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_1_2X_ARGB8888			"water_temp_1_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_2_2X_ARGB8888			"water_temp_2_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_3_2X_ARGB8888			"water_temp_3_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_4_2X_ARGB8888			"water_temp_4_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_5_2X_ARGB8888			"water_temp_5_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_6_2X_ARGB8888			"water_temp_6_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_7_2X_ARGB8888			"water_temp_7_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_8_2X_ARGB8888			"water_temp_8_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_9_2X_ARGB8888			"water_temp_9_2x.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_10_2X_ARGB8888			"water_temp_10_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_0_2X_ARGB8888			"oil_mass_0_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_1_2X_ARGB8888			"oil_mass_1_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_2_2X_ARGB8888			"oil_mass_2_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_3_2X_ARGB8888			"oil_mass_3_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_4_2X_ARGB8888			"oil_mass_4_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_5_2X_ARGB8888			"oil_mass_5_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_6_2X_ARGB8888			"oil_mass_6_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_7_2X_ARGB8888			"oil_mass_7_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_8_2X_ARGB8888			"oil_mass_8_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_9_2X_ARGB8888			"oil_mass_9_2x.ARGB8888"
#define ROM_IMAGE_OIL_MASS_10_2X_ARGB8888			"oil_mass_10_2x.ARGB8888"
#define ROM_IMAGE_LEFT_2X_ARGB8888					"left_2x.ARGB8888"
#define ROM_IMAGE_RIGHT_2X_ARGB8888					"right_2x.ARGB8888"
#define ROM_IMAGE_HIGH_BEAM_2X_ARGB8888				"high_beam_2x.ARGB8888"
#define ROM_IMAGE_FOG_LAMPS_2X_ARGB8888				"fog_lamps_2x.ARGB8888"
#define ROM_IMAGE_DIPPED_HEADLIGHT_2X_ARGB8888		"dipped_headlight_2x.ARGB8888"
#define ROM_IMAGE_CORRIDOR_LAMP_2X_ARGB8888			"corridor_lamp_2x.ARGB8888"
#define ROM_IMAGE_BRAKE_LIGHTS_2X_ARGB8888			"brake_lights_2x.ARGB8888"
#define ROM_IMAGE_TRIP_A_2X_ARGB8888				"TRIP_A_2x.ARGB8888"
#define ROM_IMAGE_TRIP_KM_2X_ARGB8888				"TRIP_km_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_0_ARGB8888				"TRIP_0_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_1_ARGB8888				"TRIP_1_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_2_ARGB8888				"TRIP_2_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_3_ARGB8888				"TRIP_3_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_4_ARGB8888				"TRIP_4_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_5_ARGB8888				"TRIP_5_2x.ARGB8888"
#define ROM_IMAGE_ICON_2X_6_ARGB8888				"TRIP_6_2x.ARGB8888"
#define ROM_IMAGE_X1000R_MIN_2X_ARGB8888			"x1000r_min_2x.ARGB8888"
#define ROM_IMAGE_OIL_CONSUMPTION_2X_ARGB8888		"oil_consumption_2x.ARGB8888"


// 将SVG画布坐标转换为OpenVG坐标(仅转换Y轴坐标), 从自顶向下转换为自底向上
// cy是基于SVG坐标(X轴自左向右, Y轴自顶向下, PC画笔也是)的某个画布的高度, y是该画布下的Y轴坐标
#define	MAP_SVG_TO_OPENVG(y,cy)	(cy - 1.0f - (y))


extern void XM_printf(char *fmt, ...);
extern unsigned int xm_vg_get_bpp (void);
extern unsigned int xm_vg_get_osd_fb (int *no);
extern char* XM_RomAddress (const char *src);
extern void xm_icon_state_reset(void);
static void draw_icon(int icon_id);
static void init_icons(void);
static void icon_state_simulate(void);
// 重新绘制icon对应位置的背景图
static void draw_icon_background (int icon_id);
void bitblt_test(void);

// 绘制指针包络矩形
#define	DRAW_POINTER_BOUNDING_RECT

// 读取VG帧在OSD帧内部的开窗位置
// 	VG帧属于OSD帧的内部开窗
void xm_vg_get_osd_window_old(unsigned int* x,		// 相对于OSD帧原点(左上角)的像素偏移
	unsigned int* y,
	unsigned int* w,		//	VG帧的像素尺寸
	unsigned int* h,
	unsigned int* stride	// VG帧的每行字节长度
)
{
	*x = 0;
	*y = 0;
	*w = VG_W;
	*h = VG_H;
	*stride = VG_W * VG_BPP / 8;
}

// 获取OSD帧的每行数据字节长度
unsigned int xm_vg_get_osd_stride_pc(void)
{
	return VG_W * VG_BPP / 8;
}


// 请按照表盘的中心点坐标(屏幕坐标)确定以下指针的位置(PC画笔坐标, 自顶向下), 一般需要设计师给出准确的坐标.
#define	PT_CENTRE_X		(VG_W/2+1.50)
#define	PT_CENTRE_Y		304//(VG_H/2-5.5)

// 背景图RGB565
static unsigned short *bkimg;
// 光晕底图aRGB8888
static unsigned int *halo;

// 光晕底图Image对象, 用于扇形区域填充
static	VGImage maskImage;
// 光晕画笔对象
static	VGPaint maskPaint;

// 指针路径对象(SVG设计图将整个指针分为内径/外径/圆盘3部分)
// 参考".\image\原始设计数据\指针\pointer_2.svg"
static VGPath innerPath;		// 指针内径路径对象
static VGPath outerPath;		// 指针外径路径对象
static VGPath circlePath;		// 指针圆盘路径对象

static VGPaint colorPaint;

// 全画面输出次数 
static int full_dial_output_count = 2;  // 2个framebuffer
static VGint dirty_rects_coords[2][4];	// 保存与指针相关的脏矩形
static int dirty_rects_count[2];		// 记录脏矩形的个数, 固定为1
static float dial_vehicle_speeds[2];	// 记录对应于2个FrameBuffer的表盘的时速值



unsigned int* get_halo_image(void)
{
	return halo;
}


unsigned short* get_bk_image(void)
{
	return (unsigned short *)bkimg;
}

// 使用红色填充扇形区域, 用于测试扇形区域的中心位置,扇形区域的开始/结束角度 
//#define	HALO_FILL_PURE_COLOR

// 绘制表盘的光晕效果
// startAngle   光晕对应的开始角度 (笛卡尔坐标系, 逆时针方向)
// angleExtent  光晕的展开角度
// startAngle + angleExtent 为光晕展开的最大角度
// pointer_x, floater_y 指针圆盘的中心点坐标(屏幕坐标)
static void drawHalo(float startAngle, float  angleExtent, float pointer_x, float floater_y)
{
	VGPath  path;

	// 光晕角度低于0.002时, 不再绘制扇形光晕, 避免底层vguArc算法的浮点计算异常
	// 测试中0.001不会导致浮点计算异常. 此处将范围放宽一倍.
	if ( fabs(angleExtent) < 0.002) 
		return;

#ifdef HALO_FILL_PURE_COLOR	

	// 测试代码, 
	// 使用红色填充扇形区域, 用于测试扇形区域的中心位置,扇形区域的开始/结束角度 

	VGfloat col[4];
	col[0] = 0xff / 255.0f;
	col[1] = 0x00/ 255.0f;
	col[2] = 0x00 / 255.0f;
	col[3] = 1.00f;
	vgSetParameterfv(colorPaint, VG_PAINT_COLOR, 4, col);
	vgSetPaint(colorPaint, VG_FILL_PATH);	// PATH填充使用

#else
	
	// 设置光晕图为扇形填充所用画笔的底图
	vgPaintPattern(maskPaint, maskImage);
	// 设置图像填充模式
	vgSetPaint( maskPaint, VG_FILL_PATH ); 
	// 光晕图像与path使用相同的空间尺寸(VG_W, VG_H)
	// 定位到光晕图像的中心点
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();	// 设置单位矩阵, 此时OpenVG原点在VG画布的左下角
	// 将OpenVG原点平移至光晕的中心点
	vgTranslate( -HALO_W/2, -HALO_H/2 );
#endif
	
	// 定位到path的中心点
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgTranslate (pointer_x, floater_y );

	vgSeti ( VG_FILL_RULE, VG_NON_ZERO );
	// 创建一个扇区路径
	path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	//printf ("startAngle=%f, angleExtent=%f\n", startAngle, angleExtent);
	vguArc( path, 0.0f, 0.0f, HALO_W, HALO_H, startAngle, angleExtent, VGU_ARC_PIE  );
	// 删去以下属性可确保资源使用完毕后立刻被释放
	vgRemovePathCapabilities(path, VG_PATH_CAPABILITY_APPEND_FROM | VG_PATH_CAPABILITY_APPEND_TO |
                                               VG_PATH_CAPABILITY_MODIFY | VG_PATH_CAPABILITY_TRANSFORM_FROM |
                                               VG_PATH_CAPABILITY_TRANSFORM_TO | VG_PATH_CAPABILITY_INTERPOLATE_FROM |
                                               VG_PATH_CAPABILITY_INTERPOLATE_TO);
	// 使用图像填充扇区
	vgDrawPath( path, VG_FILL_PATH );
	// 马上释放资源(资源占有内存资源较大), 否则后续资源分配易失败
	//vgClearPath (path, VG_PATH_CAPABILITY_ALL);
	vgDestroyPath( path );
	
	// 此处将一个无效句柄置为填充用途, 标记画笔对象maskPaint不再使用用于填充用途, 以便将画笔对象maskPaint的资源释放.
	vgSetPaint (VG_INVALID_HANDLE, VG_FILL_PATH);
	// 标记画笔对象maskPaint不再使用image对象maskImage, 以便maskImage资源释放
	vgPaintPattern(maskPaint, VG_INVALID_HANDLE);
}



// ************************************* 表盘指针 ***************************
//
// 1) 表盘的指针共用一个SVG设计, 参考".\image\原始设计数据\指针\pointer_2.svg", 使用UltraEdit可以打开并编辑SVG文件. 或者使用矢量绘图工具AI打开.
// 2) 使用SVG2OPENVG.exe可以将SVG格式的指针数据提取并生成如下初始的格式
//     2.1 将SVG2OPENVG.exe与指针.svg拷贝到相同的目录下, 如 "e:\proj\HMI\SW\OPENVG_DEMO\single_pointer_halo\bin"
//     2.2 在命令行模式下, 执行 "e:\>cd \proj\HMI\SW\OPENVG_DEMO\single_pointer_halo\bin"
//     2.3 在命令行模式下, 执行"SVG2OPENVG.exe pointer_2.svg"
//     2.4 输出如下矢量路径的文本信息
// svg file (pointer_2.svg)
// svg image's size: 490.000000 x 490.000000
// ...
//     2.5 或直接执行".\image\原始设计数据\指针\run_pointer_2.bat"
//
// 3) 将其粘贴到代码中并修改. 加入表盘中心点的偏移(CIRCLE_0_X, CIRCLE_0_Y), 加入指针Y方向的偏移(PT_OFF_Y), 修改后代码如下


// 以下由工具(SVG2OPENVG.exe)自动产生. 
//size: 490.000000 x 490.000000
#define pointer_2_cx 490.000000f
#define pointer_2_cy 490.000000f
static const VGubyte pointer_2_polygon_0_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat pointer_2_polygon_0_coordinates[] = {
	243.998993f, 156.000000f, 241.500000f, 179.415985f, 245.050995f, 482.000000f, 247.951004f, 482.000000f,
	251.500000f, 179.415985f, 249.001007f, 156.000000f
};

static const VGubyte pointer_2_path_1_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_HLINE_TO, VG_LINE_TO, VG_LINE_TO, VG_HLINE_TO, VG_MOVE_TO,
	VG_HLINE_TO, VG_CUBIC_TO, VG_LINE_TO, VG_CUBIC_TO, VG_LINE_TO, VG_CUBIC_TO, VG_HLINE_TO, VG_CUBIC_TO,
	VG_LINE_TO, VG_CUBIC_TO, VG_LINE_TO, VG_CUBIC_TO, VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat pointer_2_path_1_coordinates[] = {
	247.951004f, 482.000000f, 251.500000f, 179.415985f, 249.001007f, 156.000000f, 243.999008f, 241.500015f,
	179.416016f, 245.050995f, 482.000000f, 247.951004f, 247.951004f, 485.000000f, 245.051010f, 243.408005f,
	485.000000f, 242.071014f, 483.678009f, 242.051010f, 482.035004f, 238.500000f, 179.450989f, 238.498993f,
	179.332977f, 238.503998f, 179.214996f, 238.516998f, 179.096985f, 241.015991f, 155.680969f, 241.178986f,
	154.155975f, 242.464996f, 152.998962f, 243.998993f, 152.998962f, 249.000992f, 250.534988f, 152.998962f,
	251.820999f, 154.154968f, 251.983994f, 155.680969f, 254.482986f, 179.096985f, 254.495987f, 179.213989f,
	254.500992f, 179.331970f, 254.499985f, 179.450989f, 250.951004f, 482.035004f, 250.932007f, 483.678009f,
	249.593994f, 485.000000f, 247.951004f, 485.000000f, 247.951004f, 485.000000f
};

static const VGubyte pointer_2_circle_2_commands[] = {
	VG_MOVE_TO, VG_SCCWARC_TO_ABS, VG_LCCWARC_TO_ABS, VG_CLOSE_PATH
};

static const VGfloat pointer_2_circle_2_coordinates[] = {
	246.500000f, 217.001007f, 27.000000f, 27.000000f, 0.000000f, 273.500000f, 244.001007f, 27.000000f,
	27.000000f, 0.000000f, 246.500000f, 217.001007f
};



// 指针内部路径填充画笔对象定义
// 以下定义的数据源均来自".\image\原始设计数据\指针\pointer_2.svg"
// <linearGradient id="SVGID_1_" gradientUnits="userSpaceOnUse" x1="-1272.8271" y1="-2434.123" x2="-946.8271" y2="-2434.123" gradientTransform="matrix(0 1 -1 0 -2187.623 1279.8271)">
// 内部路径的线性梯度渐变(linearGradien)填充的起点(x1,y1))/终点坐标(x2, y2) (SVG画布坐标)
// 起点（x1，y1）到终点（x2，y2）的连线是线性渐变的径向。
// 线性渐变径向起点(x1，y1)之前的颜色使用最小offset的stop-color的纯色, 此处为 offset="0.1857" 对应的 stop-color:#E54322
// 线性渐变径向终点(x2，y2)之后的颜色使用最大offset的stop-color的纯色, 此处为 offset="0.9852" 对应的 stop-color:#3F0008"
// 线性渐变径向起点(x1，y1)与径向终点(x2，y2)之间的颜色使用线性差值, 此处起点纯色为#E54322, 终点纯色为#3F0008
// gradientUnits="userSpaceOnUse" x1="-1272.8271" y1="-2434.123" x2="-946.8271" y2="-2434.123"
// 使用gradientTransform转换后的起点(x1,y1)/终点(x2, y2)坐标, 此时起点(246.5,7.0) 终点(246.5, 333.0), 计算过程请参考"使用gradientTransform计算线性渐变填充的起点和终点坐标"
// 起点(246.5,7.0) 终点(246.5, 333.0)在指针的中轴线上, POINTER_2_CENTER_OF_ROTATION_X为246.5
static VGfloat  fill_linear_gradient_innerPath[] = {
//    x1             y1          x2         y2
	-1272.8271f, -2434.123f, -946.8271f, -2434.123f,	// 未转换前的坐标, UI设计师使用矢量绘图软件Adobe Illustrator设计指针时, Adobe Illustrator自动创建
};

// 指针内部路径的转换矩阵gradientTransform定义(SVG转换矩阵格式)
// gradientTransform="matrix(0 1 -1 0 -2187.623 1279.8271)"
// SVG矩阵为2行3列,存储按列组织. { sx, shy, shx, sy, tx, ty }
//  sx, shy, shx, sy, tx, ty的具体含义可参考OpenVG 1.1 SPEC中Affine Transformations
//  简单含义可参考下面的代码注释 // 仿射变换参数
static VGfloat matrix_linear_gradient_innerPath[] = {
//  sx   shy    shx   sy     tx            ty
	0,    1,    -1,   0,   -2187.623f,   1279.8271f		// 转换矩阵, UI设计师使用矢量绘图软件Adobe Illustrator设计指针时, Adobe Illustrator自动创建
};

// 指针内部路径的线性梯度渐变填充的关键点(6个)
// 可以修改offset及RGBA的值观察stop特性对指针颜色的影响
// stop-color:#E54322  
//   0xE5/0xFF = 0.898  0x43/0xff = 0.263, 0x22/0xff = 0.133
static VGfloat rampStops_innerPath[30] = {
//  offset    R        G       B     Alpha
	0.1857f, 0.898f, 0.263f, 0.133f, 1.0f,		//<stop  offset="0.1857" style="stop-color:#E54322"/>
	0.4307f, 0.898f, 0.012f, 0.129f, 1.0f,		//<stop  offset="0.4307" style="stop-color:#E50321"/>
	0.9125f, 0.898f, 0.122f, 0.122f, 1.0f,		//<stop  offset="0.9125" style="stop-color:#E51F1F"/>
	0.9253f, 0.369f, 0.024f, 0.037f, 1.0f,		//<stop  offset="0.9253" style="stop-color:#5E060C"/>
	0.9458f, 0.129f, 0.016f, 0.043f, 1.0f,		//<stop  offset="0.9458" style="stop-color:#21040B"/>
	0.9852f, 0.247f, 0.000f, 0.031f, 1.0f,		//<stop  offset="0.9852" style="stop-color:#3F0008"/>
};


// 指针外部路径填充画笔对象定义
// 以下定义的数据均来自".\image\原始设计数据\指针\pointer_2.svg", 以文字方式打开pointer_2.svg来查看
// 外部路径的线性梯度渐变(linearGradien)填充的起点(x1,y1))/终点坐标(x2, y2) (SVG画布坐标)
// 起点（x1，y1）到终点（x2，y2）的连线是线性渐变的径向。
// 渐变径向起点之前的为最小offset的stop-color的纯色, 此处为 offset="0.0040f" 对应的 stop-color:#8A8A8A
// 渐变径向终点之后的为最大offset的stop-color的纯色, 此处为 offset="0.9852"  对应的 stop-color:#8A8A8A
// <linearGradient id = "SVGID_2_" gradientUnits = "userSpaceOnUse" x1 = "246.5" y1 = "4" x2 = "246.5" y2 = "336">
// 外部路径的线性梯度渐变(linearGradien)填充的起点(x1,y1))/终点坐标(x2, y2) (SVG画布坐标)
// 起点(246.5, 4.0) 终点(246.5, 336.0)在指针的中轴线上, POINTER_2_CENTER_OF_ROTATION_X为246.5
static VGfloat  fill_linear_gradient_outerPath[] = {
	//    x1             y1          x2         y2
	    246.5f,        4.000000f,   246.5f,    336.0f
};

// 指针外部路径的线性梯度渐变填充的关键点(8个)
// 可以修改offset及RGBA的值观察stop特性对指针颜色的影响
static VGfloat rampStops_outerPath[40] = {
	0.0040f, 0.541f, 0.541f, 0.541f, 1.0f,		//<stop  offset="0.004" style="stop-color:#8A8A8A"/>
	0.1669f, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.1669" style="stop-color:#808080"/>
	0.4818f, 0.722f, 0.722f, 0.722f, 1.0f,		//<stop  offset="0.4818" style="stop-color:#B8B8B8"/>
	0.9031f, 0.800f, 0.800f, 0.800f, 1.0f,		//<stop  offset="0.9031" style="stop-color:#CCCCCC"/>
	0.9174f, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.9174" style="stop-color:#808080"/>
	0.9423f, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.9423" style="stop-color:#808080"/>
	0.9592f, 0.502f, 0.502f, 0.502f, 1.0f,		//<stop  offset="0.9592" style="stop-color:#808080"/>
	0.9852f, 0.541f, 0.541f, 0.541f, 1.0f,		//<stop  offset="0.9852" style="stop-color:#8A8A8A"/>
};

// 仿射变换参数
// sx, sy 缩放系数 
//   sx X轴缩放系数, 1.0不缩放
//   sy Y轴缩放系数, 1.0不缩放
// shx, shy 剪切系数, 理解为长方形到平行四边形的剪切变形系数
//   shx 长方形上下2条边X轴移位, 0 表示无剪切变形
//   shy 长方形左右2条边Y轴移位, 0 表示无剪切变形
// tx, ty 平移系数
//   tx X轴平移值, 0表示无平移
//   ty Y轴平移值, 0表示无平移
// 透视投影参数
// w0, w1, w2 透视投影参数, 可简单模拟3D变换
//   一般不使用, 固定为 0, 0, 1
// SVG矩阵格式为2行3列,存储按列组织. { sx, shy, shx, sy, tx, ty }
// OPENVG矩阵格式为3行3列,存储按列组织. { sx, shy, w0, shx, sy, w1, tx, ty, w2 }
// 将SVG格式的转换矩阵转换为OpenVG格式的转换矩阵
// 参考OpenVG 1.1 SPEC的"Affine Transformations"了解转换矩阵的知识
//static void svg_matrix_to_openvg_matrix(float svg_matrix[6], float openvg_matrix[9])
//{
//	openvg_matrix[0] = svg_matrix[0];
//	openvg_matrix[1] = svg_matrix[1];
//	openvg_matrix[2] = 0;
//	openvg_matrix[3] = svg_matrix[2];
//	openvg_matrix[4] = svg_matrix[3];
//	openvg_matrix[5] = 0;
//	openvg_matrix[6] = svg_matrix[4];
//	openvg_matrix[7] = svg_matrix[5];
//	openvg_matrix[8] = 1.0;
//}

static void genPointerPaths (void)
{
	// 创建指针的三个路径对象(外径,内径,圆盘)
	// 指针内径矢量路径对象
	innerPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	// 将路径的命令及坐标加入到路径对象
	vgAppendPathData(innerPath, sizeof(pointer_2_polygon_0_commands)/sizeof(VGubyte), pointer_2_polygon_0_commands, pointer_2_polygon_0_coordinates);
	// 指针外径矢量路径对象
	outerPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(outerPath, sizeof(pointer_2_path_1_commands)/sizeof(VGubyte), pointer_2_path_1_commands, pointer_2_path_1_coordinates);
	// 指针圆盘矢量路径对象
	circlePath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(circlePath, sizeof(pointer_2_circle_2_commands)/sizeof(VGubyte), pointer_2_circle_2_commands, pointer_2_circle_2_coordinates);
	// 创建一个纯色类型的画笔对象
	colorPaint = vgCreatePaint();
	// 设置画笔为纯色类型
	vgSetParameteri(colorPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	

	// 2.1 定义一个完整光晕的背景底板(带有alpha效果)
	// 定义一个光晕图像的正方形背景底板(带有alpha效果), 该正方形与扇形的包络区域完全匹配.
	if(maskImage == VG_INVALID_HANDLE)
	{
		// 光晕底图为ARGB8888格式的数据
		// 此处光晕底图的尺寸为496*496, 宽高相同
		maskImage = vgCreateImage( VG_sRGBA_8888, HALO_W, HALO_H, VG_IMAGE_QUALITY_BETTER );
		if (maskImage)
		{
			// OpenVG坐标系是自左向右, 自底向上, OpenVG画布的左下角为坐标系原点(0,0)
			// halo图的宽度为HALO_W, 高度为HALO_H.  HALO_W > HALO_H
			// 填充部分区域 (0, HALO_W - HALO_H) (HALO_W-1, HALO_W-1) , 注意不是完整区域, 光晕的设计底图是一个长方形区域(HALO_W * HALO_H)
			// 从最后一行开始(halo + HALO_W * (HALO_H - 1)), 自底向上填充 (-HALO_W * 4)
			vgImageSubData(maskImage, 
								 VG_IMAGE_DATA(halo) + VG_IMAGE_STRIDE(halo) * (VG_IMAGE_HEIGHT(halo) - 1),
								0 - VG_IMAGE_STRIDE(halo),
								VG_IMAGE_FORMAT(halo), 
								0, 
								0, 
								VG_IMAGE_WIDTH(halo), 
								VG_IMAGE_HEIGHT(halo)
							);
		}
	}
	
	
	// 创建一个画笔对象
	maskPaint = vgCreatePaint();
	// 图像填充类型
	vgSetParameteri (maskPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
	// 图像填充方式为平铺模式
	vgSetParameteri (maskPaint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_PAD);
}

// SVG坐标变换
// t 转换矩阵
// sx, sy 原始坐标(转换前)
// dx, dy 保存转换后的坐标
static void svgTransformPoint(float* dx, float* dy, const float* t, float sx, float sy)
{
	*dx = sx*t[0] + sy*t[2] + t[4];
	*dy = sx*t[1] + sy*t[3] + t[5];
}

static void svgTransformIdentity (float* t)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void svgTransformTranslate (float* t, float tx, float ty)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = tx; t[5] = ty;
}

static void svgTransformMultiply (float* t, const float* s)
{
	float t0 = t[0] * s[0] + t[1] * s[2];
	float t2 = t[2] * s[0] + t[3] * s[2];
	float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
	t[1] = t[0] * s[1] + t[1] * s[3];
	t[3] = t[2] * s[1] + t[3] * s[3];
	t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
	t[0] = t0;
	t[2] = t2;
	t[4] = t4;
}

//static void svgTransformScale(float* t, float sx, float sy)
//{
//	t[0] = sx; t[1] = 0.0f;
//	t[2] = 0.0f; t[3] = sy;
//	t[4] = 0.0f; t[5] = 0.0f;
//}

#define vgvPI				3.141592653589793238462643383279502f
static void svgTransformRotate (float* t, float angle)
{
	VGfloat a = angle * vgvPI / 180.0f;
	float cs = cosf(a);
	float sn = sinf(a);
	t[0] = cs; t[1] = sn;
	t[2] = -sn; t[3] = cs;
	t[4] = 0.0f; t[5] = 0.0f;
}

static void svgTransformPremultiply (float* t, const float* s)
{
	float s2[6];
	memcpy (s2, s, sizeof(float) * 6);
	svgTransformMultiply(s2, t);
	memcpy (t, s2, sizeof(float) * 6);
}

static int svgTranslate (float* xform, float x, float y)
{
	float t[6];
	if (isnan(x) || isnan(y))
		return -1;
	svgTransformTranslate (t, x, y);
	svgTransformPremultiply (xform, t);
	return 0;
}

static int svgRotate(float* xform, float angle)
{
	float t[6];
	if (isnan(angle))
		return -1;
	svgTransformRotate(t, angle);
	svgTransformPremultiply (xform, t);
	return 0;
}

//static int svgScale(float* xform, float x, float y)
//{
//	float t[6];
//	if (isnan(x) || isnan(y))
//		return -1;
//
//	svgTransformScale (t, x, y);
//	svgTransformPremultiply (xform, t);
//	return 0;
//}
//
//static void svgTransformSkewX (float* t, float a)
//{
//	t[0] = 1.0f; t[1] = 0.0f;
//	t[2] = tanf(a); t[3] = 1.0f;
//	t[4] = 0.0f; t[5] = 0.0f;
//}
//
//static void svgTransformSkewY (float* t, float a)
//{
//	t[0] = 1.0f; t[1] = tanf(a);
//	t[2] = 0.0f; t[3] = 1.0f;
//	t[4] = 0.0f; t[5] = 0.0f;
//}

//static int svgSkewX(float* xform, float angle)
//{
//	float t[6];
//	if (isnan(angle))
//		return -1;
//
//	svgTransformSkewX (t, angle);
//	svgTransformPremultiply (xform, t);
//	return 0;
//}
//
//static int svgSkewY(float* xform, float angle)
//{
//	float t[6];
//	if (isnan(angle))
//		return -1;
//
//	svgTransformSkewY (t, angle);
//	svgTransformPremultiply (xform, t);
//	return 0;
//}

// 使用矢量绘图软件Adobe Illustrator或者在线SVG编辑工具(https://c.runoob.com/more/svgeditor/)打开".\image\原始设计数据\指针\pointer_2.svg", 点击圆盘, 获取圆盘的中心点坐标(246.5, 245.0)及尺寸(54, 54)
#define	POINTER_2_CENTER_OF_ROTATION_X	(246.5f)
#define	POINTER_2_CENTER_OF_ROTATION_Y	(245.0f)

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#include <stdlib.h>
#include <math.h>
// 指针外包络矩形 (指针SVG坐标空间)
// 圆盘的中心点坐标 (246.5, 245.0)及尺寸(54, 54)
// 外径的中心点参数 (246.5, 170)及尺寸(16, 332)
static const float circle_region[] = { 246.5f, 245.0f, 54.0f, 54.0f };
static const float outer_region[] = { 246.5f, 170.0f, 16.0f, 332.0f };


// 计算指针的包络路径
static float sub_xmin;
static float sub_xmax;
static float sub_ymin;
static float sub_ymax;

// 计算指针的包络矩形区域
static void calcPointerBoundingRect(void)
{
	float xmin, ymin, xmax, ymax;
	// 根据指针的外形定义xmin, ymin, xmax, ymax
	// 本实例的指针由外径/内径/圆盘3部分构成, 此处使用外径及圆盘的位置信息构建xmin, ymin, xmax, ymax
	// 此处假设SVG设计文件中指针的径向沿着Y轴向上, 给出的坐标使用SVG坐标系(自顶向下).
	xmin = min(circle_region[0] - circle_region[2] / 2.0f, outer_region[0] - outer_region[2] / 2.0f);
	ymin = min(circle_region[1] - circle_region[3] / 2.0f, outer_region[1] - outer_region[3] / 2.0f);
	xmax = max(circle_region[0] + circle_region[2] / 2.0f, outer_region[0] + outer_region[2] / 2.0f);
	ymax = max(circle_region[1] + circle_region[3] / 2.0f, outer_region[1] + outer_region[3] / 2.0f);

	// 考虑可能的计算误差 (避免指针在水平角度/垂直角度上出现的残留)
	xmin -= 2.0f;
	xmax += 2.0f;
	ymin -= 2.0f;
	ymax += 2.0f;
	
	// 将SVG坐标系(自顶向下)转换为OpenVG坐标系(自底向上), 仅修改Y轴, X轴保持不变
	ymin = MAP_SVG_TO_OPENVG(ymin, pointer_2_cy);
	ymax = MAP_SVG_TO_OPENVG(ymax, pointer_2_cy);


	sub_xmin = xmin;
	sub_xmax = xmax;
	sub_ymin = ymin ;
	sub_ymax = ymax;
}

// 根据角度及旋转中心点位置计算转换矩阵, 并保存在xform中(SVG格式转换矩阵)
static void calc_magic_transform(float  angleExtent, float cx, float cy, float xform[6])
{
	float svg_x, svg_y;
	//float xform[6];
	//加载单位矩阵, 将坐标原点移至绘制面的原点(屏幕左下角)
	svgTransformIdentity(xform);
	// 将坐标原点平移至指针SVG的坐标原点
	// 根据2个中心点(指针圆盘中心点在SVG平面中的偏移, 指针圆盘中心点在OpenVG中的偏移)的差异计算指针SVG平面相对于OpenVG原点的偏移值
	svg_x = cx - POINTER_2_CENTER_OF_ROTATION_X;
	svg_y = cy - MAP_SVG_TO_OPENVG(POINTER_2_CENTER_OF_ROTATION_Y, pointer_2_cy);
	// 将OpenVG坐标原点平移至SVG平面的原点
	svgTranslate(xform, svg_x, svg_y);

	// 将OpenVG坐标原点继续平移至指针的旋转中心点(指针圆盘的中心点)
	svgTranslate(xform, POINTER_2_CENTER_OF_ROTATION_X, MAP_SVG_TO_OPENVG(POINTER_2_CENTER_OF_ROTATION_Y, pointer_2_cy));

	// 将坐标轴旋转至指定的角度
	svgRotate(xform, angleExtent);

	// 将坐标原点平移至指针SVG的坐标原点
	svgTranslate(xform, -(POINTER_2_CENTER_OF_ROTATION_X), -(MAP_SVG_TO_OPENVG(POINTER_2_CENTER_OF_ROTATION_Y, pointer_2_cy)));
}

// 调试使用(显示当前的指针脏矩形区域包络框)
//#define	SHOW_POINTER_DIRTY_RECTANGLE

#ifdef SHOW_POINTER_DIRTY_RECTANGLE
// 绘制矩形包络框, 用于调试
static void drawBoundingRect(int coords[4])
{
	VGfloat col[4];
	float xmin, xmax, ymin, ymax;
	xmin = coords[0];
	ymin = coords[1];
	xmax = coords[0] + coords[2] - 1;
	ymax = coords[1] + coords[3] - 1;

	VGPaint stroke;	// 描边画笔对象


	// 调试用途, 绘制剪切矩形
	stroke = vgCreatePaint();
	col[0] = 0;// 0xff / 255.0f;  // R
	col[1] = 0xff / 255.0f;  // G 
	col[2] = 0;// 0xff / 255.0f;  // B
	col[3] = 1.00f;  // Alpha
						 // 纯色类型的画笔
	vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, col);
	vgSetf(VG_STROKE_LINE_WIDTH, 1);
	vgSetPaint(stroke, VG_STROKE_PATH);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();


	// 绘制矩形
	VGubyte sub_bounding_commands[5] = { VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH };
	VGfloat sub_bounding_coordinates[8];
	sub_bounding_coordinates[0] = xmin;
	sub_bounding_coordinates[1] = ymin;
	sub_bounding_coordinates[2] = xmax;
	sub_bounding_coordinates[3] = ymin;
	sub_bounding_coordinates[4] = xmax;
	sub_bounding_coordinates[5] = ymax;
	sub_bounding_coordinates[6] = xmin;
	sub_bounding_coordinates[7] = ymax;

	VGPath boundingRectPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(boundingRectPath, sizeof(sub_bounding_commands) / sizeof(VGubyte), sub_bounding_commands, sub_bounding_coordinates);
	vgDrawPath(boundingRectPath, VG_STROKE_PATH);
	vgDestroyPath(boundingRectPath);

	vgDestroyPaint(stroke);

}
#endif

// 根据指针指向的角度及旋转中心点计算指针的最小脏矩形区域
static int scanSubBoundingRect(float  angleExtent, float cx, float cy, VGint *coords)
{
	float xform[6];

	// 根据角度及旋转中心点位置计算转换矩阵, 并保存在xform中(SVG格式转换矩阵),
	// 该矩阵描述如何将指针SVG空间的点坐标映射到OpenVG空间
	calc_magic_transform (angleExtent, cx, cy, xform);
	
	// 将SVG空间的指针包络矩形映射到OpenVG空间, 并求解在OpenVG空间中包含该指针包络矩形的最小矩形
	{
		// 将SVG空间的指针包络矩形(sub_xmin, sub_ymin, sub_xmax, sub_ymax)映射到OpenVG空间(rect_x, rect_y)
		float sx, sy;
		float dx, dy;
		float xmin, xmax, ymin, ymax;
		float rect_x[4], rect_y[4];
		sx = sub_xmin;	// 左下角
		sy = sub_ymin;
		// 坐标系变换(包含旋转/平移变换), 将(sx,sy)对应的SVG坐标值转换到(dx,dy)对应的OpenVG空间
		svgTransformPoint (&dx, &dy, xform, sx, sy);
		rect_x[0] = dx;
		rect_y[0] = dy;
		sx = sub_xmax;	// 右下角
		sy = sub_ymin;
		svgTransformPoint(&dx, &dy, xform, sx, sy);
		rect_x[1] = dx;
		rect_y[1] = dy;
		sx = sub_xmax;	// 右上角
		sy = sub_ymax;
		svgTransformPoint(&dx, &dy, xform, sx, sy);
		rect_x[2] = dx;
		rect_y[2] = dy;
		sx = sub_xmin;	// 右上角
		sy = sub_ymax;
		svgTransformPoint(&dx, &dy, xform, sx, sy);
		rect_x[3] = dx;
		rect_y[3] = dy;
		
		// 求解在OpenVG空间中包含该指针包络矩形的最小矩形
		// 求解包含rect的最小长方形
		xmin = 9999999.0f;
		xmax = -9999999.0f;
		ymin = 9999999.0f;
		ymax = -9999999.0f;
		for (int j = 0; j < 4; j++)
		{
			if (rect_x[j] > xmax)
				xmax = rect_x[j];
			if(rect_x[j] < xmin)
				xmin = rect_x[j];
			if (rect_y[j] > ymax)
				ymax = rect_y[j];
			if (rect_y[j] < ymin)
				ymin = rect_y[j];
		}

		// 考虑计算误差, 避免指针在水平角度/垂直角度上出现的残留
		// 此处位置/尺寸信息未做检查, 由VG自动裁减
		coords[0] = (VGint)(xmin - 0);
		coords[1] = (VGint)(ymin - 0);
		coords[2] = (VGint)(xmax + 1.0f) - (VGint)xmin + 1;
		coords[3] = (VGint)(ymax + 1.0f) - (VGint)ymin + 1;
	}

	return 1;
}


// 绘制指针
// 指针由3个路径对象组成, 内径/外径/表盘
// angleExtent 指针的旋转角度值(0 ~ 360)
// cx, cy为指针的旋转中心点坐标(OpenVG画布的坐标)
static void drawPointer (float  angleExtent, float cx, float cy)
{
	//angleExtent = 50;
	VGfloat col[4];
	VGPaint fill;	// 填充画笔对象
	VGPaint stroke;	// 描边画笔对象
	col[0] = 0xc4/255.0f; 
	col[1] = 0x05/255.0f; 
	col[2] = 0x05/255.0f; 
	col[3] = 1.00f;
	vgSetParameterfv(colorPaint, VG_PAINT_COLOR, 4, col);

	//angleExtent = 0;


	float x0, y0, x1, y1;
	// 坐标系转换 
	// (使用Adobe Illustrator给定的转换矩阵(matrix_linear_gradient_innerPath)将起点/终点坐标(fill_linear_gradient_innerPath)转换到SVG坐标系)
	// 使用gradientTransform计算线性渐变填充的起点和终点坐标(基于SVG画布坐标系, 自左向右, 自顶向下, 画布的左上角为坐标系原点)(同Windows的画图程序使用的坐标系)
	// 起点坐标转换
	svgTransformPoint(&x0, &y0, matrix_linear_gradient_innerPath, fill_linear_gradient_innerPath[0], fill_linear_gradient_innerPath[1]);
	// 终点坐标转换
	svgTransformPoint(&x1, &y1, matrix_linear_gradient_innerPath, fill_linear_gradient_innerPath[2], fill_linear_gradient_innerPath[3]);
	// printf("x0 = %f, y0 = %f, x1 = %f, y1 = %f\n", x0, y0, x1, y1);
	// 将SVG画布坐标转换到OpenVG坐标系
	y0 = MAP_SVG_TO_OPENVG(y0, pointer_2_cy);
	y1 = MAP_SVG_TO_OPENVG(y1, pointer_2_cy);
	float fill_linear_gradient[4] = { x0, y0, x1, y1 };
	// 创建用于指针内部路径填充使用的画笔对象fill
	fill = vgCreatePaint();
	// 设置线性梯度渐变填充的转换矩阵(单位矩阵), 因为开始/结束点坐标已转换为基于SVG画布原点的坐标值
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();
	// 设置画笔使用线性梯度渐变填充模式
	vgSetParameteri( fill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT );
	// 设置线性梯度渐变的开始/结束点坐标(已转换至OpenVG空间)
	vgSetParameterfv(fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient);
	//vgSetParameterfv( fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient_innerPath );
	// 设置画笔对象fill的线性梯度渐变填充的关键点(6个), 每个关键点包含偏移(0表示渐变线上的起始位置，1为终点位置),颜色, 透明度
	vgSetParameterfv( fill, VG_PAINT_COLOR_RAMP_STOPS, 30, rampStops_innerPath );
	vgSetParameteri( fill, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD );
	// 设置画笔对象fill用于后续的路径填充
	vgSetPaint(fill, VG_FILL_PATH);

	// 设置用户路径到绘制面的转换矩阵
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	// 加载单位矩阵, 将坐标原点移至绘制面的原点(屏幕左下角)
	vgLoadIdentity();
	
	// 将坐标原点平移至指针SVG的坐标原点
	// 根据2个中心点(指针圆盘中心点在SVG平面中的偏移, 指针圆盘中心点在OpenVG中的偏移)的差异计算指针SVG平面相对于OpenVG原点的偏移值
	float svg_x = cx - POINTER_2_CENTER_OF_ROTATION_X;
	float svg_y = cy - MAP_SVG_TO_OPENVG(POINTER_2_CENTER_OF_ROTATION_Y, pointer_2_cy);
	// 将OpenVG坐标原点平移至SVG平面的原点
	vgTranslate (svg_x, svg_y);
	
	// 将OpenVG坐标原点继续平移至指针的旋转中心点(指针圆盘的中心点)
	vgTranslate (POINTER_2_CENTER_OF_ROTATION_X, MAP_SVG_TO_OPENVG(POINTER_2_CENTER_OF_ROTATION_Y, pointer_2_cy));
	
	// 将坐标轴旋转至指定的角度
	vgRotate (angleExtent);
	//vgRotate(0);		// 测试
	//vgRotate(90);
	//vgRotate(180);
	
	// 将坐标原点平移至指针SVG的坐标原点
	vgTranslate(-(POINTER_2_CENTER_OF_ROTATION_X), -(MAP_SVG_TO_OPENVG(POINTER_2_CENTER_OF_ROTATION_Y, pointer_2_cy)));
	
	// 设置好以上平移, 平移, 旋转, 平移转换矩阵后, 指针就可以绕着固定的中心点旋转.
	// 请参阅OpenVG 1.1 SPEC了解详细的说明
	// 绘制内部红色部分
	vgDrawPath (innerPath, VG_FILL_PATH);
	// 此处将一个无效句柄置为填充用途的画笔, 标记画笔对象fill不再使用用于填充用途, 以便将画笔对象fill的资源释放.
	vgSetPaint (VG_INVALID_HANDLE, VG_FILL_PATH );
	// 删除画笔对象
	vgDestroyPaint (fill);
	
	// 设置非0填充规则
	vgSeti ( VG_FILL_RULE, VG_NON_ZERO );
	// 创建用于指针外部路径绘制填充的画笔对象
	fill = vgCreatePaint();
	// 该画笔用于路径填充
	vgSetPaint( fill, VG_FILL_PATH );
	// 设置该画笔对象的转换矩阵
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	//初始为为单位矩阵
	vgLoadIdentity();
	// 将SVG画布坐标转换到OpenVG坐标系
	y0 = MAP_SVG_TO_OPENVG(fill_linear_gradient_outerPath[1], pointer_2_cy);
	y1 = MAP_SVG_TO_OPENVG(fill_linear_gradient_outerPath[3], pointer_2_cy);
	// 定义一个数组保存转换后的OpenVG空间的线性梯度渐变的起点和终点坐标
	fill_linear_gradient[0] = fill_linear_gradient_outerPath[0];	// 起点
	fill_linear_gradient[1] = y0;
	fill_linear_gradient[2] = fill_linear_gradient_outerPath[2];	// 终点
	fill_linear_gradient[3] = y1;
	// 设置指针外部路径画笔对象的属性
	// 设置画笔使用线性梯度渐变填充模式
	vgSetParameteri( fill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT );
	//vgSetParameterfv( fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient_outerPath ); // 这个是SVG空间的坐标, 不能用于OpenVG空间.
	// 设置线性梯度渐变的开始/结束点坐标(已转换至OpenVG空间)
	vgSetParameterfv(fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient);

	// 设置画笔对象fill的线性梯度渐变填充的关键点(8个), 每个关键点包含偏移(0表示渐变线上的起始位置，1为终点位置),颜色, 透明度
	vgSetParameterfv( fill, VG_PAINT_COLOR_RAMP_STOPS, 40, rampStops_outerPath );
	vgSetParameteri( fill, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD );
	
	// 以填充模式绘制指针的外部灰色部分outerPath
	vgDrawPath (outerPath, VG_FILL_PATH);
	// 此处将一个无效句柄置为填充用途的画笔, 标记画笔对象fill不再使用用于填充用途, 以便将画笔对象fill的资源释放.
	vgSetPaint( VG_INVALID_HANDLE, VG_FILL_PATH );
	// 删除画笔对象fill
	vgDestroyPaint (fill);
	
	// 绘制指针上的圆盘
	// 以下信息来自".\image\原始设计数据\指针\pointer_2.svg"
	// <circle fill="#2B2A2E" stroke="#F84000" stroke-width="4" stroke-miterlimit="10" cx="246.5" cy="244.999" r="27"/>
	// 圆盘填充颜色 #2B2A2E, 圆盘描边颜色 #F84000, 描边线宽 4, 圆盘轮廓路径使用 pointer_2_circle_2_commands 及 pointer_2_circle_2_coordinates 描述
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	//	设置纯色填充用途的画笔 ( #2B2A2E )
	col[0] = 0x2b/255.0f;  // R
	col[1] = 0x2a/255.0f;  // G 
	col[2] = 0x2e/255.0f;  // B
	col[3] = 1.00f;  // Alpha
	// 纯色类型的画笔
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, col);
	// 设置该画笔用于后续的路径填充
	vgSetPaint (colorPaint, VG_FILL_PATH);
	// 创建一个用于描边用途的画笔
	stroke = vgCreatePaint();
	// 使用该画笔用于后续的描边操作, stroke="#F84000"
	vgSetPaint( stroke, VG_STROKE_PATH );
	// 设置纯色类型的画笔, 颜色 "#F84000"
	col[0] = 0xf8/255.0f; // R
	col[1] = 0x40/255.0f; // G
	col[2] = 0x00/255.0f; // B
	col[3] = 1.00f; // Alpha
	// 设置该描边画笔使用纯色
	vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, col);
	// 设置该描边画笔的线宽为4 ( stroke-width="4" )
	vgSetf (VG_STROKE_LINE_WIDTH, 4);
	// 斜接长度 stroke-miterlimit="10"
	vgSetf (VG_STROKE_MITER_LIMIT, 10);
	// 绘制圆盘(描边及填充)
	vgDrawPath (circlePath, VG_FILL_PATH|VG_STROKE_PATH);
	// 将当前描边画笔对象置为空, 释放当前已使用结束的描边画笔对象
	vgSetPaint( VG_INVALID_HANDLE, VG_STROKE_PATH );
	// 将当前填充画笔对象置为空, 释放当前已使用结束的填充画笔对象
	vgSetPaint( VG_INVALID_HANDLE, VG_FILL_PATH );
	// 恢复设置线宽为1
	vgSetf(VG_STROKE_LINE_WIDTH, 1);
	// 释放描边画笔的资源
	vgDestroyPaint (stroke);

}

#ifdef __cplusplus
extern "C" {
#endif

	// 参考".\字体\readme.txt"了解如何从TTF文件中提取指定字符集的字形数据文件
	extern Font arial_font;			// 运行".\字体\run.bat"创建该demo所需的字形C文件
									//		参考".\字体\font_utf8.txt"了解已创建的字形编码
	extern Font courbd_font;		// 运行".\字体\run_courbd.bat"创建该demo所需的courbd字形C文件

#define my_font	arial_font		// 使用arial_font字体

#ifdef __cplusplus
}
#endif


// font_w 字形的像素宽度
// font_h 字形的像素高度
// speed 显示的字符串文本
// centre_x centre_y 为效果图中对应的速度文本其显示区域的中心点坐标(相对于效果图左上角原点的偏移, SVG画布坐标)
// color为字符的颜色
static void DrawSpeed(char *speed, int font_w, int font_h, int centre_x, int centre_y, unsigned int color)
{
	VGfloat col[4];
	int str_size;
	VGfloat glyphOrigin[2] = { 0.0f, 0.0f };
	float speed_scale_x, speed_scale_y;

	// 将效果图中的坐标值(自顶向下)转换为VG区域中的坐标值(自底向上)
	int vg_offset_x = centre_x;
	int vg_offset_y = centre_y;
	vg_offset_y = VG_H - 1 - vg_offset_y;

	str_size = strlen(speed);
	(void)(str_size);

	// 计算未缩放前字符串的逻辑总宽度/逻辑高度值信息 (逻辑高度值总是1.0)
	vgTextSize(&my_font, speed, &speed_scale_x, &speed_scale_y);

	// 字符串像素宽度(按高度值为基准值计算得到的字符串宽度)
	float str_width = font_h * (float)speed_scale_x;
	// 字符串像素高度
	float str_height = font_h / speed_scale_y;	// speed_scale_y为1.0

	// 获取数字字符'0'的逻辑宽度/高度
	vgTextSize(&my_font, "0", &speed_scale_x, &speed_scale_y);

	// 计算字体实际宽度/高度设置值引入的缩放系数
	float scale_rato = (font_w / speed_scale_x) / (font_h / speed_scale_y);
	str_width *= scale_rato;

	// 设置车速显示时的文字颜色 
	col[0] = ((color >> 0) & 0xff) / 255.0f; 	// R
	col[1] = ((color >> 8) & 0xff) / 255.0f; 	// G
	col[2] = ((color >> 16) & 0xff) / 255.0f; 	// B
	col[3] = 1.00f;
	// 纯色画笔用于路径填充
	vgSetParameterfv(colorPaint, VG_PAINT_COLOR, 4, col);
	// 设置colorPaint为填充的画笔
	vgSetPaint(colorPaint, VG_FILL_PATH);

	vgSeti(VG_FILL_RULE, VG_NON_ZERO);
	vgSetfv(VG_GLYPH_ORIGIN, 2, glyphOrigin);
	// 设置文字绘制的坐标系
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
	vgLoadIdentity();
	// 定位显示的原点
	vgTranslate(vg_offset_x - str_width / 2, vg_offset_y - str_height / 2);
	// 计算字形的放大倍数
	// 计算数字字符'0'的逻辑宽度/高度信息及实际显示字符的像素宽度/像素高度信息计算宽度/高度的缩放系数
	vgScale(font_w / speed_scale_x, font_h / speed_scale_y);
	// arial_font为车速显示使用的字形数据
	vgTextOut(&my_font, speed, VG_FILL_PATH);

	// 标记用于填充用途的画笔不再使用
	vgSetPaint(VG_INVALID_HANDLE, VG_FILL_PATH);
}


#define	RGB(r,g,b)	((r) | ((g) << 8) | ((b) << 16))


static float startAngle = 212.0f;


#define VG_COLOR(r,g,b,a)   ((r) | ((g) << 8) | ((b) << 16) | ((a) << 24))
// 矩形填充
void vg_clear(
	int vg_x, int vg_y,				// VG写入区域的偏移 (自左向右, 自顶向下)
	int vg_w, int vg_h,				// VG写入区域的尺寸
	int vg_cx, int vg_cy,			// VG尺寸
	unsigned int color
	)
{
	VGfloat clear_color[4] = { (color & 0xFF)/255.0f,  ((color >> 8) & 0xFF) / 255.0f,  ((color >> 16) & 0xFF) / 255.0f, ((color >> 24) & 0xFF) / 255.0f };
	// 设置清除指令使用的颜色
	vgSetfv (VG_CLEAR_COLOR, 4, clear_color);
	// 转换坐标系并执行清除指令
	vgClear ((VGint)vg_x, (VGint)(vg_cy - (vg_y + vg_h)), (VGint)vg_w, (VGint)vg_h);
	// 等待指令执行完毕
	vgFinish();
}

static void draw_speed_level (int speed_level)
{
	char text[8];
	
	// 使用黑色(当前项目的背景色), 清除背景区域
	vg_clear (437, 463, 151, 70, VG_W, VG_H, VG_COLOR(0, 0, 0, 0xFF));
	
	// 格式化时速值
	sprintf (text, "%d", speed_level);
	// 绘制时速值
	DrawSpeed (text, 42, 42, 514, 498, RGB(255,255,255)); 
	
}

// 绘制表盘上的刻度值 (0, 30, ... 330, 360)
static void draw_scale_value (void)
{
#if 0
	// 字体大小
	int fw, fh;
	fw = 16;
	fh = 20;

	// 表盘刻度不再需要绘制. 已使用包含表盘刻度字符的背景图 (BGRGB565.RGB)

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
#endif
}


// 绘制主函数
static void do_dial_paint (float angleExtent, float speed)
{
	float old_vehicle_speed;
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb (&current_fb_no);

	// 保存当前的刻度值
	old_vehicle_speed = dial_vehicle_speeds[current_fb_no];
	dial_vehicle_speeds[current_fb_no] = speed;
	
	VGint coords[4 * 2];
	int dirty_count = 0;
	int total_dirty_count = 0;
	VGint *dirty_coords = coords;
	
	// 脏矩形列表包含2部分, 1) 最近指针对应的脏矩形(重绘上一个指针区域) 2) 当前指针对应的脏矩形区域 
	// 检查上一次的脏矩形是否存在. 若存在, 复制上一次的脏矩形列表
	if( dirty_rects_count[current_fb_no])
	{
		// 复制该FB(FrameBuffer)保存的上一次的脏矩形列表, 因为该区域需要被重绘(上一次的指针在此位置显示)
		memcpy (coords, dirty_rects_coords[current_fb_no], dirty_rects_count[current_fb_no] * 4 * sizeof(VGint));
		dirty_count = dirty_rects_count[current_fb_no];
		dirty_coords += dirty_count * 4;
		total_dirty_count = dirty_count;
	}
	
	// dirty_coords指向保存当前脏矩形区域的缓冲区
	// 计算当前位置/角度指针的脏矩形
	dirty_count = scanSubBoundingRect(startAngle + angleExtent - 90, PT_CENTRE_X, MAP_SVG_TO_OPENVG(PT_CENTRE_Y, VG_H), dirty_coords);
	total_dirty_count += dirty_count;
	// 更新当前FrameBuffer对应的脏矩形区域, 以便下次重绘该FrameBuffer时将该脏矩形区域包含并重绘
	memcpy (dirty_rects_coords[current_fb_no], dirty_coords, dirty_count * 4 * sizeof(VGint));
	dirty_rects_count[current_fb_no] = dirty_count;
	
	
	// 注意: 双指针仪表(DOUBLE_POINTER_HALO)demo中计算裁减区域的方法与单指针仪表(SINGLE_POINTER_HALO)存在差异.
	// 双指针仪表将表盘中心数字部分也纳入裁减区域. 请参考相应demo的代码
	
	if (full_dial_output_count > 0)
	{
		// 第一次/第二次输出, 全区域元素输出, 对应于FB的个数.
		// VG关闭裁剪区
		vgSeti(VG_SCISSORING, VG_FALSE);
		full_dial_output_count --;
	}
	else if(speed == old_vehicle_speed)
	{
		// 转速未改变时无需重绘
		return;
	}
	else
	{
		// 部分区域输出

		// 设置当前GPU渲染的裁剪区(输出仅在裁剪区中的被处理),优化GPU存取效率
		// 使能裁剪区
		vgSeti(VG_SCISSORING, VG_TRUE);
		// 写入裁减矩形
		vgSetiv(VG_SCISSOR_RECTS, total_dirty_count * 4, coords);
	}
	
#ifdef SHOW_POINTER_DIRTY_RECTANGLE
	vgSeti(VG_SCISSORING, VG_FALSE);
#endif
	
	
	// 1 绘制背景
	// 设置绘制图像与绘制表面为1:1映射模式(完全覆盖)
	
	VGfloat m[9];
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgLoadIdentity();	// 单位矩阵
	// 将自底向上转换为自顶向下, 因为vgDrawImageDirect使用的image是自顶向下
	vgGetMatrix(&m[0]);
	m[4] = -1;
	m[7] = VG_H;
	vgLoadMatrix(&m[0]);
	
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);		// 普通绘图模式
	// 绘制背景Image对象
	char *image = (char *)XM_RomAddress(ROM_IMAGE_BG_RGB16_RGB565);
	// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
	vgDrawImageDirect (VG_IMAGE_DATA(image), 	// 背景源图像基地址, 必须为64字节对齐
							 VG_IMAGE_WIDTH(image),
							 VG_IMAGE_HEIGHT(image),
							  VG_IMAGE_STRIDE(image),
							 // 源图像开窗设置(全画面输出)
							 0,
							 0,
							  VG_IMAGE_WIDTH(image),
							 VG_IMAGE_HEIGHT(image),
							 VG_IMAGE_QUALITY_BETTER,
							 VG_IMAGE_FORMAT(image)
							);

	vgLoadIdentity();	// 单位矩阵
	// 2 叠加光晕效果
	drawHalo (startAngle, angleExtent, PT_CENTRE_X, MAP_SVG_TO_OPENVG(PT_CENTRE_Y,VG_H) );

	//vgDestroyPaint (maskPaint);
	//vgDestroyImage (maskImage);
	
	// 3 绘制刻度盘上的刻度值
	draw_scale_value ();
	
	// 4 绘制指针
	// 参考OpenVG 1.1 SPEC中 vguArc的说明, 了解startAngle, angleExtent, 以及startAngle + angleExtent的含义
	// startAngle + angleExtent 为笛卡尔坐标系的角度, 与指针的角度(在指针设计时已指定Y轴向上为其0度基准)相差90度
	drawPointer (startAngle + angleExtent - 90, PT_CENTRE_X, MAP_SVG_TO_OPENVG(PT_CENTRE_Y, VG_H));
	
	// 此处缺省关闭SCISSORING功能, 后续由应用自己确定是否使用SCISSORING功能.
	vgSeti(VG_SCISSORING, VG_FALSE);

	// 5 绘制表盘底部的刻度
	draw_speed_level ((int)speed);

#ifdef SHOW_POINTER_DIRTY_RECTANGLE
	// 显示当前的指针脏矩形区域包络框
	drawBoundingRect (dirty_rects_coords[current_fb_no]);
#endif	
	
}

// 光晕起始角度定义, 用户根据表盘UI的实际起始位置来修改以下START_ANGLE, END_ANGLE
#define START_ANGLE	 	(-0.0f)					// 光晕起始位置对应的角度 (参考vguArc说明)
#define END_ANGLE  		(-244.0f)				// 光晕结束位置对应的角度 (参考vguArc说明)


// 将仪表时速的刻度映射为光晕扇区展开的角度
// 时速            0.0f  ~  360.0f
// angleExtent    START_ANGLE  ~ (END_ANGLE)
static float vehicle_speed_to_angle(float vehicle_speed)
{
	VGfloat angleExtent;
	// 用户根据表盘UI的实际刻度值范围修改刻度/展开角度转换公式
	angleExtent = START_ANGLE + (END_ANGLE - START_ANGLE) * (vehicle_speed - 0.0f) / (360.0f - 0.0f);
	return angleExtent;
}

// vehicle_speed 时速
static void draw_dial(float vehicle_speed)
{	
	// 将时速映射为展开角度
	float angleExtent = vehicle_speed_to_angle(vehicle_speed);
	

	// 表盘绘制		
	do_dial_paint (angleExtent, vehicle_speed);
	
	// 等待当前OpenVG的所有指令全部执行完毕, 避免指令未执行完毕时出现的画面异常
	//vgFinish();
}


int single_pointer_halo_init (int width, int height)
{
	if(xm_vg_get_bpp() != 16)
	{
		XM_printf ("Fatal Error, please define BPP16\n");
		return -1;
	}
	
	// 全屏输出(无裁减区域设置)的次数为2, 对应于LCD底层配置的双FrameBuffer显示机制(请参考LCD.C)
	full_dial_output_count = 2;
	
	// ARGB8888格式光晕底图
	// 
	// PNG格式设计图      
	//     ".\image\原始设计数据\背景\halo_496x496.png"
	// 运行".\image\原始设计数据\背景\run.bat", 将PNG格式的设计底图转换为ARGB8888格式
	//     ARGB8888格式BIN文件 ".\image\原始设计数据\背景\PNG2VG\ARGB8888\halo.ARGB8888"
	// 将该文件复制至 ".\rom\image\"用于ROM.BIN文件制作
	//     ARGB8888格式BIN文件 ".\rom\image\halo.ARGB8888"
	halo = (unsigned int *)XM_RomAddress(ROM_IMAGE_HALO_496X496_ARGB8888);
	
	// RGB565格式背景图
	//
	// PNG格式设计图
   //  1) 原始设计底图 32位PNG格式
   //     ".\image\原始设计数据\背景\bg_RGB32_包含刻度.png"
	//  2) 已转换到RGB16的PNG格式
	//      .\image\原始设计数据\背景\bg_RGB16.PNG"
	//      参考"\image\原始设计数据\背景\readme.txt"了解如何将图像颜色从RGB32转换到RGB16,且尽可能保留细节
	//
	// 运行".\image\原始设计数据\背景\run.bat", 将PNG格式的设计底图转换为RGB565格式
	//     RGB565格式BIN文件 ".\image\原始设计数据\背景\PNG2VG\RGB565\bg_RGB16.RGB565"
	// 将该文件复制至 ".\rom\image\"用于ROM.BIN文件制作
	//     RGB565格式BIN文件 ".\rom\image\bg_RGB16.RGB565"
	bkimg = (unsigned short *)XM_RomAddress(ROM_IMAGE_BG_RGB16_RGB565);

	// 字体资源初始化
	vgFontInit ();

	// 创建指针的路径对象及画笔对象
	genPointerPaths ();

	// 计算指针的包络矩形区域
	calcPointerBoundingRect();

	// 初始化ICON状态
	init_icons(); 
	return 0;
}

// 退出并使用资源
int single_pointer_halo_exit (void)
{
	//XM_printf ("pointer_halo_exit\n");
	// 释放光晕画笔对象
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
	
	// 释放指针的3个路径对象资源
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
	return 0;
}

static unsigned int start_ticket;

static void draw_icons(void);
static void draw_message(void);

//  贴图功能测试代码使能
#define ENABLE_BITBLT_TEST

int single_pointer_halo_draw (void)
{
	static float vehicle_speed = 0.0f;
	static int v_dir = 0;

	// 使用VG绘制中间的表盘区域
	draw_dial (vehicle_speed);
	
	//draw_dial(0);
	// 测试角度位置时使能 HALO_FILL_PURE_COLOR 使用红色填充扇形区域
	//draw_dial (0.0f);		// 测试 0 对应的位置,
	//draw_dial (360.0f);	// 测试 360 对应的位置
	
	// 关闭裁减功能
	vgSeti(VG_SCISSORING, VG_FALSE);
	
	// 绘制ICON
	draw_icons();

	// 显示文字信息
	draw_message();


#ifdef ENABLE_BITBLT_TEST
	// 贴图功能测试代码
	bitblt_test ();
#endif
	
	vgFinish();

	// 模拟
	if(v_dir == 0 && vehicle_speed < 360.0f)
	{
		// 顺时针
		vehicle_speed += 1.0f;
		if (vehicle_speed >= 360.0f)
		{
			vehicle_speed = 360.0f;
			v_dir = 1;
		}
	}
	else if (v_dir == 1 && vehicle_speed > 0.0f)
	{
		// 逆时针
		vehicle_speed -= 1.0f;
		if (vehicle_speed <= 0.0f)
		{
			vehicle_speed = 0.0f;
			v_dir = 0;
		}
	}

	icon_state_simulate();
	
	return 0;
}

enum {
	ICON_TYPE_ROTATE_SPEED = 0,		// 转速标尺
	ICON_TYPE_WATER_TEMP,			// 水温标尺
	ICON_TYPE_OIL_MASS,				// 油量标尺
	ICON_TYPE_LEFT,					// 左
	ICON_TYPE_RIGHT,					// 右
	ICON_TYPE_HIGH_BEAM,				// 远光灯
	ICON_TYPE_FOG_LAMPS,				// 前雾灯
	ICON_TYPE_DIPPED_HEADLIGHT,	// 近光灯
	ICON_TYPE_CORRIDOR_LAMP,		// 示廊灯
	ICON_TYPE_BRAKE_LIGHTS,			// 刹车灯
	ICON_TYPE_TRIP_A,					// TRIP A (对应 TRIP A:)
	ICON_TYPE_TRIP_A_NUMBER,		// 对应TRIP A的数字部分
	ICON_TYPE_TRIP_A_KM,				// TRIP A KM (对应 KM)
	ICON_TYPE_ICON_0,
	ICON_TYPE_ICON_1,
	ICON_TYPE_ICON_2,
	ICON_TYPE_ICON_3,
	ICON_TYPE_ICON_4,
	ICON_TYPE_ICON_5,
	ICON_TYPE_ICON_6,				// 水温图标
	ICON_TYPE_X1000R_MIN,			// 转速图标
	ICON_TYPE_OIL_CONSUMPTION,		// 油耗图标
	ICON_TYPE_KAIYANGDIANZI,		

	ICON_TYPE_COUNT
};


typedef struct _icon_image {
	unsigned int id;			// 资源的唯一标识
	char name[32];				// rom 文件名
	float x;						// 显示位置
	float y;

	unsigned int state[2];	// 保存与VG的2个FrameBuffer对应的ICON的最近状态值. 
							//			比较该值与ICON的最新状态值, 决定是否刷新该ICON对应的Surface区域并更新为ICON的最新状态值

} ICON_IMAGE;


static ICON_IMAGE icon_image[] = {
	// // 转速
	{
		ICON_TYPE_ROTATE_SPEED,
		ROM_IMAGE_ROTATE_SPEED_0_2X_ARGB8888,
		44, 262,
		-1, -1,
	},
	// 水温
	{
		ICON_TYPE_WATER_TEMP,
		ROM_IMAGE_WATER_TEMP_0_2X_ARGB8888,
		46, 120,
		-1,	-1, // -1表示尚未获得有效值
	},
	// 油量
	{
		ICON_TYPE_OIL_MASS,
		ROM_IMAGE_OIL_MASS_0_2X_ARGB8888,
		844, 117,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 左
	{
		ICON_TYPE_LEFT,
		ROM_IMAGE_LEFT_2X_ARGB8888,
		67, 11,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 右
	{
		ICON_TYPE_RIGHT,
		ROM_IMAGE_RIGHT_2X_ARGB8888,
		889, 11,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 远光灯
	{
		ICON_TYPE_HIGH_BEAM,
		ROM_IMAGE_HIGH_BEAM_2X_ARGB8888,
		768, 20,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 前雾灯
	{
		ICON_TYPE_FOG_LAMPS,
		ROM_IMAGE_FOG_LAMPS_2X_ARGB8888,
		616, 22,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 近光灯
	{
		ICON_TYPE_DIPPED_HEADLIGHT,
		ROM_IMAGE_DIPPED_HEADLIGHT_2X_ARGB8888,
		216, 20,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 示廊灯
	{
		ICON_TYPE_CORRIDOR_LAMP,
		ROM_IMAGE_CORRIDOR_LAMP_2X_ARGB8888,
		374, 22,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// 刹车灯
	{
		ICON_TYPE_BRAKE_LIGHTS,
		ROM_IMAGE_BRAKE_LIGHTS_2X_ARGB8888,
		495, 15,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// TRIP A
	{
		ICON_TYPE_TRIP_A,
		ROM_IMAGE_TRIP_A_2X_ARGB8888,
		33, 506,
		-1,  -1,	// -1表示尚未获得有效值
	},
	// TRIP A (NUMBER)
	{
		ICON_TYPE_TRIP_A_NUMBER,
		ROM_IMAGE_TRIP_A_2X_ARGB8888,
		120, 506,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// TRIP A (KM)
	{
		ICON_TYPE_TRIP_A_KM,
		ROM_IMAGE_TRIP_KM_2X_ARGB8888,
		219, 506,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 0
	{
		ICON_TYPE_ICON_0,
		ROM_IMAGE_ICON_2X_0_ARGB8888,
		266, 554,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 1
	{
		ICON_TYPE_ICON_1,
		ROM_IMAGE_ICON_2X_1_ARGB8888,
		345, 554,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 2
	{
		ICON_TYPE_ICON_2,
		ROM_IMAGE_ICON_2X_2_ARGB8888,
		441, 554,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 3
	{
		ICON_TYPE_ICON_3,
		ROM_IMAGE_ICON_2X_3_ARGB8888,
		517, 554,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 4
	{
		ICON_TYPE_ICON_4,
		ROM_IMAGE_ICON_2X_4_ARGB8888,
		575, 554,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 5
	{
		ICON_TYPE_ICON_5,
		ROM_IMAGE_ICON_2X_5_ARGB8888,
		646, 554,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	},
	// icon 6 , 
	{
		ICON_TYPE_ICON_6,
		ROM_IMAGE_ICON_2X_6_ARGB8888,
		130, 165,	// 水温图标
		-1,  -1,		// -1表示尚未获得有效值
	},
	// x1000r_min
	{
		ICON_TYPE_X1000R_MIN,
		ROM_IMAGE_X1000R_MIN_2X_ARGB8888,
		121, 365,	// TRIP A里程区的位置及大小
		-1,  -1,		// -1表示尚未获得有效值
	}		,
	// OIL_CONSUMPTION
	{
		ICON_TYPE_OIL_CONSUMPTION,
		ROM_IMAGE_OIL_CONSUMPTION_2X_ARGB8888,
		842, 258,
		-1, -1,		// -1表示尚未获得有效值
	},
	{
		ICON_TYPE_KAIYANGDIANZI,
		0,
		0, 0,		
		-1, -1,		// -1表示尚未获得有效值
	},
	

};

// 转速标尺图像偏移
static const char *RotateSpeedLevelImage[] = {
	ROM_IMAGE_ROTATE_SPEED_0_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_1_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_2_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_3_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_4_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_5_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_6_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_7_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_8_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_9_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_10_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_11_2X_ARGB8888,
	ROM_IMAGE_ROTATE_SPEED_12_2X_ARGB8888
};

// 油耗标尺图像偏移
static const char *OilLevelImage[] = {
	ROM_IMAGE_OIL_MASS_0_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_1_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_2_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_3_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_4_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_5_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_6_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_7_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_8_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_9_2X_ARGB8888,
	ROM_IMAGE_OIL_MASS_10_2X_ARGB8888
};

// 水温标尺图像偏移
static const char *WaterTempLevelImage[] = {
	ROM_IMAGE_WATER_TEMP_0_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_1_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_2_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_3_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_4_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_5_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_6_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_7_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_8_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_9_2X_ARGB8888,
	ROM_IMAGE_WATER_TEMP_10_2X_ARGB8888
};

#define RotateSpeedLevelCount	(sizeof(RotateSpeedLevelImage) / sizeof(RotateSpeedLevelImage[0]))
#define OilLevelCount	(sizeof(OilLevelImage) / sizeof(OilLevelImage[0]))
#define WaterTempLevelCount	(sizeof(WaterTempLevelImage) / sizeof(WaterTempLevelImage[0]))

// 保存ICON对应的最新状态值
static unsigned int icon_states[ICON_TYPE_COUNT];



// 设置icon的最新状态值
void set_icon_state(int icon, unsigned int state)
{
	if (icon < 0 || icon >= ICON_TYPE_COUNT)
		return;

	icon_states[icon] = state;
}

// 复位ICON的显示状态
void reset_icon_state(void)
{
	int i;
	for (i = 0; i < sizeof(icon_image) / sizeof(icon_image[0]); i++)
	{
		icon_image[i].state[0] = -1;		// 与FB0(FrameBuffer 0)对应的ICON状态值
		icon_image[i].state[1] = -1;		// 与FB1对应的ICON状态值
	}
}




// 绘制油耗/水温标尺 
static void draw_oil_level_icon(int icon_id)
{
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb(&current_fb_no);
	ICON_IMAGE *icon = icon_image + icon_id;
	// 检查ICON的最新状态值是否与FB相关的最近状态值相同
	unsigned int last_state = icon_states[icon_id];
	if (last_state != icon_image[icon_id].state[current_fb_no])
	{
		// 状态值变化, 需要重绘
		icon_image[icon_id].state[current_fb_no] = last_state;

		
		// 将原点移至图片的左上角
		vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
		vgLoadIdentity();
		VGfloat m[9];
		vgGetMatrix(&m[0]);
		m[4] = -1;
		m[7] = VG_H;
		vgLoadMatrix(&m[0]);
		vgTranslate(icon->x, icon->y);
		// 覆盖模式
		vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);

		// 绘制标尺状态 (此处使用的是背景色为黑色的图片,绘制时对应的图标被污染, 因此绘制后需要重绘标尺图标)
		// 改为背景色为透明色的图片, 可以不需要重绘时对应的图标
		//draw_icon_background (icon_id);
		if (icon_id == ICON_TYPE_ROTATE_SPEED)
		{
			char *image = (char *)XM_RomAddress(RotateSpeedLevelImage[last_state]);
			if(image == NULL)
				return;
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect (VG_IMAGE_DATA(image), 
												 VG_IMAGE_WIDTH(image),
												 VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_STRIDE(image),
												 // 源图像开窗设置
												 0,
												 0,
												 VG_IMAGE_WIDTH(image),
												 VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_QUALITY_BETTER,
												 VG_IMAGE_FORMAT(image)														 
												);
		}
		else if(icon_id == ICON_TYPE_WATER_TEMP)
		{
			char *image = (char *)XM_RomAddress(WaterTempLevelImage[last_state]);
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect (VG_IMAGE_DATA(image), 
												 VG_IMAGE_WIDTH(image),
												 VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_STRIDE(image),
												 // 源图像开窗设置
												 0,
												 0,
												 VG_IMAGE_WIDTH(image),
												 VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_QUALITY_BETTER,
												 VG_IMAGE_FORMAT(image)													 
												);			
		}
		else if (icon_id == ICON_TYPE_OIL_MASS)
		{
			char *image = (char *)XM_RomAddress(OilLevelImage[last_state]);
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect (VG_IMAGE_DATA(image), 
												 VG_IMAGE_WIDTH(image),
												 VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_STRIDE(image),
												 // 源图像开窗设置
												 0,
												 0,
												 VG_IMAGE_WIDTH(image),
												 VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_QUALITY_BETTER,
												 VG_IMAGE_FORMAT(image)													 
												);
		}


		
		// 强制转速图标重绘
		if (icon_id == ICON_TYPE_ROTATE_SPEED)
		{
			// 标记其状态值为-1, 强制其重绘
			icon_image[ICON_TYPE_X1000R_MIN].state[current_fb_no] = -1;
			draw_icon(ICON_TYPE_X1000R_MIN);
		}
		// 强制油耗图标重绘
		else if (icon_id == ICON_TYPE_OIL_MASS)
		{
			// 标记其状态值为-1, 强制其重绘
			icon_image[ICON_TYPE_OIL_CONSUMPTION].state[current_fb_no] = -1;
			draw_icon(ICON_TYPE_OIL_CONSUMPTION);
		}
		// 强制水温图标重绘
		else if (icon_id == ICON_TYPE_WATER_TEMP)
		{
			// 标记其状态值为-1, 强制其重绘
			icon_image[ICON_TYPE_ICON_6].state[current_fb_no] = -1;
			draw_icon(ICON_TYPE_ICON_6);
		}
		
		vgFlush();
	}
}



static void init_icons(void)
{
	int i;


	for (i = 0; i < ICON_TYPE_COUNT; i++)
	{
		icon_states[i] = 1;
	}

	icon_states[ICON_TYPE_TRIP_A_NUMBER] = 0;

	icon_states[ICON_TYPE_KAIYANGDIANZI] = 0;

	// 复位icon状态
	reset_icon_state();
}

static void draw_icon(int icon_id)
{
	int i;
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb(&current_fb_no);
	
	for (i = 0; i < sizeof(icon_image) / sizeof(icon_image[0]); i++)
	{
		if (icon_image[i].id == icon_id)
		{
			vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
			// 设置自顶向下坐标转换, 因为image格式为自顶向下组织
			// 加载单位矩阵
			vgLoadIdentity();
			// 垂直方向镜像
			VGfloat m[9];
			vgGetMatrix(&m[0]);
			m[4] = -1;
			m[7] = VG_H;
			vgLoadMatrix(&m[0]);
			// 平移至ICON显示原点(ICON左上角)
			vgTranslate(icon_image[i].x,  icon_image[i].y);
			vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);

			// 检查ICON的最新状态值是否与FB相关的最近状态值相同
			unsigned int last_state = icon_states[i];

			int fill_back_and_then_fill_fore = 0;	// 填充背景再绘制前景模式, 当icon被表盘刷新过程污染时

			if (icon_image[i].state[current_fb_no] == -1)
				fill_back_and_then_fill_fore = 1;

			if (last_state != icon_image[i].state[current_fb_no])
			{
				// ICON状态值存在变化
				// 更新与当前FB相关的ICON最近状态值
				icon_image[i].state[current_fb_no] = last_state;

				// 是否填充背景
				if (last_state == 0 || fill_back_and_then_fill_fore)	// 填充背景
				{
					draw_icon_background (i);
				}

				// 绘制前景
				if (last_state == 1)
				{
					// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
					char *image = (char *)XM_RomAddress(icon_image[i].name);
					if(image)
					{
						vgDrawImageDirect (VG_IMAGE_DATA(image), 						// 源图像基地址, 必须为64字节对齐
												 VG_IMAGE_WIDTH(image),		// 源图像像素宽度
												 VG_IMAGE_HEIGHT(image),		// 源图像像素高度
												 VG_IMAGE_STRIDE(image),		// 源图像行字节长度, 必须满足16像素对齐
												 // 源图像开窗设置
												 0,
												 0,
												 (VGint)VG_IMAGE_WIDTH(image),
												 (VGint)VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_QUALITY_BETTER,		// 图像渲染质量
												 // 源图像格式
												 VG_IMAGE_FORMAT(image) 
												 );
						
					}
				}
			}
			else
			{
				// ICON状态值未变化

			}

			return;
		}
	}
}

// ICON输出时未使用裁减区域加速GPU显示, 用户可根据ICON的状态变化创建相应的裁减区域来优化GPU访问
static void draw_icons(void)
{
	VGint coords[4];

	// 设置裁剪区域为整个VG区域
	coords[0] = 0;
	coords[1] = 0;
	coords[2] = VG_W;
	coords[3] = VG_H;
	vgSeti(VG_SCISSORING, VG_TRUE);
	vgSetiv(VG_SCISSOR_RECTS, 4, coords);

	// 检查ICON的最新状态, 并根据ICON状态是否变化来绘制ICON
	draw_icon(ICON_TYPE_LEFT);
	draw_icon(ICON_TYPE_RIGHT);

	draw_icon(ICON_TYPE_HIGH_BEAM);
	draw_icon(ICON_TYPE_FOG_LAMPS);
	draw_icon(ICON_TYPE_DIPPED_HEADLIGHT);
	draw_icon(ICON_TYPE_CORRIDOR_LAMP);
	draw_icon(ICON_TYPE_BRAKE_LIGHTS);

	// 绘制转速标尺
	draw_oil_level_icon(ICON_TYPE_ROTATE_SPEED);

	// 绘制水温标尺
	draw_oil_level_icon(ICON_TYPE_WATER_TEMP);
	// 水温标尺的绘制会污染水温图标
	//draw_icon(ICON_TYPE_OIL_MASS);

	// 绘制油耗标尺
	draw_oil_level_icon(ICON_TYPE_OIL_MASS);
	// 油耗标尺的绘制会污染油耗图标
	//draw_icon(ICON_TYPE_WATER_TEMP);

	// 绘制自检状态
	draw_icon(ICON_TYPE_ICON_0);
	draw_icon(ICON_TYPE_ICON_1);
	draw_icon(ICON_TYPE_ICON_2);
	draw_icon(ICON_TYPE_ICON_3);
	draw_icon(ICON_TYPE_ICON_4);
	draw_icon(ICON_TYPE_ICON_5);

	// TRAP A
}

// 重新绘制icon对应位置的背景图
static void draw_icon_background (int icon_id)
{
	int i;
	for (i = 0; i < sizeof(icon_image) / sizeof(icon_image[0]); i++)
	{
		if (icon_image[i].id == icon_id)
		{
			vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
			// 将OpenVG坐标系转换到Windows的画笔坐标系(自左向右 自顶向下, 与SVG相同)
			vgLoadIdentity();
			VGfloat m[9];
			vgGetMatrix(&m[0]);
			m[4] = -1;
			m[7] = VG_H;
			vgLoadMatrix(&m[0]);
			// 平移至ICON背景区左上角(该坐标为该ICON在LCD显示屏上的起始坐标)
			vgTranslate(icon_image[i].x, icon_image[i].y );
			vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
			char *image = (char *)XM_RomAddress(icon_image[i].name);
			if(image == NULL)
				return;
			// // 从背景图中挖出与ICON位置对应的区域重新填充背景, 背景图为RGB565格式
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect (VG_IMAGE_DATA(bkimg), 		// 背景源图像基地址, 必须为64字节对齐
												 VG_IMAGE_WIDTH(bkimg),		// 源图像像素宽度
												 VG_IMAGE_HEIGHT(bkimg),		// 源图像像素高度
												 VG_IMAGE_STRIDE(bkimg),		// 源图像行字节长度, 必须满足16像素对齐
												 (VGint)icon_image[i].x,		// 源图像开窗参数, 
												 (VGint)icon_image[i].y,
												 (VGint)VG_IMAGE_WIDTH(image),
												 (VGint) VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_QUALITY_BETTER,		// 图像渲染质量
												 VG_IMAGE_FORMAT(bkimg)		// 源图像格式
												);
			//vgFlush();
			return;
		}
	}
}

static void draw_message(void)
{
	char info[32];
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb(&current_fb_no);

	// TRAP A计数值
	if (icon_image[ICON_TYPE_TRIP_A_NUMBER].state[current_fb_no] != icon_states[ICON_TYPE_TRIP_A_NUMBER])
	{
		// 更新状态值
		icon_image[ICON_TYPE_TRIP_A_NUMBER].state[current_fb_no] = icon_states[ICON_TYPE_TRIP_A_NUMBER];
		
		// 重绘背景 (此处背景需要恢复)
		draw_icon_background (ICON_TYPE_TRIP_A_NUMBER);
		
		// 绘制文本信息, 
		// 注意, 为保证数字串绘制时始终右对齐位置不变, 空白字符(编码32)的宽度与数字字符(编码48, 49)的宽度相同, 参考arial.c
		//		{    32, { 0.556152f, 0.000000f },   0, NULL, 0, NULL, VG_NON_ZERO },
		//		{    48, { 0.556152f, 0.000000f },  24, arial_glyph48_commands,  84, arial_glyph48_coordinates, VG_NON_ZERO },
		//		{    49, { 0.556152f, 0.000000f },  11, arial_glyph49_commands,  28, arial_glyph49_coordinates, VG_NON_ZERO },
		//		{    50, { 0.556152f, 0.000000f },  24, arial_glyph50_commands,  82, arial_glyph50_coordinates, VG_NON_ZERO },
		sprintf(info, "TRIP A:%6d km", icon_states[ICON_TYPE_TRIP_A_NUMBER]);
		DrawSpeed(info, 12, 16, 135, 510, RGB(255, 255, 255));
	}
	
	// TRAP B计数值, 请自行参考TRAP A自己实现


	// 汉字串"开阳电子"显示, 演示如何显示UTF8字符串. 
	// 所有的字符串显示均为UTF8格式, 以'\0'结束
	if (icon_image[ICON_TYPE_KAIYANGDIANZI].state[current_fb_no] != icon_states[ICON_TYPE_KAIYANGDIANZI])
	{
		// 更新状态值
		icon_image[ICON_TYPE_KAIYANGDIANZI].state[current_fb_no] = icon_states[ICON_TYPE_KAIYANGDIANZI];
		
		// 此处仅绘制一次, 后续不再修改
		
		// 参考 ".\字体\font_utf8.txt", 使用run_ARIALUNI.bat从ARIALUNI.TTF提取中文字符, 然后将其替换arial.c中的版本
		// 参考 ".\字体\readme.txt"了解如何提取字形数据
		// 以下为中文字符 "开阳电子"的字形数据, 从ARIALUNI.TTF提取
		// 	{ 23376,{ 1.000000f, 0.000000f },  24, ARIALUNI_glyph23376_commands,  56, ARIALUNI_glyph23376_coordinates, VG_NON_ZERO },
		//  { 24320, { 1.000000f, 0.000000f }, 28, ARIALUNI_glyph24320_commands, 56, ARIALUNI_glyph24320_coordinates, VG_NON_ZERO },
		//  { 30005,{ 1.000000f, 0.000000f },  47, ARIALUNI_glyph30005_commands,  94, ARIALUNI_glyph30005_coordinates, VG_NON_ZERO },
		//  { 38451,{ 1.000000f, 0.000000f },  40, ARIALUNI_glyph38451_commands,  90, ARIALUNI_glyph38451_coordinates, VG_NON_ZERO }
		// 以下为"开阳电子"的UTF8字符串
		const char ver[] = { 0xe5, 0xbc, 0x80, 0xe9, 0x98, 0xb3, 0xe7, 0x94, 0xb5, 0xe5, 0xad, 0x90, 0x00 };
		DrawSpeed((char *)ver, 	// UTF8字符串, 必须以'\0'结束
					 20, 					// 字形的宽度
					 32, 					// 字形的高度
					 100, 				// 字符串显示区域中心点的屏幕X坐标
					 570, 				// 字符串显示区域中心点的屏幕Y坐标 (自顶向下)
					 RGB(255, 255, 255)	// 显示时使用的颜色, 白色
					);
	}
}

#define	ICON_TEST
static int refresh_count = 2;
static unsigned int start_ticket;
long unsigned int XM_GetTickCount(void);

static void icon_state_simulate(void)
{
	// 模拟ICON状态变化
	if (refresh_count > 0)
	{
		// 用于保证2帧FB初始为相同的ICON状态
		refresh_count--;
		start_ticket = XM_GetTickCount();
		return;
	}

#ifdef ICON_TEST
	// 模拟ICON测试

	unsigned int time = XM_GetTickCount() - start_ticket;

	icon_states[ICON_TYPE_LEFT] = (time / 3000) % 2;
	icon_states[ICON_TYPE_RIGHT] = (time / 3000) % 2;


	icon_states[ICON_TYPE_HIGH_BEAM] = 1 - ((time / 2500) % 2);
	icon_states[ICON_TYPE_FOG_LAMPS] = 1 - ((time / 3000) % 2);
	icon_states[ICON_TYPE_DIPPED_HEADLIGHT] = 1 - ((time / 4000) % 2);
	icon_states[ICON_TYPE_CORRIDOR_LAMP] = 1 - ((time / 4200) % 2);
	icon_states[ICON_TYPE_BRAKE_LIGHTS] = 1 - ((time / 7600) % 2);



	// 修改转速/油耗/水温
	icon_states[ICON_TYPE_ROTATE_SPEED] = (time / 2500) % RotateSpeedLevelCount;
	icon_states[ICON_TYPE_OIL_MASS] = (time / 2000) % OilLevelCount;	
	icon_states[ICON_TYPE_WATER_TEMP] = (time / 3000) % WaterTempLevelCount;

	icon_states[ICON_TYPE_ICON_0] = 1;
	icon_states[ICON_TYPE_ICON_1] = 1;
	icon_states[ICON_TYPE_ICON_2] = 1;
	icon_states[ICON_TYPE_ICON_3] = 1;
	icon_states[ICON_TYPE_ICON_4] = 1;
	icon_states[ICON_TYPE_ICON_5] = 1;

	icon_states[ICON_TYPE_TRIP_A_NUMBER] = time / 1000;

	icon_states[ICON_TYPE_KAIYANGDIANZI] = 1;
#endif	
	
	
}

// 图像填充
// 返回值
// -1 失败
// 0  成功
static int vg_bitblt_slow(
	// VG写入区域开窗定义(自左向右, 自顶向下)
	float vg_x, float vg_y,				// VG写入区域的偏移 
	float vg_w, float vg_h,				// VG写入区域的尺寸
	float vg_cx, float vg_cy,			// VG尺寸
	unsigned int src_image_format,		// 源图像格式 1 RGB565格式 0 ARGB8888格式
	unsigned char *src_image_buffer,				// 源图像数据基址
	unsigned int src_stride,			// 源图像行字节长度
	unsigned int src_w,					// 源图像宽度
	unsigned int src_h,					// 源图像高度
	// 源图像读取开窗定义(自左向右, 自顶向下)
	unsigned int src_x,					// 源图像读出区域的偏移 
	unsigned int src_y,
	unsigned int image_w,				// 源图像读出区域的尺寸
	unsigned int image_h
)
{
	VGImage image = src_image_format;
	float old_matrix[9];
	unsigned int old_mode;
	int y;
	float scale_x, scale_y;

	image = vgCreateImage(src_image_format, image_w, image_h, VG_IMAGE_QUALITY_BETTER);
	if (image == VG_INVALID_HANDLE)
		return -1;

	y = src_y + image_h - 1;
	if(src_image_format == VG_sARGB_8888)
		vgImageSubData(image, src_image_buffer + src_stride * y + src_x * 4, 0-src_stride, VG_sARGB_8888, 0, 0, image_w, image_h);
	else if(src_image_format == VG_sRGB_565)
		vgImageSubData(image, src_image_buffer + src_stride * y + src_x * 2, 0-src_stride, VG_sRGB_565,   0, 0, image_w, image_h);
	else if(src_image_format == VG_sBGRA_8888)
		vgImageSubData(image, src_image_buffer + src_stride * y + src_x * 4, 0-src_stride, VG_sBGRA_8888, 0, 0, image_w, image_h);

	vg_y = vg_cy - (vg_y + vg_h);
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
	old_mode = vgGeti(VG_MATRIX_MODE);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgGetMatrix(old_matrix);
	vgLoadIdentity();
	vgTranslate (vg_x, vg_y);
	scale_x = vg_w / image_w;
	scale_y = vg_h / image_h;
	vgScale(scale_x, scale_y);
	vgDrawImage (image);
	vgDestroyImage(image);
	vgFlush();

	vgLoadMatrix(old_matrix);
	// 恢复之前的matrix mode
	vgSeti(VG_MATRIX_MODE, old_mode);
	return 0;
}

static int vg_bitblt_fast(
	// VG写入区域开窗定义(自左向右, 自顶向下)
	float vg_x, float vg_y,				// VG写入区域的偏移 
	float vg_w, float vg_h,				// VG写入区域的尺寸
	float vg_cx, float vg_cy,			// VG尺寸
	unsigned int src_image_format,		// 源图像格式 (VG_sARGB_8888, VG_sBGRA_8888, VG_sRGB_565)
	unsigned char *src_image_buffer,				// 源图像数据基址
	unsigned int src_stride,			// 源图像行字节长度
	unsigned int src_w,					// 源图像宽度
	unsigned int src_h,					// 源图像高度
	// 源图像读取开窗定义(自左向右, 自顶向下)
	unsigned int src_x,					// 源图像读出区域的偏移 
	unsigned int src_y,
	unsigned int image_w,				// 源图像读出区域的尺寸
	unsigned int image_h
)
{
	VGImageFormat format = src_image_format;
	float old_matrix[9];
	unsigned int old_mode;
	float scale_x, scale_y;

	// 设置图像转换质量
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
	old_mode = vgGeti(VG_MATRIX_MODE);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	// 保存老的转换矩阵
	vgGetMatrix(old_matrix);
	
	// 将VG坐标系从自底向上转换为自顶向下, 因为图像源数据均为自顶向下排列
	vgLoadIdentity();
	VGfloat m[9];
	vgGetMatrix(&m[0]);
	m[4] = -1;
	m[7] = vg_cy;
	vgLoadMatrix(&m[0]);

	// 平移至显示坐标点
	vgTranslate (vg_x, vg_y);
	// 计算缩放系数
	scale_x = vg_w / image_w;
	scale_y = vg_h / image_h;
	vgScale(scale_x, scale_y);
	
	// 绘制
	vgDrawImageDirect (src_image_buffer,
							 src_w,
							 src_h,
							 src_stride,
							 // 源图像开窗
							 src_x,
							 src_y,
							 image_w,
							 image_h,
							 VG_IMAGE_QUALITY_BETTER,
							 format
							 );
	
	
	// 如果src_image_buffer马上被摧毁或者内容易变,那么此处该改为vgFinish等待VG完成后再继续执行, 或者在外面调用处等待VG完成.
	// 使用不必要的vgFinish会导致无效的等待时间
	// vgFinish ();
	vgFlush();

	// 恢复bitblt之前的转换矩阵
	vgLoadMatrix(old_matrix);
	// 恢复之前的matrix mode
	vgSeti(VG_MATRIX_MODE, old_mode);
	return 0;
}

// VG贴图函数 vg_bitblt
// 支持原窗口开窗
// 支持写入区开窗
// 支持缩放
// 支持Alpha混合
// 支持ARGB8888, RGB565, BGRA8888源数据格式, (由PNG2VG工具产生, PNG2VG工具自动产生的数据符合快速bitblt要求)
//
// 当源图像满足以下要求时, 可以实现快速bitblt操作, 并且所需内存也减少一倍
// 
// 1) 源图像基地址64字节对齐
// 2) 源图像stride满足16个像素对齐, 即32位需要64字节对齐,16位满足32字节对齐
// 
int vg_bitblt(
	// VG写入区域开窗定义(自左向右, 自顶向下)
	float vg_x, float vg_y,				// VG写入区域的偏移 
	float vg_w, float vg_h,				// VG写入区域的尺寸
	float vg_cx, float vg_cy,			// VG尺寸
	unsigned int src_image_format,		// 源图像格式 (VG_sARGB_8888, VG_sBGRA_8888, VG_sRGB_565)
	unsigned char *src_image_buffer,				// 源图像数据基址
	unsigned int src_stride,			// 源图像行字节长度
	unsigned int src_w,					// 源图像宽度
	unsigned int src_h,					// 源图像高度
	// 源图像读取开窗定义(自左向右, 自顶向下)
	unsigned int src_x,					// 源图像读出区域的偏移 
	unsigned int src_y,
	unsigned int image_w,				// 源图像读出区域的尺寸
	unsigned int image_h
)
{
	// 检查源图像stride是否16像素对齐
	int do_fast_bitblt = 1;
	unsigned int stride = (src_w + 15) & (~15);
	if(src_image_format == VG_sARGB_8888)	// ARGB8888格式
		stride *= 4;
	else if(src_image_format == VG_sRGB_565)	// RGB565
		stride *= 2;
	else if(src_image_format == VG_sBGRA_8888)	// BGRA8888格式
		stride *= 4;
	
	if(src_stride % stride)
		do_fast_bitblt = 0;
	// 检查源图像地址是否64字节对齐
	else if((unsigned int)src_image_buffer & (64-1))
		do_fast_bitblt = 0;
	
	if (src_x + image_w > src_w)
		image_w = src_w - src_x;
	if (src_y + image_h > src_h)
		image_h = src_h - src_y;
	if (vg_x + vg_w > vg_cx)
		vg_w = vg_cx - vg_x;
	if (vg_y + vg_h > vg_cy)
		vg_h = vg_cy - vg_y;
	
	if(do_fast_bitblt)
	{
		return vg_bitblt_fast (vg_x, vg_y,
									  vg_w, vg_h,
									  vg_cx, vg_cy,
									  src_image_format,
									  src_image_buffer,
									  src_stride,
									  src_w, src_h,
									  src_x, src_y,
									  image_w,
									  image_h
									);
	}
	else
	{
		return vg_bitblt_slow (vg_x, vg_y,
									  vg_w, vg_h,
									  vg_cx, vg_cy,
									  src_image_format,
									  src_image_buffer,
									  src_stride,
									  src_w, src_h,
									  src_x, src_y,
									  image_w,
									  image_h
									);
				  
	}
}

#ifdef ENABLE_BITBLT_TEST

void bitblt_test(void)
{
	// ARGB8888 bitblt
	char *IMAGE_FOG_LAMPS_2X = (char *)XM_RomAddress(ROM_IMAGE_FOG_LAMPS_2X_ARGB8888);
	// ARGB8888
	vg_bitblt(
		800, 540,		// VG写入区域的偏移
		33, 26,		// VG写入区域的尺寸
		VG_W, VG_H,	// VG尺寸
		VG_IMAGE_FORMAT(IMAGE_FOG_LAMPS_2X),  // 源图像VG格式
		VG_IMAGE_DATA(IMAGE_FOG_LAMPS_2X),	// 源图像数据基址
		VG_IMAGE_STRIDE(IMAGE_FOG_LAMPS_2X),		// 源图像行字节长度
		VG_IMAGE_WIDTH(IMAGE_FOG_LAMPS_2X), 	// 源图像宽度
		VG_IMAGE_HEIGHT(IMAGE_FOG_LAMPS_2X),		// 源图像高度
		0, 0,		// 源图像读出区域的偏移
		33, 26		// 源图像读出区域的尺寸
	);
	

	vg_bitblt(
		840, 540,		// VG写入区域的偏移
		33, 26,		// VG写入区域的尺寸
		VG_W, VG_H,	// VG尺寸
		VG_IMAGE_FORMAT(IMAGE_FOG_LAMPS_2X),  // 源图像VG格式
		VG_IMAGE_DATA(IMAGE_FOG_LAMPS_2X),	// 源图像数据基址
		VG_IMAGE_STRIDE(IMAGE_FOG_LAMPS_2X),		// 源图像行字节长度
		VG_IMAGE_WIDTH(IMAGE_FOG_LAMPS_2X), 	// 源图像宽度
		VG_IMAGE_HEIGHT(IMAGE_FOG_LAMPS_2X),		// 源图像高度
		12, 0,		// 源图像读出区域的偏移
		33-12, 26		// 源图像读出区域的尺寸
	);
	
	vg_bitblt(
		900, 540,		// VG写入区域的偏移
		66, 52,		// VG写入区域的尺寸
		VG_W, VG_H,	// VG尺寸
		VG_IMAGE_FORMAT(IMAGE_FOG_LAMPS_2X),  // 源图像VG格式
		VG_IMAGE_DATA(IMAGE_FOG_LAMPS_2X),	// 源图像数据基址
		VG_IMAGE_STRIDE(IMAGE_FOG_LAMPS_2X),		// 源图像行字节长度
		VG_IMAGE_WIDTH(IMAGE_FOG_LAMPS_2X), 	// 源图像宽度
		VG_IMAGE_HEIGHT(IMAGE_FOG_LAMPS_2X),		// 源图像高度
		0, 0,		// 源图像读出区域的偏移
		33, 26		// 源图像读出区域的尺寸
	);
	
	
	// RGB565 bitblt
	//char *bkg = (char *)bkimg;
	vg_bitblt(
		720, 540,		// VG写入区域的偏移
		64, 64,		// VG写入区域的尺寸
		VG_W, VG_H,	// VG尺寸
		VG_IMAGE_FORMAT(IMAGE_FOG_LAMPS_2X),  // 源图像VG格式
		VG_IMAGE_DATA(IMAGE_FOG_LAMPS_2X),	// 源图像数据基址
		VG_IMAGE_STRIDE(IMAGE_FOG_LAMPS_2X),		// 源图像行字节长度
		VG_IMAGE_WIDTH(IMAGE_FOG_LAMPS_2X), 	// 源图像宽度
		VG_IMAGE_HEIGHT(IMAGE_FOG_LAMPS_2X),		// 源图像高度
		480, 60,		// 源图像读出区域的偏移
		64, 64		// 源图像读出区域的尺寸
	);


}
#endif

#endif

