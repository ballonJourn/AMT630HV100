1) 样本批处理获取字形数据
运行 run.bat 从字体文件"arial.ttf"获取字形数据, 结果保存在arial.c

运行 run_ARIALUNI.bat 从字体文件"ARIALUNI.ttf"获取字形数据, 结果保存在ARIALUNI.c

运行 run_courbd.bat 从字体文件"courbd.ttf"获取字形数据, 结果保存在ARIALUNI.c

2) 如何从字库文件(TTF)提取字形数据?
打开批处理文件run.bat, 包含2行代码
FontFaceExtract.exe arial.ttf font_utf8.txt
pause

FontFaceExtract.exe 字形提取程序
将该字形提取程序提取的字形数据用于商业用途的风险由使用者自行承担.
当前程序仅支持TTF字库格式, 文件后缀为ttf

arial.ttf     字库源文件
表示待抽取字形数据的源TTF字库文件.
可以指定其他字库源文件, 如ARIALUNI.TTF或者courbd.ttf

font_utf8.txt 字符集文件
表示待提取字形数据的字符集合, 每个字符均使用UTF8编码. 
字符集文件也可使用其他文件命名, 如my_charset.txt.
字符集文件使用 UltraEdit.exe 编辑, 包含待抽取文字字形数据的一个或多个UTF8字符编码.
例如抽取10个数字. 如下编辑
0123456789
保存文件时在格式中指定utf-8.
若文字字符个数超过80个, 请将文字分为多行处理, 每行字符个数不大于80个.

3) 如何将创建的字库C源码加入到工程中?
手工编辑vg_font.c, 
3.1) 包含字库的外部声明
VGErrorCode arialFontInit(void);
void arialFontDestroy(void);
extern Font arial_font;


3.2) 包含字库初始化及退出的代码

int vgFontInit (void)
{
	arialFontInit (); // 将从arial.ttf中提取的字形数据加入到项目
	//ARIALUNIFontInit ();  // 将从ARIALUNI.TTF中提取的字形数据加入到项目

	return 0;
}

int vgFontExit (void)
{
	arialFontDestroy ();
	//ARIALUNIFontDestroy ();
	return 0;
}

3.3) 手工修改double_pointer_demo.c或其他包含字体绘制的源文件,
1) 包含字库的外部声明
extern Font arial_font;

2) 修改以下函数的vgTextSize 及vgTextOut语句, 指定实际使用的字体, 如arial_font;.
static void DrawSpeed(char *speed, int font_w, int font_h, int centre_x, int centre_y, unsigned int color)

如将以下代码中的my_font改为arial_font或者courbd_font或者ARIALUNI_font
vgTextSize (&my_font, speed, &speed_scale_x, &speed_scale_y);
vgTextOut(&my_font, speed, VG_FILL_PATH);


