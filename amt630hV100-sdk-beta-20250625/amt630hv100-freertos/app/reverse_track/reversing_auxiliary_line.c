//#include <windows.h>
#include <vg/openvg.h>
#include <vg/vgu.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "reversing_auxiliary_line.h"


// 红黄绿辅助线及轨迹线的原始SVG数据. 
// 直接运行.\image\run.bat, 将原始的SVG格式设计数据转换为OpenVG格式的C数据文件.
// 注意: SVG格式的坐标系是自顶向下, 使用时应将其转换到OpenVG坐标系. 
//
// 参考如下 do_paint_auxiliary_line 代码中的坐标映射
// 	VGfloat  m[9];
//	// 设置自顶向下(SVG坐标系)到自底向上(OpenVG)的坐标映射, 将SVG坐标转换为OpenVG坐标
//	// 以下红黄绿辅助线及轨迹线均使用SVG元素绘制.
//	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
//	vgGetMatrix(m);
//	m[4] = -1;
//	m[7] = VG_H;
//	vgLoadMatrix(m);
// 
#include ".\image\bg.c"
#include ".\image\BackLine001.c"
#include ".\image\BackLine002.c"
#include ".\image\BackLine003.c"
#include ".\image\BackLine004.c"
#include ".\image\BackLine005.c"
#include ".\image\BackLine006.c"
#include ".\image\BackLine007.c"
#include ".\image\BackLine008.c"
#include ".\image\BackLine009.c"
#include ".\image\BackLine010.c"
#include ".\image\BackLine011.c"
#include ".\image\BackLine012.c"
#include ".\image\BackLine013.c"
#include ".\image\BackLine014.c"
#include ".\image\BackLine015.c"
#include ".\image\BackLine016.c"
#include ".\image\BackLine017.c"
#include ".\image\BackLine018.c"
#include ".\image\BackLine019.c"
#include ".\image\BackLine020.c"
#include ".\image\BackLine021.c"
#include ".\image\BackLine022.c"
#include ".\image\BackLine023.c"
#include ".\image\BackLine024.c"
#include ".\image\BackLine025.c"
#include ".\image\BackLine026.c"
#include ".\image\BackLine027.c"
#include ".\image\BackLine028.c"
#include ".\image\BackLine029.c"
#include ".\image\BackLine030.c"
#include ".\image\BackLine031.c"
#include ".\image\BackLine032.c"
#include ".\image\BackLine033.c"
#include ".\image\BackLine034.c"
#include ".\image\BackLine035.c"
#include ".\image\BackLine036.c"
#include ".\image\BackLine037.c"
#include ".\image\BackLine038.c"
#include ".\image\BackLine039.c"
#include ".\image\BackLine040.c"
#include ".\image\BackLine041.c"
#include ".\image\BackLine042.c"
#include ".\image\BackLine043.c"
#include ".\image\BackLine044.c"
#include ".\image\BackLine045.c"
#include ".\image\BackLine046.c"
#include ".\image\BackLine047.c"
#include ".\image\BackLine048.c"
#include ".\image\BackLine049.c"
#include ".\image\BackLine050.c"
#include ".\image\BackLine051.c"
#include ".\image\BackLine052.c"
#include ".\image\BackLine053.c"
#include ".\image\BackLine054.c"
#include ".\image\BackLine055.c"
#include ".\image\BackLine056.c"
#include ".\image\BackLine057.c"
#include ".\image\BackLine058.c"
#include ".\image\BackLine059.c"
#include ".\image\BackLine060.c"
#include ".\image\BackLine061.c"
#include ".\image\BackLine062.c"
#include ".\image\BackLine063.c"
#include ".\image\BackLine064.c"
#include ".\image\BackLine065.c"
#include ".\image\BackLine066.c"
#include ".\image\BackLine067.c"
#include ".\image\BackLine068.c"
#include ".\image\BackLine069.c"
#include ".\image\BackLine070.c"
#include ".\image\BackLine071.c"
#include ".\image\BackLine072.c"
#include ".\image\BackLine073.c"
#include ".\image\BackLine074.c"
#include ".\image\BackLine075.c"
#include ".\image\BackLine076.c"
#include ".\image\BackLine077.c"
#include ".\image\BackLine078.c"
#include ".\image\BackLine079.c"
#include ".\image\BackLine080.c"
#include ".\image\BackLine081.c"
#include ".\image\BackLine082.c"
#include ".\image\BackLine083.c"
#include ".\image\BackLine084.c"
#include ".\image\BackLine085.c"
#include ".\image\BackLine086.c"
#include ".\image\BackLine087.c"
#include ".\image\BackLine088.c"
#include ".\image\BackLine089.c"
#include ".\image\BackLine090.c"
#include ".\image\BackLine091.c"
#include ".\image\BackLine092.c"
#include ".\image\BackLine093.c"
#include ".\image\BackLine094.c"
#include ".\image\BackLine095.c"
#include ".\image\BackLine096.c"
#include ".\image\BackLine097.c"
#include ".\image\BackLine098.c"
#include ".\image\BackLine099.c"
#include ".\image\BackLine100.c"
#include ".\image\BackLine101.c"
#include ".\image\BackLine102.c"
#include ".\image\BackLine103.c"
#include ".\image\BackLine104.c"
#include ".\image\BackLine105.c"
#include ".\image\BackLine106.c"
#include ".\image\BackLine107.c"
#include ".\image\BackLine108.c"
#include ".\image\BackLine109.c"
#include ".\image\BackLine110.c"
#include ".\image\BackLine111.c"
#include ".\image\BackLine112.c"
#include ".\image\BackLine113.c"


// 轨迹线描述结构
typedef struct {
	const VGubyte *command;
	int cb_command;
	const VGfloat *coordinate;
} BACK_LINE;

#define	L_DEF(num)	{ BackLine##num##_path_0_commands , sizeof(BackLine##num##_path_0_commands )/sizeof(BackLine##num##_path_0_commands [0]),  BackLine##num##_path_0_coordinates }
#define	R_DEF(num)	{ BackLine##num##_path_1_commands , sizeof(BackLine##num##_path_1_commands )/sizeof(BackLine##num##_path_1_commands [0]),  BackLine##num##_path_1_coordinates }

// 左轨迹数据
static const BACK_LINE back_lines_left[] = {
	L_DEF(001),
	L_DEF(002),
	L_DEF(003),
	L_DEF(004),
	L_DEF(005),
	L_DEF(006),
	L_DEF(007),
	L_DEF(008),
	L_DEF(009),
	L_DEF(010),
	L_DEF(011),
	L_DEF(012),
	L_DEF(013),
	L_DEF(014),
	L_DEF(015),
	L_DEF(016),
	L_DEF(017),
	L_DEF(018),
	L_DEF(019),
	L_DEF(020),
	L_DEF(021),
	L_DEF(022),
	L_DEF(023),
	L_DEF(024),
	L_DEF(025),
	L_DEF(026),
	L_DEF(027),
	L_DEF(028),
	L_DEF(029),
	L_DEF(030),
	L_DEF(031),
	L_DEF(032),
	L_DEF(033),
	L_DEF(034),
	L_DEF(035),
	L_DEF(036),
	L_DEF(037),
	L_DEF(038),
	L_DEF(039),
	L_DEF(040),
	L_DEF(041),
	L_DEF(042),
	L_DEF(043),
	L_DEF(044),
	L_DEF(045),
	L_DEF(046),
	L_DEF(047),
	L_DEF(048),
	L_DEF(049),
	L_DEF(050),
	L_DEF(051),
	L_DEF(052),
	L_DEF(053),
	L_DEF(054),
	L_DEF(055),
	L_DEF(056),
	L_DEF(057),
	L_DEF(058),
	L_DEF(059),
	L_DEF(060),
	L_DEF(061),
	L_DEF(062),
	L_DEF(063),
	L_DEF(064),
	L_DEF(065),
	L_DEF(066),
	L_DEF(067),
	L_DEF(068),
	L_DEF(069),
	L_DEF(070),
	L_DEF(071),
	L_DEF(072),
	L_DEF(073),
	L_DEF(074),
	L_DEF(075),
	L_DEF(076),
	L_DEF(077),
	L_DEF(078),
	L_DEF(079),
	L_DEF(080),
	L_DEF(081),
	L_DEF(082),
	L_DEF(083),
	L_DEF(084),
	L_DEF(085),
	L_DEF(086),
	L_DEF(087),
	L_DEF(088),
	L_DEF(089),
	L_DEF(090),
	L_DEF(091),
	L_DEF(092),
	L_DEF(093),
	L_DEF(094),
	L_DEF(095),
	L_DEF(096),
	L_DEF(097),
	L_DEF(098),
	L_DEF(099),
	L_DEF(100),	
	L_DEF(101),
	L_DEF(102),
	L_DEF(103),
	L_DEF(104),
	L_DEF(105),
	L_DEF(106),
	L_DEF(107),
	L_DEF(108),
	L_DEF(109),
	L_DEF(110),	
	L_DEF(111),
	L_DEF(112),
	L_DEF(113),
};

// 右轨迹数据
static const BACK_LINE back_lines_right[] = {
	R_DEF(001),
	R_DEF(002),
	R_DEF(003),
	R_DEF(004),
	R_DEF(005),
	R_DEF(006),
	R_DEF(007),
	R_DEF(008),
	R_DEF(009),
	R_DEF(010),
	R_DEF(011),
	R_DEF(012),
	R_DEF(013),
	R_DEF(014),
	R_DEF(015),
	R_DEF(016),
	R_DEF(017),
	R_DEF(018),
	R_DEF(019),
	R_DEF(020),
	R_DEF(021),
	R_DEF(022),
	R_DEF(023),
	R_DEF(024),
	R_DEF(025),
	R_DEF(026),
	R_DEF(027),
	R_DEF(028),
	R_DEF(029),
	R_DEF(030),
	R_DEF(031),
	R_DEF(032),
	R_DEF(033),
	R_DEF(034),
	R_DEF(035),
	R_DEF(036),
	R_DEF(037),
	R_DEF(038),
	R_DEF(039),
	R_DEF(040),
	R_DEF(041),
	R_DEF(042),
	R_DEF(043),
	R_DEF(044),
	R_DEF(045),
	R_DEF(046),
	R_DEF(047),
	R_DEF(048),
	R_DEF(049),
	R_DEF(050),
	R_DEF(051),
	R_DEF(052),
	R_DEF(053),
	R_DEF(054),
	R_DEF(055),
	R_DEF(056),
	R_DEF(057),
	R_DEF(058),
	R_DEF(059),
	R_DEF(060),
	R_DEF(061),
	R_DEF(062),
	R_DEF(063),
	R_DEF(064),
	R_DEF(065),
	R_DEF(066),
	R_DEF(067),
	R_DEF(068),
	R_DEF(069),
	R_DEF(070),
	R_DEF(071),
	R_DEF(072),
	R_DEF(073),
	R_DEF(074),
	R_DEF(075),
	R_DEF(076),
	R_DEF(077),
	R_DEF(078),
	R_DEF(079),
	R_DEF(080),
	R_DEF(081),
	R_DEF(082),
	R_DEF(083),
	R_DEF(084),
	R_DEF(085),
	R_DEF(086),
	R_DEF(087),
	R_DEF(088),
	R_DEF(089),
	R_DEF(090),
	R_DEF(091),
	R_DEF(092),
	R_DEF(093),
	R_DEF(094),
	R_DEF(095),
	R_DEF(096),
	R_DEF(097),
	R_DEF(098),
	R_DEF(099),
	R_DEF(100),
	R_DEF(101),
	R_DEF(102),
	R_DEF(103),
	R_DEF(104),
	R_DEF(105),
	R_DEF(106),
	R_DEF(107),
	R_DEF(108),
	R_DEF(109),
	R_DEF(110),
	R_DEF(111),
	R_DEF(112),
	R_DEF(113),
};

#define LEFT_PATH_ORG_X		(0.0f)
#define LEFT_PATH_ORG_Y		(0.0f)

#define RIGHT_PATH_ORG_X		(0.0f)
#define RIGHT_PATH_ORG_Y		(0.0f)

#define BACK_LINE_WIDTH		7.0f		// 红黄绿辅助线线宽


static VGPath back_line_path_0;		// 绿色标线
static VGPath back_line_path_1;		// 黄色标线
static VGPath back_line_path_2;		// 红色标线
static int vg_w, vg_h;		


// 绘制红黄绿辅助线及轨迹线
// path_line_index  轨迹线图片索引
static void do_paint_auxiliary_line (int path_line_index)
{
	VGfloat line_color[4];
	VGPaint colorPaint;

	if (path_line_index < 0 || path_line_index >= sizeof(back_lines_left) / sizeof(back_lines_left[0])
		|| path_line_index >= sizeof(back_lines_right) / sizeof(back_lines_right[0]))
		return;

	// 设置单位矩阵
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();

	// 创建一个纯色画笔对象
	colorPaint = vgCreatePaint();
	vgSetParameteri(colorPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);

	VGfloat  m[9];
	// 设置自顶向下(SVG坐标系)到自底向上(OpenVG)的坐标映射, 将SVG坐标转换为OpenVG坐标
	// 以下红黄绿辅助线及轨迹线均使用SVG元素绘制.
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgGetMatrix(m);
	m[4] = -1;
	m[7] = vg_h;
	vgLoadMatrix(m);

	// 绘制红黄绿辅助线 (以下路径使用的坐标为SVG坐标, 通过上面的转换矩阵m进行坐标系转换)
	
	// 绘制绿色标线
	// SVG代码
	// <path d="M568.5 199L499 148H223L152.5 199" stroke="#00FF00" stroke-width="7" stroke-linecap="round"/>
	// 创建绿色画笔
	line_color[0] = 0.00f;		// 绿色
	line_color[1] = 1.00f;
	line_color[2] = 0.00f;
	line_color[3] = 1.00f;
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, line_color);
	// 标记画笔对象colorPaint用于下面的路径描边操作
	vgSetPaint (colorPaint, VG_STROKE_PATH);
	// 设置笔宽度
	vgSetf (VG_STROKE_LINE_WIDTH, BACK_LINE_WIDTH);
	// 设置笔帽类型
	vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_ROUND);
	// 绘制绿色标线
	vgDrawPath (back_line_path_0, VG_STROKE_PATH);

	// 绘制黄色标线
	// SVG代码
	// <path d="M653.5 266.5L568.5 199H152.51L66.5 266.5" stroke="#FFFF00" stroke-width="7" stroke-linecap="round"/>
	// 创建黄色画笔
	line_color[0] = 1.00f;		// 黄色
	line_color[1] = 1.00f;
	line_color[2] = 0.00f;
	line_color[3] = 1.00f;
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, line_color);
	// 标记画笔对象colorPaint用于下面的路径描边操作
	vgSetPaint (colorPaint, VG_STROKE_PATH);
	// 设置笔宽度
	vgSetf (VG_STROKE_LINE_WIDTH, BACK_LINE_WIDTH);
	// 设置笔帽类型
	vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_ROUND);
	// 绘制黄色标线
	vgDrawPath (back_line_path_1, VG_STROKE_PATH);

	// 绘制红色标线
	// SVG代码
	// <path d="M713.5 316.5L653 266H67L3.5 319.5" stroke="#FF0000" stroke-width="7" stroke-linecap="round"/>
	// 创建红色画笔
	line_color[0] = 1.00f;		// 红色
	line_color[1] = 0.00f;
	line_color[2] = 0.00f;
	line_color[3] = 1.00f;
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, line_color);
	// 标记画笔对象colorPaint用于下面的路径描边操作
	vgSetPaint (colorPaint, VG_STROKE_PATH);
	// 设置笔宽度
	vgSetf (VG_STROKE_LINE_WIDTH, BACK_LINE_WIDTH);
	// 设置笔帽类型
	vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_ROUND);
	// 绘制红色标线
	vgDrawPath (back_line_path_2, VG_STROKE_PATH);


	// 绘制倒车轨迹
	// leftBackLine001.svg
	// <path d="M6.5 4C15 3.99996 41.6468 9.29998 43 30.4997C44.5 54 18 84 3 98.4997" stroke="#FFFF00" stroke-width="7"/>
	line_color[0] = 1.00f;		// 黄色
	line_color[1] = 1.00f;
	line_color[2] = 0.00f;
	line_color[3] = 1.00f;
	vgSetParameterfv (colorPaint, VG_PAINT_COLOR, 4, line_color);
	// 标记画笔对象colorPaint用于下面的路径填充操作
	vgSetPaint (colorPaint, VG_STROKE_PATH);
	// 设置笔宽度
	vgSetf (VG_STROKE_LINE_WIDTH, BACK_LINE_WIDTH);
	// 设置笔帽类型
	vgSetf (VG_STROKE_CAP_STYLE, VG_CAP_BUTT);	// VG_CAP_BUTT为缺省类型

	// 绘制左边的轨迹线
	// 移至坐标原点至左轨迹SVG相对于VG背景的偏移位置
	vgTranslate(LEFT_PATH_ORG_X, LEFT_PATH_ORG_Y);
	const BACK_LINE *back_line_left = back_lines_left + path_line_index;
	// 创建左轨迹路径
	VGPath path_line_path_left = vgCreatePath (VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	if (path_line_path_left != VG_INVALID_HANDLE)
	{
		// 以下VG命令个数 - 1 "back_line_left->cb_command - 1"的作用是去除最后的 VG_CLOSE_PATH 命令. SVG2OPENVG.exe工具转换时自动在所有路径命令上补全  VG_CLOSE_PATH
		vgAppendPathData (path_line_path_left, back_line_left->cb_command - 1, back_line_left->command, back_line_left->coordinate);
		// 绘制左轨迹
		vgDrawPath (path_line_path_left, VG_STROKE_PATH);
	}
	// 释放左轨迹路径资源
	vgDestroyPath (path_line_path_left);
	vgTranslate (-LEFT_PATH_ORG_X, -LEFT_PATH_ORG_Y);

	// 绘制右边的轨迹线
	// 移至坐标原点至右轨迹SVG相对于VG背景的偏移位置
	vgTranslate (RIGHT_PATH_ORG_X, RIGHT_PATH_ORG_Y);
	const BACK_LINE *back_line_right = back_lines_right + path_line_index;
	// 创建右轨迹路径
	VGPath path_line_path_right = vgCreatePath (VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	if (path_line_path_right != VG_INVALID_HANDLE)
	{
		// 以下VG命令个数 - 1 "back_line_right->cb_command - 1"的作用是去除最后的 VG_CLOSE_PATH 命令. SVG2OPENVG.exe工具转换时自动在所有路径命令上补全  VG_CLOSE_PATH
		vgAppendPathData (path_line_path_right, back_line_right->cb_command - 1, back_line_right->command, back_line_right->coordinate);
		vgDrawPath (path_line_path_right, VG_STROKE_PATH);
	}
	// 释放右轨迹路径资源
	vgDestroyPath (path_line_path_right);
	vgTranslate (-RIGHT_PATH_ORG_X, -RIGHT_PATH_ORG_Y);

	// 设置无效句柄用于路径描边, 将colorPaint的引用次数减1
	vgSetPaint (VG_INVALID_HANDLE, VG_STROKE_PATH);
	// 释放colorPaint资源
	vgDestroyPaint (colorPaint);

	vgFinish();
}

int reversing_auxiliary_line_draw (int index)
{
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	do_paint_auxiliary_line(index);

	return 0;
}

int reversing_auxiliary_line_init (int width, int height)
{
	vg_w = width;
	vg_h = height;

	(void)vg_w;
	(void)vg_h;
	
	// 创建红黄绿辅助线路径对象
	// 以下VG命令个数 - 1 "sizeof(bg_path_0_commands) / sizeof(VGubyte) - 1"的作用是去除最后的 VG_CLOSE_PATH 命令. SVG2OPENVG.exe工具转换时自动在所有路径命令上补全  VG_CLOSE_PATH
	back_line_path_0 = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	if(back_line_path_0 != VG_INVALID_HANDLE)
		vgAppendPathData(back_line_path_0, sizeof(bg_path_0_commands) / sizeof(VGubyte) - 1, bg_path_0_commands, bg_path_0_coordinates);

	back_line_path_1 = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	if (back_line_path_1 != VG_INVALID_HANDLE)
		vgAppendPathData(back_line_path_1, sizeof(bg_path_1_commands) / sizeof(VGubyte) - 1, bg_path_1_commands, bg_path_1_coordinates);

	back_line_path_2 = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	if (back_line_path_2 != VG_INVALID_HANDLE)
		vgAppendPathData(back_line_path_2, sizeof(bg_path_2_commands) / sizeof(VGubyte) - 1, bg_path_2_commands, bg_path_2_coordinates);

	return 0;
}

int reversing_auxiliary_line_exit (void)
{
	// 释放背景轨迹路径对象
	if (back_line_path_0 != VG_INVALID_HANDLE)
	{
		vgDestroyPath(back_line_path_0);
		back_line_path_0 = VG_INVALID_HANDLE;
	}
	if (back_line_path_1 != VG_INVALID_HANDLE)
	{
		vgDestroyPath(back_line_path_1);
		back_line_path_1 = VG_INVALID_HANDLE;
	}
	if (back_line_path_2 != VG_INVALID_HANDLE)
	{
		vgDestroyPath(back_line_path_2);
		back_line_path_2 = VG_INVALID_HANDLE;
	}

	return 0;
}



