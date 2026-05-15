参考 https://www.w3cschool.cn/svg/svg-reference.html

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

// size: 8.000000 x 76.000000
// 指针0的数据定义, 
static const VGubyte pointer_path_0_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};
// PT_OFF_Y表示将环形指针顶点向上移动的距离, 将其环形指针的顶部延伸到指针圆盘的外圆上.
// 可以将该值改为其他值测试其效果
static const VGfloat pointer_path_0_coordinates[] = {
	1.603790f + CIRCLE_0_X, 2.002140f  + CIRCLE_0_Y + PT_OFF_Y,
	7.496920f + CIRCLE_0_X, 75.243500f + CIRCLE_0_Y + PT_OFF_Y,
	0.000000f + CIRCLE_0_X, 75.030098f + CIRCLE_0_Y + PT_OFF_Y,
	1.603790f + CIRCLE_0_X, 2.002140f  + CIRCLE_0_Y + PT_OFF_Y
};

// 指针1的数据定义
static const VGubyte pointer_path_1_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};
static const VGfloat pointer_path_1_coordinates[] = {
	1.603790f + CIRCLE_1_X, 2.002140f  + CIRCLE_1_Y + PT_OFF_Y,
	7.496920f + CIRCLE_1_X, 75.243500f + CIRCLE_1_Y + PT_OFF_Y,
	0.000000f + CIRCLE_1_X, 75.030098f + CIRCLE_1_Y + PT_OFF_Y,
	1.603790f + CIRCLE_1_X, 2.002140f  + CIRCLE_1_Y + PT_OFF_Y
};