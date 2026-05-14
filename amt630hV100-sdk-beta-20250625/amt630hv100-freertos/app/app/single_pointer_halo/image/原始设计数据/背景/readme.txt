直接点击PNG2VG.exe, 将与PNG2VG.exe在同一目录下的PNG文件转换为RGB565, ARGB8888, RGBA8888格式.

生成的格式转换文件保存在.\PNG2VG目录下 
ARGB8888格式保存在.\PNG2VG\ARGB888子目录下
RGB565格式保存在.\PNG2VG\RGB565子目录下
BGRA8888格式保存在.\PNG2VG\BGRA888子目录下

产生的图像数据文件命名规则定义 
1) 后缀名为转换后的格式名(RGB565,ARGB8888)
bg_RGB16.RGB565
halo_496x496.ARGB8888


QA
1) 如何将RGB24位位图转换到RGB565格式, 且尽量保留图像的细节?
考虑资源大小、GPU带宽及刷新帧率, 仪表demo使用RGB565格式。
所有的图片格式应按照下面的流程将其转换为适应RGB565显示的格式
为保证RGB24到RGB16转换过程颜色的精度, 使用ACDSee Pro 8里的Export功能（File菜单）将原始PNG或JPEG图片转换为15Bit BMP格式
然后再将15位 BMP格式无损转换为PNG或者JPEG格式, 用于后续的显示。

PNG图片转换到ICON框架支持的ARGB8888, RGB32, RGB565格式
 PNG2VG.exe
 将PNG2VG.exe与待转换的PNG图片放置在同一目录下，然后运行PNG2VG.exe, 将所有PNG图片转换为ARGB8888及RGB565格式， 可直接用于ICON框架的显示用途。

转换后的RGB565背景图(bg_RGB16.RGB565)
.\image\原始设计数据\背景\PNG2VG\RGB565\bg_RGB16.RGB565

转换后的ARGB8888光晕底图(halo_496x496.ARGB8888)
.\image\原始设计数据\背景\PNG2VG\ARGB8888\halo_496x496.ARGB8888







