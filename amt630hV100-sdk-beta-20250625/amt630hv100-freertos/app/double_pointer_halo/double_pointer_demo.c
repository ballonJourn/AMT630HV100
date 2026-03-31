//#include <windows.h>
#include <vg/openvg.h>
#include <vg/vgu.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "double_pointer_demo.h"
#include "vg_font.h"
#include "vg_image.h"

#ifdef DOUBLE_POINTER_HALO

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT   480

#define	VG_W			1280
#define	VG_H			480
#define	VG_OSD_X		0
#define	VG_OSD_Y		0
#define	OSD_W			1280
#define	OSD_H			480


#define	HALO_W	350
#define	HALO_H	299

#define CIRCLE_R		(214)
#define	CIRCLE_0_X		(30+214)
#define CIRCLE_0_Y		(26+214)

#define	CIRCLE_1_X		(1280 - (30+214))
#define CIRCLE_1_Y		(26+214)

#define PT_OFF_Y	132

#define ROM_IMAGE_HALO_ARGB8888				"halo.ARGB8888"
#define ROM_IMAGE_BGRGB565_RGB565			"BGRGB565.RGB565"
#define ROM_IMAGE_OIL_LEVEL_1_ARGB8888		"oil_level_1.ARGB8888"
#define ROM_IMAGE_OIL_LEVEL_2_ARGB8888		"oil_level_2.ARGB8888"
#define ROM_IMAGE_OIL_LEVEL_3_ARGB8888		"oil_level_3.ARGB8888"
#define ROM_IMAGE_OIL_LEVEL_4_ARGB8888		"oil_level_4.ARGB8888"
#define ROM_IMAGE_OIL_LOW_ARGB8888			"oil_low.ARGB8888"
#define ROM_IMAGE_OIL_HIGH_ARGB8888			"oil_high.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_LOW_ARGB8888	"water_temp_low.ARGB8888"
#define ROM_IMAGE_WATER_TEMP_HIGH_ARGB8888	"water_temp_high.ARGB8888"
#define	ROM_IMAGE_CAR_ARGB8888				"car.ARGB8888"
#define ROM_IMAGE_LEFT_FRONT_ARGB8888		"Left_front.ARGB8888"
#define ROM_IMAGE_LEFT_REAR_ARGB8888		"Left_rear.ARGB8888"
#define ROM_IMAGE_RIGHT_FRONT_ARGB8888		"Right_front.ARGB8888"
#define ROM_IMAGE_RIGHT_REAR_ARGB8888		"Right_rear.ARGB8888"
#define ROM_IMAGE_LEFT_1_ARGB8888			"left_1.ARGB8888"
#define ROM_IMAGE_RIGHT_1_ARGB8888			"right_1.ARGB8888"
#define ROM_IMAGE_ABS_ARGB8888				"ABS.ARGB8888"
#define ROM_IMAGE_ENGINE_ARGB8888			"engine.ARGB8888"
#define ROM_IMAGE_ENGINE_OIL_ARGB8888		"engine_oil.ARGB8888"
#define ROM_IMAGE_LOWER_BEAM_ARGB8888		"lower_beam.ARGB8888"
#define ROM_IMAGE_STOP_LAMP_ARGB8888		"stop_lamp.ARGB8888"
#define ROM_IMAGE_WIDTH_LAMP_ARGB8888		"width_lamp.ARGB8888"
#define ROM_IMAGE_DOUBLE_FLASHING_ARGB8888	"double_flashing.ARGB8888"
#define ROM_IMAGE_FOGLIGHT_ARGB8888			"foglight.ARGB8888"
#define ROM_IMAGE_HIGH_BEAM_ARGB8888		"high_beam.ARGB8888"


// 将SVG画布坐标转换为OpenVG坐标(仅转换Y轴坐标), 从自顶向下转换为自底向上
// cy是基于SVG坐标(X轴自左向右, Y轴自顶向下, PC画笔也是)的某个画布的高度, y是该画布下的Y轴坐标
#define	MAP_SVG_TO_OPENVG(y,cy)	(cy - 1.0f - (y))


static	VGImage maskImage;
static	VGPaint maskPaint;


// 全画面输出次数 
static int full_dial_output_count = 2;  // 2个framebuffer


// 光晕底图aRGB8888
static unsigned char *halo;

static int process_dial_level_rectangle(int dial, VGint* coords, int cb_coords);
static void init_icons (void);
static void draw_icons (void);
static void draw_message (void);
static void icon_state_simulate (void);
extern unsigned int xm_vg_get_osd_fb (int *no);
extern void* XM_RomAddress (const char *src);
extern unsigned int XM_GetTickCount (void);


// 扫描所有被表盘绘制污染的ICON并标记
static void scan_and_mark_icons_polluted_by_dial_paint (VGint* coords, int cb_coords);

// 用于计算2个表盘的脏矩形区域
//   1) 对于每个FrameBuffer, 根据每个表盘的前一刻度值与当前刻度值, 计算刻度值差异形成的重新绘制区域(由一个或多个脏矩形构成). 
//   2) 根据重新绘制区域设置Scissoring Rectangles, 约束VG的Surface裁减区域, 大幅减少VG的计算与带宽
static float dial_vehicle_speeds[2];	// 记录对应于2个FrameBuffer的表盘0的时速值
static VGint vehicle_speeds_dirty_rects_coords[2][4];	// 保存与指针相关的脏矩形
static int vehicle_speeds_dirty_rects_count[2];		// 记录脏矩形的个数, 固定为1

static float dial_rotate_speeds[2];	// 记录对应于2个FrameBuffer的表盘1的转速值
static VGint rotate_speeds_dirty_rects_coords[2][4];	// 保存与指针相关的脏矩形
static int rotate_speeds_dirty_rects_count[2];		// 记录脏矩形的个数, 固定为1


//#define	BG_RGB565	// 背景图使用RGB565格式,节省内存及DRAM带宽

// 仪表背景图
static unsigned short *bkimg;

static VGPaint colorPaint;
unsigned int* get_halo_image(void)
{
	return (unsigned int *) halo;
}


unsigned short* get_bk_image(void)
{
	return (unsigned short *)bkimg;
}

//#define HALO_FILL_PURE_COLOR		// 测试HALO区域的位置

// 2.2 第一个对应于指针扇区(从指针起始指向角度(0读数)延伸到当前指针读数)的pie行path
//     该path使用完整光晕的背景底板进行绘制,即得到仅包含有效区域的光晕
// 定义起始角度 (笛卡尔坐标系)

// pointer_x, floater_y 指针圆盘的中心点坐标(屏幕坐标)
void drawHalo(float startAngle, float  angleExtent, float pointer_x, float floater_y)
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
	vgSetPaint( maskPaint, VG_FILL_PATH );	// PATH填充使用

	// 光晕图像与扇形path使用相同的空间尺寸(HALO_W, HALO_W)
	// 定义转换矩阵, 将光晕底图(矩形包络)每个像素的颜色一一映射到扇形的整个包络矩形区域的每个点
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();	// 单位矩阵
	// 将光晕填充图像(HALO_W*HALO_W)的中心点映射到扇形包络区域(HALO_W*HALO_W)的中心点
	vgTranslate (-HALO_W / 2, -HALO_W / 2);

#endif
	

	// 使用画笔(光晕图)填充一个指定角度的扇形区域, 该扇形的中心点与指针圆的中心吻合, 半径与指针圆的半径相同
	// 定位到扇形path的中心点(相对于屏幕坐标)
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgTranslate (pointer_x, floater_y);

	// 设置非0路径填充模式
	vgSeti ( VG_FILL_RULE, VG_NON_ZERO );
	// 创建一个扇区路径
	path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	
	// 创建一个长宽相同的扇形矢量路径(参考vguArc文档说明)
	vguArc( path, 0.0f, 0.0f, HALO_W, HALO_W, startAngle, angleExtent, VGU_ARC_PIE  );
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

// ***************************************************************************************************
//
// ************** 使用以下宏定义可以选择不同的指针外形,指针填充方式,指针填充颜色 *************************
//
// ***************************************************************************************************
//
// 指针外形选择
// 以下2个指针的外观尺寸(外框)一致
//#define	POINTER_SHAPE_1		// 类三角形指针
#define	POINTER_SHAPE_2			// 类圆柱形指针


// 指针填充方式选择("使用纯色填充"刷新效率优于"使用线性梯度填充"
#define	POINTER_FILL_TYPE_LINEAR_GRADIENT		// 使用线性梯度填充
//#define	POINTER_FILL_TYPE_COLOR					// 使用纯色填充

#ifdef POINTER_SHAPE_1
#undef POINTER_FILL_TYPE_LINEAR_GRADIENT
#define	POINTER_FILL_TYPE_COLOR					// 使用纯色填充
#endif

// 指针纯色填充时颜色设置
#define 	FILL_COLOR(r,g,b,a)		((r<<24)|(g<<16)|(b<<8)|(a<<0))
// fill="#FFFFFF"		// 白色
#define	POINTER_FILL_COLOR		FILL_COLOR(0xFF,0xFF,0xFF,0xFF)
// fill="#F44336"		// 红色
//#define	POINTER_FILL_COLOR		FILL_COLOR(0xF4,0x43,0x36,0xFF)
// fill="#00D2FF"		// 蓝色
//#define	POINTER_FILL_COLOR		FILL_COLOR(0x00,0xD2,0xFF,0xFF)



// ************************************* 表盘指针 ***************************
//
// 1) 表盘的指针共用一个SVG设计, 参考".\image\原始设计数据\指针\指针.svg", 使用UltraEdit可以打开并编辑SVG文件. 或者使用矢量绘图工具AI打开.
// 2) 使用SVG2OPENVG.exe可以将SVG格式的指针数据提取并生成如下初始的格式
//     2.1 将SVG2OPENVG.exe与指针.svg拷贝到相同的目录下, 如 "e:\proj\HMI\SW\OPENVG_DEMO\double_pointer\bin"
//     2.2 在命令行模式下, 执行 "e:\>cd \proj\HMI\SW\OPENVG_DEMO\double_pointer\bin"
//     2.3 在命令行模式下, 执行"SVG2OPENVG.exe 指针.svg"
//     2.4 输出如下矢量路径的文本信息
//     size: 8.000000 x 76.000000
//     static const VGubyte 指针_path_0_commands[] = {
//         VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
//     };
//     2.5 或直接执行".\image\原始设计数据\指针\run.bat"
//
//    static const VGfloat 指针_path_0_coordinates[] = {
//         1.603790f, 2.002140f, 7.496920f, 75.243500f, 0.000000f, 75.030098f, 1.603790f, 2.002140f
//     };
// 3) 将其粘贴到代码中并修改. 加入表盘中心点的偏移(CIRCLE_0_X, CIRCLE_0_Y), 加入指针Y方向的偏移(PT_OFF_Y), 修改后代码如下

#ifdef POINTER_SHAPE_1
// 三角形指针
// size: 8.000000 x 76.000000
// 指针0的数据定义, 
// svg file (pointer.svg)
// svg image's size: 8.000000 x 76.000000
#define pointer_cx 8.000000f
#define pointer_cy 76.000000f
static const VGubyte pointer_path_0_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};
// PT_OFF_Y表示将环形指针顶点向上移动的距离, 将其环形指针的顶部延伸到指针圆盘的外圆上.
// 可以将该值改为其他值测试其效果
static const VGfloat pointer_path_0_coordinates[] = {
	1.603790f , 2.002140f  ,
	7.496920f , 75.243500f,
	0.000000f , 75.030098f,
	1.603790f , 2.002140f 
};

// 指针1的数据定义
static const VGubyte pointer_path_1_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};
static const VGfloat pointer_path_1_coordinates[] = {
	1.603790f , 2.002140f  ,
	7.496920f , 75.243500f ,
	0.000000f , 75.030098f ,
	1.603790f , 2.002140f 
};

// 指针旋转中心在SVG坐标系中的位置(将pointer.svg在Adobe Illustrator中打开, 测量其底部旋转中心线的2个端点坐标, 得到中心点如下坐标)
// 指针旋转中心在SVG坐标系中的位置
// (0, 75.03), (7.5, 75.24)
#define	POINTER_CENTER_OF_ROTATION_X	(0 + 7.5f) / 2.0f
#define	POINTER_CENTER_OF_ROTATION_Y	(75.03f + 75.24f) / 2.0f


#elif defined(POINTER_SHAPE_2)

// 类圆柱形指针
// svg file (pointer01_2.svg)
// svg image's size: 8.000000 x 74.000000
#define pointer01_2_cx 8.000000f
#define pointer01_2_cy 74.000000f

static const VGubyte pointer_path_0_commands[] = {
	VG_MOVE_TO, VG_CUBIC_TO, VG_VLINE_TO, VG_CUBIC_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};


static const VGfloat pointer_path_0_coordinates[] = {
	1.707420f, 70.863373f, 1.735730f, 72.051544f, 2.707060f, 73.000000f, 3.895560f, 73.000000f, 
	73.000000f, 5.081660f, 73.000000f, 6.051940f, 72.055252f, 6.083560f, 70.869568f, 8.000000f, 
	-1.000000f, 0.000000f, -0.790100f, 1.707420f, 70.863373f
};

static const VGubyte pointer_path_1_commands[] = {
	VG_MOVE_TO, VG_CUBIC_TO, VG_VLINE_TO, VG_CUBIC_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat pointer_path_1_coordinates[] = {
	1.707420f, 70.863373f, 1.735730f, 72.051544f, 2.707060f, 73.000000f, 3.895560f, 73.000000f, 
	73.000000f, 5.081660f, 73.000000f, 6.051940f, 72.055252f, 6.083560f, 70.869568f, 8.000000f, 
	-1.000000f, 0.000000f, -0.790100f, 1.707420f, 70.863373f
};

#define pointer_cx pointer01_2_cx
#define pointer_cy pointer01_2_cy

// 指针旋转中心在SVG坐标系中的位置(将pointer01_2.svg在Adobe Illustrator中打开, 测量其底部旋转中心线的2个端点坐标, 得到中心点如下坐标)
// 底部旋转中心线的2个端点坐标
// (0.0f, 73.79f), (8.0f, 74.0f)
// 中心点坐标为2个端点取平均
#define	POINTER_CENTER_OF_ROTATION_X	(0 + 8.0f) / 2.0f
#define	POINTER_CENTER_OF_ROTATION_Y	(73.79f + 74.0f) / 2.0f

// 以下数据来源于pointer01_2.svg, 使用文本方式打开该文件. <linearGradient>在SVG中描述一个线性梯度渐变的定义
//<linearGradient id="paint0_linear_1987:6021" x1="4" y1="0" x2="4" y2="74" gradientUnits="userSpaceOnUse">
//<stop stop-color="#FF001E"/>
//<stop offset="1" stop-color="#FFF06F"/>
//</linearGradient>
// 指针路径的线性梯度渐变(linearGradien)填充的起点(x1,y1))/终点坐标(x2, y2) (SVG画布坐标)
// 起点（x1，y1）到终点（x2，y2）的连线是线性渐变的径向。
static VGfloat  fill_linear_gradient_pointer[] = {
//    x1             y1          x2         y2
	   4,             0,         4,          74,	
};

// 指针路径的线性梯度渐变填充的关键点(2个)
// 可以修改offset及RGBA的值观察stop特性对指针颜色的影响
static VGfloat rampStops_pointer[10] = {
	0.0f,  0xff / 255.0f,  0x00 / 255.0f, 0x1e / 255.0f, 1.0f,		// <stop stop-color="#FF001E"/>
	1.0f,  0xff / 255.0f,  0xf0 / 255.0f, 0x6f / 255.0f, 1.0f,		// <stop offset="1" stop-color="#FFF06F"/>
};

#endif





// ************************************* 表盘指针 ***************************

// 环形指针矢量路径对象
static VGPath pointerPath_0;		// 时速指针对象
static VGPath pointerPath_1;		// 转速指针对象

static void genPointerPaths (void)
{
	// 创建2个环形指针的矢量路径对象
	pointerPath_0 = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(pointerPath_0, sizeof(pointer_path_0_commands)/sizeof(pointer_path_0_commands[0]), pointer_path_0_commands, pointer_path_0_coordinates);
	pointerPath_1 = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vgAppendPathData(pointerPath_1, sizeof(pointer_path_1_commands)/sizeof(pointer_path_1_commands[0]), pointer_path_1_commands, pointer_path_1_coordinates);

	// 定义一个光晕图像的正方形背景底板(带有alpha效果), 该正方形与扇形的包络区域完全匹配.
	maskImage = vgCreateImage( VG_sRGBA_8888, HALO_W, HALO_W, VG_IMAGE_QUALITY_BETTER );
	// halo图的宽度为HALO_W, 高度为HALO_H.  HALO_W > HALO_H
	// 填充部分区域 (0, HALO_W - HALO_H) (HALO_W-1, HALO_W-1) , 注意不是完整区域, 光晕的设计底图是一个长方形区域(HALO_W * HALO_H)
	// 从最后一行开始(halo + HALO_W * (HALO_H - 1)), 自底向上填充 (-HALO_W * 4)
	
	vgImageSubData (maskImage, 
						 VG_IMAGE_DATA(halo) + VG_IMAGE_STRIDE(halo) * (VG_IMAGE_HEIGHT(halo) - 1), 
						 0 - VG_IMAGE_STRIDE(halo),
						 VG_IMAGE_FORMAT(halo), 
						 0, VG_IMAGE_WIDTH(halo) - VG_IMAGE_HEIGHT(halo),  VG_IMAGE_WIDTH(halo), VG_IMAGE_HEIGHT(halo));

	
	// 创建一个图形填充画笔对象
	maskPaint = vgCreatePaint();
	// 设置画笔为图形填充模式
	vgSetParameteri (maskPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
	vgSetParameteri (maskPaint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_PAD);
	// 将画笔置为图案填充
	vgPaintPattern (maskPaint, maskImage);

	// 创建一个纯色画笔对象
	colorPaint = vgCreatePaint();
	vgSetParameteri(colorPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);

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
//
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

// 指针外包络矩形 (指针SVG坐标空间)
// <svg width="8" height="74" viewBox="0 0 8 74"
static const float pointer_region[] = { 0.0f, 0.0f, 8.0f, 74.0f };

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
	xmin = pointer_region[0];
	ymin = pointer_region[1];
	xmax = pointer_region[2];
	ymax = pointer_region[3];


	// 将SVG坐标系(自顶向下)转换为OpenVG坐标系(自底向上), 仅修改Y轴, X轴保持不变
	ymin = MAP_SVG_TO_OPENVG(ymin, pointer_cy);
	ymax = MAP_SVG_TO_OPENVG(ymax, pointer_cy);


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
	svg_x = cx - POINTER_CENTER_OF_ROTATION_X;
	svg_y = cy - MAP_SVG_TO_OPENVG(POINTER_CENTER_OF_ROTATION_Y, pointer_cy);
	// 将OpenVG坐标原点平移至SVG平面的原点
	svgTranslate(xform, svg_x, svg_y);

	// 将OpenVG坐标原点继续平移至指针的旋转中心点(指针圆盘的中心点)
	svgTranslate(xform, POINTER_CENTER_OF_ROTATION_X, MAP_SVG_TO_OPENVG(POINTER_CENTER_OF_ROTATION_Y, pointer_cy));

	// 将坐标轴旋转至指定的角度
	svgRotate(xform, angleExtent);
	// 将指针向径向(Y轴)延伸(指针需要在外环中旋转, 该外环内圈至圆环中心点的距离表示为PT_OFF_Y, 移至该位置)
	svgTranslate (xform, 0, PT_OFF_Y);
}

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
		
		// 考虑计算误差, 避免角度水平/垂直时计算误差
		coords[0] = (VGint)(xmin - 1);	
		coords[1] = (VGint)(ymin - 1);
		coords[2] = (VGint)(xmax + 1.0f) - (VGint)xmin + 1;
		coords[3] = (VGint)(ymax + 1.0f) - (VGint)ymin + 1;
	}

	return 1;
}

//static void drawBoundingRect(int coords[4])
//{
//	VGfloat col[4];
//	float xmin, xmax, ymin, ymax;
//	xmin = coords[0];
//	ymin = coords[1];
//	xmax = coords[0] + coords[2] - 1;
//	ymax = coords[1] + coords[3] - 1;
//
//	VGPaint stroke;	// 描边画笔对象
//
//
//	// 调试用途, 绘制剪切矩形
//	stroke = vgCreatePaint();
//	col[0] = 0;// 0xff / 255.0f;  // R
//	col[1] = 0xff / 255.0f;  // G 
//	col[2] = 0;// 0xff / 255.0f;  // B
//	col[3] = 1.00f;  // Alpha
//						 // 纯色类型的画笔
//	vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, col);
//	vgSetf(VG_STROKE_LINE_WIDTH, 1);
//	vgSetPaint(stroke, VG_STROKE_PATH);
//	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
//	vgLoadIdentity();
//
//
//	// 绘制矩形
//	VGubyte sub_bounding_commands[5] = { VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH };
//	VGfloat sub_bounding_coordinates[8];
//	sub_bounding_coordinates[0] = xmin;
//	sub_bounding_coordinates[1] = ymin;
//	sub_bounding_coordinates[2] = xmax;
//	sub_bounding_coordinates[3] = ymin;
//	sub_bounding_coordinates[4] = xmax;
//	sub_bounding_coordinates[5] = ymax;
//	sub_bounding_coordinates[6] = xmin;
//	sub_bounding_coordinates[7] = ymax;
//
//	VGPath boundingRectPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
//	vgAppendPathData(boundingRectPath, sizeof(sub_bounding_commands) / sizeof(VGubyte), sub_bounding_commands, sub_bounding_coordinates);
//	vgDrawPath(boundingRectPath, VG_STROKE_PATH);
//	vgDestroyPath(boundingRectPath);
//
//	vgDestroyPaint(stroke);
//
//}




// 光晕起始角度定义
#define START_ANGLE	 	(212.0f + 13.0f + 0.1f)					// 光晕起始位置对应的角度 (参考vguArc说明)
#define END_ANGLE  		(-(244.0f + 13.0f*2.0f - 0.1f))		// 光晕结束位置对应的角度 (参考vguArc说明)


// 将仪表时速的刻度映射为光晕扇区展开的角度
// 时速            0.0f  ~  240.0f
// angleExtent     0.0f  ~ (END_ANGLE)
static float vehicle_speed_to_angle(float vehicle_speed)
{
	VGfloat angleExtent;
	angleExtent = 0.0f + (END_ANGLE - 0.0f) * (vehicle_speed - 0.0f) / (240.0f - 0.0f);
	return angleExtent;
}

// 将仪表转速的刻度映射为光晕扇区展开的角度
// 转速            0.0f  ~  8.0f
// angleExtent     0.0f  ~ (END_ANGLE)
static float rotate_speed_to_angle(float rotate_speed)
{
	VGfloat angleExtent;
	angleExtent = 0.0f + (END_ANGLE - 0.0f) * (rotate_speed - 0.0f) / (8.0f - 0.0f);
	return angleExtent;
}


// 指针 135度 ~ -135度
// angleExtent 与 rotate_angle对应关系
// angleExtent     0.0f   ~ (END_ANGLE)
// rotate_angle    135.0f ~ (-135.0f)
// cx, cy为圆形表盘的中心位置坐标


static void drawPointer (VGPath pointerPath, float  angleExtent, float cx, float cy)
{
	VGfloat rotate_angle;
#ifdef POINTER_FILL_TYPE_COLOR
	VGfloat col[4];
#endif
	VGPaint fill;	// 填充画笔对象
	
	//vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	//vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	
	// 将扇区张开的角度转换为指针旋转的角度
	rotate_angle = 135.0f + ((-135.0f) - 135.0f) * (angleExtent - 0.0f) / (END_ANGLE  - 0.0f);
	
#ifdef POINTER_FILL_TYPE_LINEAR_GRADIENT
	// 指针使用线性填充方案
	float x0, y0, x1, y1;
	// fill_linear_gradient 为SVG画布坐标系的坐标值
	x0 = fill_linear_gradient_pointer[0];
	y0 = fill_linear_gradient_pointer[1];
	x1 = fill_linear_gradient_pointer[2];
	y1 = fill_linear_gradient_pointer[3];
	// 将SVG画布坐标转换到OpenVG坐标系(自顶向下表示)
	y0 = MAP_SVG_TO_OPENVG(y0, pointer_cy);
	y1 = MAP_SVG_TO_OPENVG(y1, pointer_cy);
	float fill_linear_gradient[4] = { x0, y0, x1, y1 };
	// 创建用于指针路径填充使用的画笔对象fill
	fill = vgCreatePaint();
	// 设置线性梯度渐变填充的转换矩阵(单位矩阵), 因为开始/结束点坐标已转换为基于SVG画布原点的坐标值
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
	vgLoadIdentity();
	// 设置画笔使用线性梯度渐变填充模式
	vgSetParameteri( fill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT );
	// 设置线性梯度渐变的开始/结束点坐标(已转换至OpenVG空间)
	vgSetParameterfv(fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient);	
	//vgSetParameterfv(fill, VG_PAINT_LINEAR_GRADIENT, 4, fill_linear_gradient_pointer);
	// 设置画笔对象fill的线性梯度渐变填充的关键点(2个), 每个关键点包含偏移(0表示渐变线上的起始位置，1为终点位置),颜色, 透明度
	vgSetParameterfv( fill, VG_PAINT_COLOR_RAMP_STOPS, 10, rampStops_pointer );
	vgSetParameteri( fill, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD );
	// 设置画笔对象fill用于后续的路径填充
	vgSetPaint(fill, VG_FILL_PATH);
#elif defined(POINTER_FILL_TYPE_COLOR)
	// 指针使用纯色填充方案
	// 使用白色绘制指针
	// 设置画笔颜色
	col[0] = ((POINTER_FILL_COLOR >> 24) & 0xFF) / 255.0f;
	col[1] = ((POINTER_FILL_COLOR >> 16) & 0xFF) / 255.0f;
	col[2] = ((POINTER_FILL_COLOR >> 8) & 0xFF) / 255.0f;
	col[3] = ((POINTER_FILL_COLOR >> 0) & 0xFF) / 255.0f;
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, col);
	// 标记画笔对象colorPaint用于下面的路径填充与描边操作
	vgSetPaint (colorPaint, VG_FILL_PATH);
#endif
	
	// 
	vgSeti (VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	// 将坐标原点平移至指针SVG的坐标原点
	// 根据2个中心点(指针圆盘中心点在SVG平面中的偏移, 指针圆盘中心点在OpenVG中的偏移)的差异计算指针SVG平面相对于OpenVG原点的偏移值
	float svg_x = cx - POINTER_CENTER_OF_ROTATION_X;
	float svg_y = cy - MAP_SVG_TO_OPENVG(POINTER_CENTER_OF_ROTATION_Y, pointer_cy);
	// 将OpenVG坐标原点平移至SVG平面的原点
	vgTranslate (svg_x, svg_y);
	// 将OpenVG坐标原点继续平移至指针的旋转中心点(指针圆盘的中心点)
	vgTranslate (POINTER_CENTER_OF_ROTATION_X, MAP_SVG_TO_OPENVG(POINTER_CENTER_OF_ROTATION_Y, pointer_cy));
	// 无缩放
	vgScale(1.0f, 1.0f);
	// 旋转指针
	vgRotate (rotate_angle);
	// 将指针向径向(Y轴)延伸(指针需要在外环中旋转, 该外环内圈至圆环中心点的距离表示为PT_OFF_Y, 移至该位置)
	vgTranslate (0, PT_OFF_Y);

	// 绘制指针(填充模式)
	vgDrawPath (pointerPath, VG_FILL_PATH);
	// 此处将一个无效句柄置为填充用途, 标记画笔对象colorPaint不再使用用于填充用途, 以便将画笔对象colorPaint的资源释放. 
	vgSetPaint (VG_INVALID_HANDLE, VG_FILL_PATH);

#ifdef POINTER_FILL_TYPE_LINEAR_GRADIENT
	// 删除画笔对象
	vgDestroyPaint (fill);
#endif
}

#include "vg_font.h"
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
// centre_x centre_y 为效果图中对应的速度文本其显示区域的中心点坐标(相对于效果图左上角原点的偏移)
// color为字符的颜色
static void DrawSpeed(char *speed, int font_w, int font_h, int centre_x, int centre_y, unsigned int color)
{
	VGfloat col[4];
	int str_size;
	VGfloat glyphOrigin[2] = { 0.0f, 0.0f };
	float speed_scale_x, speed_scale_y;

	// 将效果图中的坐标值转换为VG区域中的坐标值
	int vg_offset_x = centre_x;
	int vg_offset_y = centre_y;
	vg_offset_y = VG_H - 1 - vg_offset_y;

	str_size = strlen(speed);
	(void)(str_size);

	// 计算未缩放前字符串的逻辑总宽度/逻辑高度值信息 (逻辑高度值总是1.0)
	vgTextSize (&my_font, speed, &speed_scale_x, &speed_scale_y);

	// 字符串像素宽度(按高度值为基准值计算得到的字符串宽度)
	float str_width = font_h * (float)speed_scale_x;
	// 字符串像素高度
	float str_height = font_h / speed_scale_y;	// speed_scale_y为1.0

	// 获取数字字符'0'的逻辑宽度/高度
	vgTextSize (&my_font, "0", &speed_scale_x, &speed_scale_y);
	
	// 计算字体实际宽度/高度设置值引入的缩放系数
	float scale_rato = (font_w / speed_scale_x) / (font_h / speed_scale_y);
	str_width *= scale_rato;	

	// 设置车速显示时的文字颜色 (白色)
	col[0] = ((color >> 0) & 0xff) / 255.0f; 	// R
	col[1] = ((color >> 8) & 0xff) / 255.0f; 	// G
	col[2] = ((color >> 16) & 0xff) / 255.0f; 	// B
	col[3] = 1.00f;
	// 纯色画笔用于路径填充
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, col);
	vgSetPaint (colorPaint, VG_FILL_PATH );

	vgSeti (VG_FILL_RULE, VG_NON_ZERO);
	vgSetfv (VG_GLYPH_ORIGIN, 2, glyphOrigin);
	vgSeti (VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
	vgLoadIdentity();
	// 定位显示的原点
	vgTranslate (vg_offset_x - str_width / 2, vg_offset_y - str_height / 2);
	// 计算字形的放大倍数
	// 计算数字字符'0'的逻辑宽度/高度信息及实际显示字符的像素宽度/像素高度信息计算宽度/高度的缩放系数
	vgScale(font_w / speed_scale_x, font_h / speed_scale_y);
	// arial_font为车速显示使用的字形数据
	vgTextOut(&my_font, speed, VG_FILL_PATH);

	vgSetPaint(VG_INVALID_HANDLE, VG_FILL_PATH);
}

// 计算表盘0的时速/表盘1的转速脏矩形区域
// coords 脏矩形缓冲区
// cb_coords 脏矩形缓冲区个数
static int process_dial_level_rectangle (int dial, VGint* coords, int cb_coords)
{
	if(cb_coords <= 0)
		return 0;
	
	if (dial == 0)
	{
		// (143, 167) (203*116)
		coords[0] = 143;
		coords[1] = VG_H - (167 + 116);
		coords[2] = 203;	// W
		coords[3] = 116;	// H
	}
	else
	{
		// (944, 166) (181*113)
		coords[0] = 944;
		coords[1] = VG_H - (166 + 113);
		coords[2] = 181;	// W
		coords[3] = 113;	// H

	}
	return 1;
}


#define	RGB(r,g,b)	((r) | ((g) << 8) | ((b) << 16))

// 绘制仪表盘的速度值
static void draw_dial_speed(float vehicle_speed, float rotate_speed)
{
	char text[8];

	sprintf (text, "%d", (int)vehicle_speed);
	// 时速
	DrawSpeed (text, 64, 96, CIRCLE_0_X, CIRCLE_0_Y - 16, RGB(255, 255, 255));
	sprintf (text, "%2.1f", rotate_speed);
	// 转速
	DrawSpeed (text, 64, 96, CIRCLE_1_X, CIRCLE_1_Y - 16, RGB(255, 255, 255));

}


#define	MAX_DIRTY_RECTS	16		// 产生的最大脏矩形个数

// 表盘绘制 
// vehicle_speed 时速刻度值
// rotate_speed  转速刻度值

// 20211030 NEW_DIRTY_METHOD比较老的脏矩形算法,每帧刷新时间减少2ms左右
#define	NEW_DIRTY_METHOD

#ifndef NEW_DIRTY_METHOD
static int process_dial_dirty_rectangles (int dial_no, float old_th, float new_th,  VGint* coords, int cb_coords);
#endif

void do_dials_paint (float vehicle_speed, float rotate_speed)
{
	VGint coords[MAX_DIRTY_RECTS*4];
	int dirty_rects = 0;
#ifdef NEW_DIRTY_METHOD
	int total_dirty_count = 0;
	VGint *dirty_coords = coords;
	int dirty_count;
	float rotate_angle;
#endif

	float angleExtent_0 = vehicle_speed_to_angle(vehicle_speed);
	float angleExtent_1 = rotate_speed_to_angle(rotate_speed);
	float startAngle = START_ANGLE;
	float old_vehicle_speed, old_rotate_speed;

	//vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	//vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	
	
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb (&current_fb_no);
	
	memset (coords, 0, sizeof(coords));
	
	// 保存当前的刻度值
	old_vehicle_speed = dial_vehicle_speeds[current_fb_no];
	old_rotate_speed = dial_rotate_speeds[current_fb_no];
	
	dial_vehicle_speeds[current_fb_no] = vehicle_speed;
	dial_rotate_speeds[current_fb_no] = rotate_speed;
	
	
	
#ifdef NEW_DIRTY_METHOD
	total_dirty_count = 0;
	dirty_coords = coords;
	
	// 脏矩形列表包含2部分, 1) 最近指针对应的脏矩形(重绘上一个指针区域) 2) 当前指针对应的脏矩形区域 
	
	// 计算指针0(vehicle_speeds)的脏矩形区域
	//int last_fb = (current_fb_no + 1) & 1;
	// 复制上一次的脏矩形列表
	if( vehicle_speeds_dirty_rects_count[current_fb_no])
	{
		// 复制该FB(FrameBuffer)保存的上一次的脏矩形列表, 因为该区域需要被重绘(上一次的指针在此位置显示)
		dirty_count = vehicle_speeds_dirty_rects_count[current_fb_no];
		memcpy (dirty_coords, vehicle_speeds_dirty_rects_coords[current_fb_no], dirty_count * 4 * sizeof(VGint));
		dirty_coords += dirty_count * 4;
		total_dirty_count += dirty_count;
	}
	// 计算新位置的脏矩形列表
	// dirty_coords指向保存当前脏矩形区域的缓冲区
	// 计算当前角度指针的脏矩形
	// 将扇区张开的角度转换为指针旋转的角度
	rotate_angle = 135.0f + ((-135.0f) - 135.0f) * (angleExtent_0 - 0.0f) / (END_ANGLE  - 0.0f);
	dirty_count = scanSubBoundingRect (rotate_angle, CIRCLE_0_X, CIRCLE_0_Y, dirty_coords);
	total_dirty_count += dirty_count;
	// 更新当前FrameBuffer对应的脏矩形区域, 以便下次重绘该FrameBuffer时将该脏矩形区域包含并重绘
	memcpy (vehicle_speeds_dirty_rects_coords[current_fb_no], dirty_coords, dirty_count * 4 * sizeof(VGint));
	vehicle_speeds_dirty_rects_count[current_fb_no] = dirty_count;
	dirty_coords += dirty_count * 4;
	
	
	// 计算指针1(rotate_speeds)的脏矩形区域
	// 复制上一次的脏矩形列表
	if( rotate_speeds_dirty_rects_count[current_fb_no])
	{
		// 复制该FB(FrameBuffer)保存的上一次的脏矩形列表, 因为该区域需要被重绘(上一次的指针在此位置显示)
		dirty_count = rotate_speeds_dirty_rects_count[current_fb_no];
		memcpy (dirty_coords, rotate_speeds_dirty_rects_coords[current_fb_no], dirty_count * 4 * sizeof(VGint));
		dirty_coords += dirty_count * 4;
		total_dirty_count += dirty_count;
	}
	// 计算新位置的脏矩形列表
	// dirty_coords指向保存当前脏矩形区域的缓冲区
	// 计算当前角度指针的脏矩形
	// 将扇区张开的角度转换为指针旋转的角度
	rotate_angle = 135.0f + ((-135.0f) - 135.0f) * (angleExtent_1 - 0.0f) / (END_ANGLE  - 0.0f);
	dirty_count = scanSubBoundingRect (rotate_angle, CIRCLE_1_X, CIRCLE_1_Y, dirty_coords);
	total_dirty_count += dirty_count;
	// 更新当前FrameBuffer对应的脏矩形区域, 以便下次重绘该FrameBuffer时将该脏矩形区域包含并重绘
	memcpy (rotate_speeds_dirty_rects_coords[current_fb_no], dirty_coords, dirty_count * 4 * sizeof(VGint));
	rotate_speeds_dirty_rects_count[current_fb_no] = dirty_count;
	
	// 累加表盘0时速值显示区域的矩形区域
	total_dirty_count += process_dial_level_rectangle(0, coords + total_dirty_count * 4, MAX_DIRTY_RECTS - total_dirty_count);
	// 累加表盘1转速值显示区域的矩形区域
	total_dirty_count += process_dial_level_rectangle(1, coords + total_dirty_count * 4, MAX_DIRTY_RECTS - total_dirty_count);
		
#else
	// 脏矩形计算, 使用脏矩形优化GPU显示
	// 复位脏矩形统计
	memset (coords, 0, sizeof(coords));
	dirty_rects = 0;
	
	// 统计时速表光晕/指针变化导致的脏矩形
	dirty_rects = process_dial_dirty_rectangles (0, old_vehicle_speed, vehicle_speed,  coords, MAX_DIRTY_RECTS);
	// 累加转速表光晕/指针变化导致的脏矩形
	dirty_rects += process_dial_dirty_rectangles (1, old_rotate_speed, rotate_speed,  coords + dirty_rects * 4, MAX_DIRTY_RECTS - dirty_rects);
	// 累加表盘0时速值显示区域的矩形区域
	dirty_rects += process_dial_level_rectangle(0, coords + dirty_rects * 4, MAX_DIRTY_RECTS - dirty_rects);
	// 累加表盘1转速值显示区域的矩形区域
	dirty_rects += process_dial_level_rectangle(1, coords + dirty_rects * 4, MAX_DIRTY_RECTS - dirty_rects);
#endif

	// 计算scissoring rectangles(裁减区)
	if (full_dial_output_count > 0)
	{
		// 第一次/第二次输出, 全区域元素输出
		// VG关闭裁剪区
		vgSeti(VG_SCISSORING, VG_FALSE);
		full_dial_output_count --;
	}
	else if(vehicle_speed == old_vehicle_speed && rotate_speed == old_rotate_speed)
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
		// 定义4个裁减矩形
#ifdef NEW_DIRTY_METHOD		
		vgSetiv(VG_SCISSOR_RECTS, total_dirty_count * 4, coords);
#else
		vgSetiv(VG_SCISSOR_RECTS, dirty_rects * 4, coords);
#endif
	}

	// 计算表盘脏矩形对ICON状态的影响
	//   若表盘脏矩形与某个ICON存在交叉, 则该ICON需要重新绘制
	scan_and_mark_icons_polluted_by_dial_paint (coords, dirty_rects);


	// 1 绘制指针的背景图
	// 设置绘制图像与绘制表面为1:1映射模式(完全覆盖)
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	VGfloat m[9];	
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgLoadIdentity();	// 单位矩阵
	// 将自底向上转换为自顶向下, 因为vgDrawImageDirect使用的image是自顶向下
	vgGetMatrix(&m[0]);
	m[4] = -1;
	m[7] = VG_H;
	vgLoadMatrix(&m[0]);
	

	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	// 绘制背景Image对象
	// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
	vgDrawImageDirect ((char *)VG_IMAGE_DATA(bkimg), 	// 背景源图像基地址, 必须为64字节对齐
							 VG_IMAGE_WIDTH(bkimg),
							 VG_IMAGE_HEIGHT(bkimg),
							  VG_IMAGE_STRIDE(bkimg),
							 // 源图像开窗设置(全画面输出)
							 0,
							 0,
							  VG_IMAGE_WIDTH(bkimg),
							 VG_IMAGE_HEIGHT(bkimg),
							 VG_IMAGE_QUALITY_BETTER,
							 VG_IMAGE_FORMAT(bkimg)
							);
		
	
	// 绘制表盘0
	vgLoadIdentity();	// 单位矩阵
	// 叠加光晕效果
	drawHalo (startAngle, angleExtent_0, CIRCLE_0_X, MAP_SVG_TO_OPENVG(CIRCLE_0_Y,VG_H));
	// 叠加指针
	drawPointer (pointerPath_0, angleExtent_0 , CIRCLE_0_X, CIRCLE_0_Y);
	
	// 20211028 Bug修复
	// halo底图(同一个Image对象)在两处(表盘0与表盘1)被引用绘制, 因为其坐标映射存在不同, 需要串行使用, 否则VG底层检查该异常并报告"gcoOS_DebugBreak".
	// 此时插入vgFinish确保表盘1在表盘0绘制后开始绘制, 避免IMAGE对象资源被同时使用. (绘制时间会有少许增加)
	// 或者使用2个不同的halo Image对象规避资源共享异常
	vgFinish ();
	
	// 绘制表盘1
	// 叠加光晕效果
	drawHalo (startAngle, angleExtent_1, CIRCLE_1_X, MAP_SVG_TO_OPENVG(CIRCLE_1_Y,VG_H));
	// 叠加指针1
	// 使用与表盘0相同的指针对象
	//drawPointer (pointerPath_0, angleExtent_1, CIRCLE_1_X, CIRCLE_1_Y);
	// 使用与表盘0不同的指针对象
	drawPointer (pointerPath_1, angleExtent_1, CIRCLE_1_X, CIRCLE_1_Y);
	
	// 绘制时速/转速信息
	draw_dial_speed(vehicle_speed, rotate_speed);

#ifdef NEW_DIRTY_METHOD
	//drawBoundingRect (vehicle_speeds_dirty_rects_coords[current_fb_no]);
	//drawBoundingRect (rotate_speeds_dirty_rects_coords[current_fb_no]);
#endif	

}

int double_pointer_halo_draw (void)
{
	static float vehicle_speed = 0.0f;
	static int v_dir = 0;
	static float rotate_speed = 1.0;
	static int r_dir = 0;
		
	// 绘制双表盘
	do_dials_paint (vehicle_speed, rotate_speed);
	
	// 此处需要加入vgFinish指令刷新等待上述双仪表绘制时未完成的或者未释放的资源(不等待会导致内存资源需求更大或者画面刷新存在异常)
	//vgFinish();

	// 绘制ICONs
	draw_icons();
	draw_message();
	vgFinish();
	
//	for (angleExtent = 0.0; angleExtent >= -244.0f; angleExtent -= 1.0f)
	//while(1)
	{
		//for (angleExtent = -2.0; angleExtent >= -242.0f; angleExtent -= 1.0f)
		{
			//vehicle_speed = 118.0f;
			//if ( vgGetError() == VG_NO_ERROR ) 
			//  eglSwapBuffers( egldisplay, eglsurface );
			//Sleep (10);


			if(v_dir == 0 && vehicle_speed < 240.0f)
			{
				// 顺时针
				vehicle_speed += 0.9f;
				if (vehicle_speed >= 240.0f)
				{
					vehicle_speed = 240.0f;
					v_dir = 1;
				}
			}
			else if (v_dir == 1 && vehicle_speed > 0.0f)
			{
				// 逆时针
				vehicle_speed -= 0.9f;
				if (vehicle_speed <= 0.0f)
				{
					vehicle_speed = 0.0f;
					v_dir = 0;
				}
			}


			if (r_dir == 0 && rotate_speed < 8.0f)
			{
				rotate_speed += 0.02f;
				if (rotate_speed >= 8.0f)
				{
					rotate_speed = 8.0f;
					r_dir = 1;
				}
			}
			else if (r_dir == 1 && rotate_speed > 0.0f)
			{
				rotate_speed -= 0.02f;
				if (rotate_speed <= 0.0f)
				{
					rotate_speed = 0.0f;
					r_dir = 0;
				}
			}
		}
	}
	
	icon_state_simulate ();
	

	return 0;
}

int double_pointer_halo_init (int width, int height)
{
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	
	vgSeti(VG_PIXEL_LAYOUT, VG_PIXEL_LAYOUT_RGB_VERTICAL );

	
	// ARGB8888格式光晕底图
	// 
	// PNG格式设计图      
	//     ".\image\原始设计数据\背景\halo.png"
	// 运行".\image\原始设计数据\背景\run.bat", 将PNG格式的设计底图转换为ARGB8888格式
	//     ARGB8888格式BIN文件 ".\image\原始设计数据\背景\PNG2VG\ARGB8888\ROM_IMAGE_HALO_ARGB8888.ARGB8888"
	// 将该文件复制至 ".\rom\image\"用于ROM.BIN文件制作
	//     ARGB8888格式BIN文件 ".\rom\image\ROM_IMAGE_HALO_ARGB8888.ARGB8888"
	halo = (unsigned char *)XM_RomAddress(ROM_IMAGE_HALO_ARGB8888);		// ARGB8888格式
	
	// RGB565格式背景图
	//
	// PNG格式设计图
   //  1) 原始设计底图 32位PNG格式
   //     ".\image\原始设计数据\背景\BG_RGB32.png"
	//  2) 已转换到RGB16的PNG格式
	//      .\image\原始设计数据\背景\BGRGB565.png"
	//      参考"\image\原始设计数据\背景\readme.txt"了解如何将图像颜色从RGB32转换到RGB16,且尽可能保留细节
	//
	// 运行".\image\原始设计数据\背景\run.bat", 将PNG格式的设计底图转换为RGB565格式
	//     RGB565格式BIN文件 ".\image\原始设计数据\背景\PNG2VG\RGB565\BGRGB565.RGB565"
	// 将该文件复制至 ".\rom\image\"用于ROM.BIN文件制作
	//     RGB565格式BIN文件 ".\rom\image\BGRGB565.RGB565"
	bkimg = (unsigned short *)XM_RomAddress(ROM_IMAGE_BGRGB565_RGB565);
	
	genPointerPaths();
	
	// 计算指针的包络矩形区域
	calcPointerBoundingRect();
	
	vgFontInit();
	init_icons();
	
	dial_vehicle_speeds[0] = 0.0f;
	dial_vehicle_speeds[1] = 0.0f;
	dial_rotate_speeds[0] = 0.0f;
	dial_rotate_speeds[1] = 0.0f;
	
	full_dial_output_count = 2;
	
	return 0;
}

int double_pointer_halo_exit (void)
{
	vgFontExit();
	return 0;
}



// 脏区域矩形定义, 主要用于通过限定某个区域输出, 大幅较少GPU对DDR的访问
typedef struct {
	// 与该脏区域矩形相关的参数值的限定范围. 如转速表 (1000 ~ 2000),  时速表 (0 ~ 40)
	float th_min;	//  最小值
	float th_max;	//  最大值

	// 脏区域矩形坐标(屏幕坐标)及尺寸, (自顶向下坐标系中定义, Windows的画笔程序使用)
	VGint x;		
	VGint y;
	VGint w;
	VGint h;
} VG_DIRTY_RECT;

#ifndef NEW_DIRTY_METHOD
// 表盘0脏矩形区域定义(与该表盘时速刻度值关联)
// 定义了6个脏矩形区域 (可以细化,定义更多的刻度范围与矩形区域对应关系)
// 每次刻度值变化时, 仅绘制对应的矩形区域
// 定义脏矩形时应包含该次光晕与指针的变化
// 表盘0的刻度范围为(0.0 ~ 240.0)
static const VG_DIRTY_RECT dial_0_rects[] = {
	// 按照th_min的递增顺序制作
	{
		0.0, 40.0,			// 时速刻度值的刻度范围(从刻度0到刻度40)
		8, 211, 173, 202	// 包含上述刻度范围(从刻度0到刻度40)的矩形区域
	},
	{
		40.0, 80.0,			// 时速刻度值的刻度范围(从刻度40到刻度80)
		9, 50, 182, 228		// 包含上述刻度范围(从刻度40到刻度80)的矩形区域
	},
	{
		80.0, 120.0,
		65, 8, 215, 167
	},
	{
		120.0, 160.0,
		220, 8, 187, 157
	},
	{
		160.0, 200.0,
		312, 76, 177, 185
	},
	{
		200.0, 240.0,
		319, 223, 157, 184
	},
};

// 表盘1脏矩形区域定义(与该表盘转速刻度值关联)
// 定义了8个脏矩形区域 (可以细化,定义更多的刻度范围与矩形区域对应关系)
// 表盘1的刻度范围为(0.0 ~ 8.0)
static const VG_DIRTY_RECT dial_1_rects[] = {
	// 按照th_min的递增顺序制作
	{
		0.0, 1.0,				// 转速刻度值的限定区域(从刻度0到刻度1)
		798, 249, 161, 148		// 包含上述刻度范围(从刻度0到刻度1)的矩形区域
	},
	{
		1.0, 2.0,
		811, 133, 126, 161
	},
	{
		2.0, 3.0,
		824, 32, 164, 175
	},
	{
		3.0, 4.0,
		857, 13, 198, 137
	},
	{
		4.0, 5.0,
		1017, 13, 154, 145
	},
	{
		5.0, 6.0,
		1095, 53, 157, 147
	},
	{
		6.0, 7.0,
		1138, 145, 122, 156
	},
	{
		7.0, 8.0,
		1119, 246, 135, 153
	},
};

static int get_dirty_index (int dial_no, float th)
{
	const VG_DIRTY_RECT *rects = NULL;
	int index, count;
	
	if(dial_no == 0)	// 时速, 以40为等分, 参考 dial_0_rects 
	{
		rects = dial_0_rects;
		count = sizeof(dial_0_rects) / sizeof(dial_0_rects[0]);
	}
	else					// 转速, 以1.0为等分, 参考 dial_1_rects
	{
		rects = dial_1_rects;
		count = sizeof(dial_1_rects) / sizeof(dial_1_rects[0]);
	}
	for (index = 0; index < count; index++)
	{
		if (th >= rects[index].th_min && th < rects[index].th_max)
			return index;
	}
	return count - 1;	
}

// 根据指针的新旧刻度值计算最少数量的脏矩形区域
// dial_no 表盘序号
// old_th, new_th 为新旧2个刻度值
// coords 保存求解出的一个或多个脏矩形区域集 (x,y,w,h)
// count  脏矩形集缓冲区的个数
// 返回值
//   已计算的脏矩形个数
static int process_dial_dirty_rectangles (int dial_no, float old_th, float new_th,  VGint* coords, int cb_coords)
{
	int count;
	int dirty_counts = 0;
	const VG_DIRTY_RECT *rects = NULL;
	const VG_DIRTY_RECT *rect;
	int old_index, new_index;
	if(old_th > new_th)
	{
		float t = new_th;
		new_th = old_th;
		old_th = t;
	}
	
	if(dial_no == 0)	// 时速, 以40为等分, 参考 dial_0_rects 
	{
		rects = dial_0_rects;
		count = sizeof(dial_0_rects) / sizeof(dial_0_rects[0]);
	}
	else					// 转速, 以1.0为等分, 参考 dial_1_rects
	{
		rects = dial_1_rects;
		count = sizeof(dial_1_rects) / sizeof(dial_1_rects[0]);
	}
	(void)(count);
	
	old_index = get_dirty_index (dial_no, old_th);
	new_index = get_dirty_index (dial_no, new_th);
	// 遍历并输出新/旧值之间的每个脏矩形, 这些区域均需要刷新
	while (cb_coords > 0 && old_index <= new_index)
	{
		rect = rects + old_index;
		
		coords[0] = rect->x;;
		coords[1] = VG_H - (rect->y + rect->h);	// 自顶向下-->自底向上
		coords[2] = rect->w;
		coords[3] = rect->h;
		
		coords += 4;
		cb_coords -= 1;
		dirty_counts ++;
		old_index ++;
	}
	return 	dirty_counts;
	
}
#endif


typedef struct _icon_image {
	unsigned int id;			// 资源的唯一标识
	char name[32];				// rom 文件名
	float x;						// 显示位置
	float y;
	
	unsigned int state[2];	// 保存与VG的2个FrameBuffer对应的ICON的最近状态值. 
										//			比较该值与ICON的最新状态值, 决定是否刷新该ICON对应的Surface区域并更新为ICON的最新状态值
} ICON_IMAGE;

enum {
	ICON_OIL = 0,
	ICON_WATER_TEMP,
	ICON_CAR,
	ICON_LEFT_FRONT,
	ICON_LEFT_REAR,
	ICON_RIGHT_FRONT,
	ICON_RIGHT_REAR,
	ICON_LEFT,
	ICON_RIGHT,
	ICON_ABS,
	ICON_ENGINE,	// 发动机
	ICON_ENGINE_OIL,	// 机油
	ICON_LOWER_BEAM,	// 近光
	ICON_STOP_LAMP,	// 刹车灯
	ICON_WIDTH_LAMP,	// 示宽灯
	ICON_DOUBLE_FLASHING,	// 双闪
	ICON_FOGLIGHT,		// 雾灯
	ICON_HIGH_BEAM,	// 远光
	ICON_OIL_LEVEL,	// 油耗标尺
	ICON_WATER_LEVEL,	// 水温标尺
	ICON_PERCENT,
	ICON_MODE,		// 模式
	ICON_GEAR,		// 档位
	ICON_ODO,		// 里程
	ICON_TEMP,
	ICON_COUNT
};

static ICON_IMAGE icon_image[ICON_COUNT] = {
	{
		ICON_OIL,
		ROM_IMAGE_OIL_HIGH_ARGB8888,
		234, 395,
		-1, -1,		
	},
	{
		ICON_WATER_TEMP,
		ROM_IMAGE_WATER_TEMP_HIGH_ARGB8888,
		1024, 395,
		-1, -1,		
	},
	{
		ICON_CAR,
		ROM_IMAGE_CAR_ARGB8888,
		586, 154.94,
		-1, -1,		
	},
	{
		ICON_LEFT_FRONT,
		ROM_IMAGE_LEFT_FRONT_ARGB8888,
		552, 214,
		-1, -1,		
	},	
	{
		ICON_LEFT_REAR,
		ROM_IMAGE_LEFT_REAR_ARGB8888,
		552, 268,
		-1, -1,		
	},	
	{
		ICON_RIGHT_FRONT,
		ROM_IMAGE_RIGHT_FRONT_ARGB8888,
		676, 214,
		-1, -1,		
	},	
	{
		ICON_RIGHT_REAR,
		ROM_IMAGE_RIGHT_REAR_ARGB8888,
		676, 268,
		-1, -1,		
	},	
	
	{
		ICON_LEFT,
		ROM_IMAGE_LEFT_1_ARGB8888,
		20, 20,
		-1, -1,		
	},	
	
	{
		ICON_RIGHT,
		ROM_IMAGE_RIGHT_1_ARGB8888,
		1190, 20,
		-1, -1,		
	},	
	
	// ABS
	{
		ICON_ABS,
		ROM_IMAGE_ABS_ARGB8888,
		662, 93,
		-1, -1,		
	},	
	
	// 发动机
	{
		ICON_ENGINE,
		ROM_IMAGE_ENGINE_ARGB8888,
		480, 93,
		-1, -1,		
	},	
	
	// 机油
	{
		ICON_ENGINE_OIL,
		ROM_IMAGE_ENGINE_OIL_ARGB8888,
		571, 93,
		-1, -1,		
	},	

	// 近光
	{
		ICON_LOWER_BEAM,
		ROM_IMAGE_LOWER_BEAM_ARGB8888,
		430, 22,
		-1, -1,		
	},	
	
	// 刹车灯
	{
		ICON_STOP_LAMP,
		ROM_IMAGE_STOP_LAMP_ARGB8888,
		616, 22,
		-1, -1,		
	},	
	
	// 示宽灯
	{
		ICON_WIDTH_LAMP,
		ROM_IMAGE_WIDTH_LAMP_ARGB8888,
		709, 22,
		-1, -1,		
	},	
	
	// 双闪
	{
		ICON_DOUBLE_FLASHING,
		ROM_IMAGE_DOUBLE_FLASHING_ARGB8888,
		753, 93,
		-1, -1,		
	},	

	// 雾灯
	{
		ICON_FOGLIGHT,
		ROM_IMAGE_FOGLIGHT_ARGB8888,
		523, 22,
		-1, -1,		
	},
	
	// 远光
	{
		ICON_HIGH_BEAM,
		ROM_IMAGE_HIGH_BEAM_ARGB8888,
		802, 22,
		-1, -1,		
	},
	
	
	// 油耗标尺
	{
		ICON_OIL_LEVEL,
		ROM_IMAGE_OIL_LEVEL_1_ARGB8888,
		156, 408,
		-1, -1
		
	},
	
	// 水温标尺
	{
		ICON_WATER_LEVEL,
		ROM_IMAGE_OIL_LEVEL_1_ARGB8888,
		948, 408,
		-1, -1
		
	},
	
	// PERCENT
	{
		ICON_PERCENT,
		"\0",
		13 + 81/2, 430 + 26/2,
		-1, -1
		
	},
	
	// MODE
	{
		ICON_MODE,
		"\0",
		0, 0,
		-1, -1
	},
	
	// GEARE
	{
		ICON_GEAR,
		"\0",
		0, 0,
		-1, -1
	},
	
	// ODO
	{
		ICON_ODO,
		"\0",
		0, 0,
		-1, -1
	},
	
	// TEMP
	{
		ICON_TEMP,
		"\0",
		0, 0,
		-1, -1
	}
	
	
};

// 保存ICON对应的最新状态值
static unsigned int icon_states[ICON_COUNT];




// 设置icon的最新状态值
void set_icon_state (int icon, unsigned int state)
{
	if(icon < 0 || icon >= ICON_COUNT)
		return;
	
	icon_states[icon] = state;
}

// 复位ICON的显示状态
void reset_icon_state (void)
{
	int i;
	for (i = 0; i < sizeof(icon_image)/sizeof(icon_image[0]); i ++)
	{
		icon_image[i].state[0] = -1;		// 与FB0(FrameBuffer 0)对应的ICON状态值
		icon_image[i].state[1] = -1;		// 与FB1对应的ICON状态值
	}
}

// 扫描所有被表盘绘制过程污染的ICON并标记
static void scan_and_mark_icon (ICON_IMAGE *icon_image, int fb_no, VGint* coords, int cb_coords)
{
	int i;
	// 转换为OpenVG坐标
	char *image = (char *)XM_RomAddress(icon_image->name);
	if(image == NULL)
		return;
	
	float ix = icon_image->x;
	float iy = VG_H - (icon_image->y + VG_IMAGE_HEIGHT(image));
	float iw = VG_IMAGE_WIDTH(image);
	float ih = VG_IMAGE_HEIGHT(image);
	
	if(iw == 0 || ih == 0)
		return;
	
	for (i = 0; i < cb_coords; i ++)
	{
		float dx = coords[0];
		float dy = coords[1];
		float dw = coords[2];
		float dh = coords[3];
		
		if( 	((ix + iw) <= dx)		// icon在脏区的左侧
			||	(ix >= (dx + dw))		// icon在脏区的右侧
			|| ((iy + ih) <= dy)		// icon在脏区的上侧
			|| (iy >= (dy + dh))		// icon在脏区的下侧
			)
		{
			// 无交集
		}
		else
		{
			// 有交集
			// 标记该ICON的最近状态值为-1, 强制其重新绘制
			icon_image->state[fb_no] = -1;
			return;
		}
		
		coords += 4;
	}
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
			// 平移至ICON待显示区域的左上角(该坐标为该ICON在LCD显示屏上的起始坐标)
			vgTranslate(icon_image[i].x, icon_image[i].y );
			vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
			char *image = (char *)XM_RomAddress(icon_image[i].name);
			if(image == NULL)
				return;
			// // 从背景图中挖出与ICON位置对应的区域重新填充背景, 背景图为RGB565格式
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect ((char *)VG_IMAGE_DATA(bkimg), 		// 背景源图像基地址, 必须为64字节对齐
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
			return;
		}

	}
}

// 绘制icon前景图
static void draw_icon_foreground (int icon_id)
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
			// 平移至ICON待显示区域的左上角(该坐标为该ICON在LCD显示屏上的起始坐标)
			vgTranslate(icon_image[i].x, icon_image[i].y );
			vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
			// 获取待绘制的ICON图像基址
			char *image = (char *)XM_RomAddress(icon_image[i].name);
			if(image == NULL)
				return;
			
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect ((char *)VG_IMAGE_DATA(image), 		// 源图像基地址, 必须为64字节对齐
												 VG_IMAGE_WIDTH(image),		// 源图像像素宽度
												 VG_IMAGE_HEIGHT(image),		// 源图像像素高度
												 VG_IMAGE_STRIDE(image),		// 源图像行字节长度, 必须满足16像素对齐
												 (VGint)0,		// 源图像开窗参数, 
												 (VGint)0,
												 (VGint)VG_IMAGE_WIDTH(image),
												 (VGint)VG_IMAGE_HEIGHT(image),
												 VG_IMAGE_QUALITY_BETTER,		// 图像渲染质量
												 // 源图像格式
												 VG_IMAGE_FORMAT(image) 
												);
			return;
		}
	}
}



// 扫描所有被表盘绘制污染的ICON并标记
static void scan_and_mark_icons_polluted_by_dial_paint (VGint* coords, int cb_coords)
{
	int i;
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb (&current_fb_no);
	for (i = 0; i < sizeof(icon_image)/sizeof(icon_image[0]); i ++)
	{
		scan_and_mark_icon (icon_image + i, current_fb_no, coords, cb_coords);
	}
}

// 油耗/水温标尺
static const char *imageOilLevel[4] = {
	ROM_IMAGE_OIL_LEVEL_1_ARGB8888,
	ROM_IMAGE_OIL_LEVEL_2_ARGB8888,
	ROM_IMAGE_OIL_LEVEL_3_ARGB8888,
	ROM_IMAGE_OIL_LEVEL_4_ARGB8888	
};

// 绘制油耗/水温标尺 
static void draw_oil_level_icon (int icon_id)
{

	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb (&current_fb_no);
	ICON_IMAGE *icon = icon_image + icon_id;
	// 检查ICON的最新状态值是否与FB相关的最近状态值相同
	unsigned int last_state = icon_states[icon_id];
	if(last_state != icon_image[icon_id].state[current_fb_no])
	{
		icon_image[icon_id].state[current_fb_no] =  last_state;
		
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
		if (icon_id == ICON_OIL_LEVEL)
		{
			// 重新填充背景
			draw_icon_background (icon_id);
			// PNG绘制
			char *image = (char *)XM_RomAddress(imageOilLevel[last_state]);
			if(image == NULL)
				return;
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect ((char *)VG_IMAGE_DATA(image), 
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
		else if(icon_id == ICON_WATER_LEVEL)
		{
			// 重新填充背景
			draw_icon_background (icon_id);
			// PNG绘制
			char *image = (char *)XM_RomAddress(imageOilLevel[last_state]);
			if(image == NULL)
				return;
			// 直接绘制, 注意源图像必须满足地址64字节对齐, stride 满足16像素对齐
			vgDrawImageDirect ((char *)VG_IMAGE_DATA(image), 
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
		
		// 强制油温图标重绘
		if(icon_id == ICON_OIL_LEVEL)
		{
			draw_icon_foreground (ICON_OIL);
		}
		// 强制水温图标重绘
		else if(icon_id == ICON_WATER_LEVEL)
		{
			draw_icon_foreground (ICON_WATER_TEMP);
		}
	}
	
}


static void init_icons (void)
{
	int i;
	
	for (i = 0; i < ICON_COUNT; i ++)
	{
		icon_states[i] = 1;
	}

	
	
	// 复位icon状态
	reset_icon_state ();
}

static void draw_icon (int icon_id)
{
	int i;
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb(&current_fb_no);
	
	for (i = 0; i < sizeof(icon_image) / sizeof(icon_image[0]); i++)
	{
		if (icon_image[i].id == icon_id)
		{
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
					draw_icon_background (icon_id);
				}
				
				if(last_state == 1)
				{
					draw_icon_foreground (icon_id);					
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


// 空格字符(空格/TAB)的数据请手工加入到字体C文件(按照字符的Unicode编码顺序), 其中宽度信息(0.556152f)请按照参考其他字符定义
// 	{    32, { 0.556152f, 0.000000f },   0, NULL, 0, NULL, VG_NON_ZERO },
//   加入后请修改number of glyphs
// 	number of glyphs
//		65+1,


static void draw_message (void)
{
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb (&current_fb_no);
	
	
	if(icon_image[ICON_PERCENT].state[current_fb_no] != icon_states[ICON_PERCENT])
	{
		char info[5];
		icon_image[ICON_PERCENT].state[current_fb_no] = icon_states[ICON_PERCENT];
		sprintf (info, "%d%%", icon_image[ICON_PERCENT].state[current_fb_no]);
		// 
		VGfloat clrcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};	// 设置clear颜色(黑色)
		vgSetfv (VG_CLEAR_COLOR, 4, clrcolor);
		vgClear (13, VG_H - (430 + 26 + 3), 81, 26); // 清除背景	(13,430) (81, 26)
		vgFlush();
		DrawSpeed (info, 16, 22, 13 + 81/2, 430 + 26/2, RGB(255, 255, 255));
	}
	
	
	// 模式
	if(icon_image[ICON_MODE].state[current_fb_no] != icon_states[ICON_MODE])
	{
		icon_image[ICON_MODE].state[current_fb_no] = icon_states[ICON_MODE];
		VGfloat clrcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		vgSetfv (VG_CLEAR_COLOR, 4, clrcolor);
		vgClear (476, VG_H - (429+34), 119, 34);
		vgFlush();
		DrawSpeed ("SNOW", 18, 24, 476 + 119/2, 429 + 34/2, RGB(4, 193, 252));
	}
	
	if(icon_image[ICON_GEAR].state[current_fb_no] != icon_states[ICON_GEAR])
	{
		icon_image[ICON_GEAR].state[current_fb_no] = icon_states[ICON_GEAR];
		VGfloat clrcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		vgSetfv (VG_CLEAR_COLOR, 4, clrcolor);
		vgClear (597, VG_H - (411+59), 86, 59);
		vgFlush();
		DrawSpeed ("D", 28, 36, 597 + 86/2, 411 + 59/2, RGB(255, 255, 255));
	}
	
	if(icon_image[ICON_ODO].state[current_fb_no] != icon_states[ICON_ODO])
	{
		icon_image[ICON_ODO].state[current_fb_no] = icon_states[ICON_ODO];
		VGfloat clrcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		vgSetfv (VG_CLEAR_COLOR, 4, clrcolor);
		vgClear (685, VG_H - (430+34), 247, 34);
		vgFlush();
		char info[24];
		// 注意: 以下文字中的空格字符 ' ' 需手工在字形代码文件将其加入
		//
		// 空格字符(空格/TAB)的数据请手工加入到字体C文件(按照字符的Unicode编码顺序), 其中宽度信息(0.556152f)请按照参考其他字符定义
		//    如字体的宽度为16, 此处宽度信息(0.556152f), 则空格的像素宽度 = 16 * 0.556152f = 8
		// 	{    32, { 0.556152f, 0.000000f },   0, NULL, 0, NULL, VG_NON_ZERO },
		//   加入后请修改字体数据结构的number of glyphs, 将其值加1
		// 	number of glyphs
		sprintf (info, "ODO%6d km", icon_image[ICON_ODO].state[current_fb_no]);
		DrawSpeed (info, 16, 22, 685 + 247/2, 430 + 34/2, RGB(128, 128, 128));
	}
	
	if(icon_image[ICON_TEMP].state[current_fb_no] != icon_states[ICON_TEMP])
	{
		icon_image[ICON_TEMP].state[current_fb_no] = icon_states[ICON_TEMP];
		VGfloat clrcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		vgSetfv (VG_CLEAR_COLOR, 4, clrcolor);
		vgClear (1187, VG_H - (430+26), 81, 26);	// (1187, 430, 81, 26)
		vgFlush();
		char info[24];
		// \xE2\x84\x83是摄氏度符号的UTF8编码
		// 注意: 测试用例摄氏度符号的字形数据来自 ARIALUNI.TTF, 因为arial.ttf无此字形
		sprintf (info, "%3d\xE2\x84\x83", icon_image[ICON_TEMP].state[current_fb_no]);
		DrawSpeed (info, 16, 22, 1187 + 81/2, 430 + 26/2, RGB(255, 255, 255));
	}
	
}

// 绘制车门状态
static void draw_card_door_status (void)
{
	int do_refresh = 0;
	
	// 获取当前FrameBuffer对应的序号
	int current_fb_no = -1;
	xm_vg_get_osd_fb (&current_fb_no);

	// 检查车门状态是否存在变化
	unsigned int last_state;
	do
	{
		// 读取最新的状态
		last_state = icon_states[ICON_LEFT_FRONT];
		if(last_state != icon_image[ICON_LEFT_FRONT].state[current_fb_no])
		{
			icon_image[ICON_LEFT_FRONT].state[current_fb_no] = last_state;
			do_refresh = 1;
		}
		
		last_state = icon_states[ICON_RIGHT_FRONT];
		if(last_state != icon_image[ICON_RIGHT_FRONT].state[current_fb_no])
		{
			icon_image[ICON_RIGHT_FRONT].state[current_fb_no] = last_state;
			do_refresh = 1;
		}
		
		last_state = icon_states[ICON_LEFT_REAR];
		if(last_state != icon_image[ICON_LEFT_REAR].state[current_fb_no])
		{
			icon_image[ICON_LEFT_REAR].state[current_fb_no] = last_state;
			do_refresh = 1;
		}
		
		last_state = icon_states[ICON_RIGHT_REAR];
		if(last_state != icon_image[ICON_RIGHT_REAR].state[current_fb_no])
		{
			icon_image[ICON_RIGHT_REAR].state[current_fb_no] = last_state;
			do_refresh = 1;
		}
		
		
	} while (0);
	
	if(do_refresh == 0)
		return;
	
	// 全部重新绘制
	// 清除背景
	// (539, 149) (199,215)
	VGfloat clrcolor[4] = {0.0f, 0.0f, 0.0f, 1.0f};	// 设置clear颜色(黑色)
	vgSetfv (VG_CLEAR_COLOR, 4, clrcolor);
	vgClear (539, VG_H - (149 + 215), 199, 215); // 清除背景	
	vgFlush();
	
	// 绘制车模型
	draw_icon_foreground (ICON_CAR);

	if(icon_image[ICON_LEFT_FRONT].state[current_fb_no])
	{
		draw_icon_foreground (ICON_LEFT_FRONT);
	}
	if(icon_image[ICON_RIGHT_FRONT].state[current_fb_no])
	{
		draw_icon_foreground (ICON_RIGHT_FRONT);
	}
	if(icon_image[ICON_LEFT_REAR].state[current_fb_no])
	{
		draw_icon_foreground (ICON_LEFT_REAR);
	}
	if(icon_image[ICON_RIGHT_REAR].state[current_fb_no])
	{
		draw_icon_foreground (ICON_RIGHT_REAR);
	}
	
}

// ICON输出时未使用裁减区域加速GPU显示, 用户可根据ICON的状态变化创建相应的裁减区域来优化GPU访问
static void draw_icons (void)
{
	VGint coords[4];
	
	coords[0] = 0;
	coords[1] = 0;
	coords[2] = VG_W;
	coords[3] = VG_H;
	vgSeti(VG_SCISSORING, VG_TRUE);
	vgSetiv(VG_SCISSOR_RECTS, 4, coords);
		
	draw_icon (ICON_LEFT);
	draw_icon (ICON_RIGHT);
	
	draw_card_door_status ();

	draw_icon (ICON_ABS);
	draw_icon (ICON_ENGINE);
	draw_icon (ICON_ENGINE_OIL);
	draw_icon (ICON_LOWER_BEAM);
	draw_icon (ICON_STOP_LAMP);

	draw_icon (ICON_WIDTH_LAMP);
	draw_icon (ICON_DOUBLE_FLASHING);
	draw_icon (ICON_FOGLIGHT);
	draw_icon (ICON_HIGH_BEAM);
	
	
	// 水温坐标：948  408   油耗坐标：156  408
	// 水温标尺
	draw_oil_level_icon (ICON_OIL_LEVEL);
	// 水温标尺的绘制会污染水温图标
	draw_icon (ICON_OIL);
	
	draw_oil_level_icon (ICON_WATER_LEVEL);
	// 油耗标尺的绘制会污染油耗图标
	draw_icon (ICON_WATER_TEMP);
	
}

#define	ICON_TEST
static int refresh_count = 2;
static unsigned int start_ticket;

static void icon_state_simulate (void)
{
	// 模拟ICON状态变化
	if(refresh_count > 0)
	{
		// 用于保证2帧FB初始为相同的ICON状态
		refresh_count --;
		start_ticket = XM_GetTickCount();
		return;
	}
	
#ifdef ICON_TEST
	// 模拟ICON测试
	
	// 修改转速
	unsigned int time = XM_GetTickCount () - start_ticket;
	//icon_values[ICON_TYPE_ROTATE_SPEED] = (time / 400) % 13;
	
	icon_states[ICON_LEFT] = (time / 2000) % 2;
	icon_states[ICON_RIGHT] = (time / 3000) % 2;
	
	
	icon_states[ICON_LEFT_FRONT] = 1 - ((time / 2500) % 2);
	icon_states[ICON_LEFT_REAR] = 1 - ((time / 3000) % 2);
	icon_states[ICON_RIGHT_FRONT] = 1 - ((time / 4000) % 2);
	icon_states[ICON_RIGHT_REAR] = 1 - ((time / 4200) % 2);
	
	
	icon_states[ICON_OIL_LEVEL] = (time / 2000) % 4;
	icon_states[ICON_WATER_LEVEL] = (time / 3000) % 4;
	
	icon_states[ICON_WIDTH_LAMP] = 1 - ((time / 7600) % 2);
	icon_states[ICON_FOGLIGHT] = 1 - ((time / 4600) % 2);
	icon_states[ICON_HIGH_BEAM] = 1 - ((time / 5800) % 2);
	icon_states[ICON_DOUBLE_FLASHING] = 1 - ((time / 5400) % 2);
	icon_states[ICON_HIGH_BEAM] = 1 - ((time / 5700) % 2);
		
	icon_states[ICON_PERCENT] = (time / 1500) % 100;
	
	icon_states[ICON_ODO] = (time / 1000) ;
	
	icon_states[ICON_TEMP] = (time / 1300) % 150;

#endif		
}

#endif
