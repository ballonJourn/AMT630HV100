参考 https://www.w3cschool.cn/svg/svg-reference.html

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