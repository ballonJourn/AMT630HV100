# awtk-widget-chart-view

chart_view图表控件，包含：曲线图、柱状图和饼图。源码位于src目录，示例位于demos目录。

- ##### 控件列表

|       控件类型       |               说明               |
| :------------------: | :------------------------------: |
|        x_axis        |             X轴控件              |
|        y_axis        |             Y轴控件              |
|       tooltip        |          提示信息框控件          |
|     line_series      |            曲线图控件            |
| line_series_colorful |          彩色曲线图控件          |
|      bar_series      |            柱状图控件            |
|  bar_series_minmax   | 柱状图控件（同时显示最大最小值） |
|      pie_slice       |             饼图控件             |

- ##### 运行示例效果截图

![](docs\images\曲线图.png)



![](docs\images\柱状图.png)



![](docs\images\饼图.png)

## 准备

1. 获取 awtk 并编译

```
git clone https://github.com/zlgopen/awtk.git
cd awtk; scons; cd -
```

## 运行

1. 生成示例代码的资源

```
python scripts/update_res.py all
```
> 也可以使用 Designer 打开项目，之后点击 “打包” 按钮进行生成；
> 如果资源发生修改，则需要重新生成资源。

如果 PIL 没有安装，执行上述脚本可能会出现如下错误：
```cmd
Traceback (most recent call last):
...
ModuleNotFoundError: No module named 'PIL'
```
请用 pip 安装：
```cmd
pip install Pillow
```

2. 编译

```
scons
```
> 注意：
> 编译前先确认 SConstruct 文件中的 awtk_root 是否为 awtk 所在目录，不是则修改。
> 默认使用动态库的形式，如果需要使用静态库，修改 SConstruct 文件中的 BUILD_SHARED = 'false' 即可。

3. 运行
```
./bin/demo
```

## 如何使用 chart_view 控件

以简单的曲线图为例子，在 xml 中创建曲线图

```xml
<chart_view name="chartview" x="0" y="0" w="400" h="200">
  <x_axis name="x_axis" axis_type="value" min="0" max="9" tick="{show:true}" split_line="{show:true}" label="{show:true}" data="[1,2,3,4,5,6,7,8,9,10]"/>
  <y_axis name="y_axis" axis_type="value" min="0" max="140" tick="{show:true}" split_line="{show:true}" label="{show:true}" data="[0,20,40,60,80,100,120,140]"/>
  <line_series name="line_series" capacity="10" value_animation="500" line="{smooth:true}" area="{show:true}" symbol="{show:true}" value="15,75,40,60,140,80,100,120,25,90" text="s1"/>
  <tooltip name="tooltip"/>
</chart_view>
```

#### axis 坐标轴控件

其中 x_axis 和 y_axis 为坐标轴控件，它们的父类都是 axis，可以通过修改属性来改变坐标轴的形态，axis 的属性名与作用如下表所示

| 属性名     | 作用                                                         |
| ---------- | ------------------------------------------------------------ |
| axis_type  | 坐标轴类型，可选项有 value,category,time                     |
| at         | 坐标轴位置，可选项有top,buttom,left,right，x轴缺省为buttom，y轴缺省为left |
| min        | 量程的最小值                                                 |
| max        | 量程的最大值                                                 |
| data       | 显示的刻度值                                                 |
| split_line | 分割线的参数，如"{show:true}"                                |
| tick       | 刻度线的参数，如"{show:true, align_with_label:true, inside:false}" |
| line       | 轴线的参数，如"{show:true,lengthen:20}"                      |
| label      | 刻度值的参数，如"{show:true,inside:false}"                   |
| title      | 标题的参数，如"{show:false}"                                 |
| time       | 时间的参数，如"{format:Y-M-D hh:mm:ss}"                      |
| offset     | 相对于初始位置的偏移（像素）                                 |

#### series 序列控件

line_series 和 bar_series 为序列控件，它们的父类都是 series 类，可以通过修改属性来改变曲线和柱形图的形态，series 类的属性名和作用如下表所示：

| 属性名          | 作用                                                        |
| --------------- | ----------------------------------------------------------- |
| fifo            | 序列 fifo，在 mvvm 中可以绑定一个 object 对象，实现数据更新 |
| offset          | 序列fifo（相对末尾）的偏移                                  |
| display_mode    | 显示模式，可选项有 push、cover，缺省为 push                 |
| value_animation | 序列值动画的持续时间，0表示不播放动画                       |
| value           | 以 "," 分割的一组序列值，不同类型的 series 格式略有不同     |
| capacity        | FIFO容量                                                    |

对于 line_series ，还有一些自身独有的属性，如下表所示，实例可以参考 demo 中以 line_series 开头的界面：

| 属性名      | 作用                                                    |
| ----------- | ------------------------------------------------------- |
| series_axis | 标示序列位置的轴的名称，为空默认为检索到的第一个 x_axis |
| value_axis  | 标示序列值的轴的名称，为空时默认为检索到的第一个 y_axis |
| line        | 序列曲线的参数，如"{show:true, smooth:true}"            |
| area        | 序列曲线与坐标轴围成的区域的参数，如"{show:true}"       |
| symbol      | 序列点的参数，如"{show:true, size:4}"                   |

对于 bar_series，它还有一些自身独有的属性，如下表所示，实例可以参考 demo 中以 bar_series 开头的界面：

| 属性名      | 作用                                                    |
| ----------- | ------------------------------------------------------- |
| series_axis | 标示序列位置的轴的名称，为空默认为检索到的第一个 x_axis |
| value_axis  | 标示序列值的轴的名称，为空时默认为检索到的第一个 y_axis |
| bar         | 柱条的参数，如"{overlap:true}"                          |

line_series 和 bar_series 的数据结构类型都为 float 类型，在 xml 中可以通过以下方式为 value 属性赋值

```xml
<line_series w="100" h="100" capacity="10" value_animation="500" line="{smooth:true}" area="{show:true}" symbol="{show:true}" value="15,75,40,60,140,80,100,120,25,90"/>
```

line_series_colorful 为 line_series 的子类，它的数据结构类型如下所示，在 xml 中对 value 属性可以用"色值,采样值,色值,采样值,……"的格式赋值，如果没有显示指定则沿用上一个采样点的色值，默认为黑色，实例可以参考 demo 中的 line_series_colorful 界面。

```c
typedef struct _series_data_colorful_t {
  float_t v;
  color_t c;
} series_data_colorful_t;
```

bar_series_minmax 为 bar_series 的子类，它的数据结构类型如下所示，在 xml 中对 value 属性可以用"最小值,最大值,最小值,最大值……"的格式赋值，实例可以参考 demo 中 bar_series_minmax 界面。

```c
typedef struct _series_data_minmax_t {
  float_t min;
  float_t max;
} series_data_minmax_t;
```

如何使用函数为序列控件赋值，请参考[chart_view 其他用法](./docs/chart_view_other_funciton.md)。

#### tooltip 控件

tooltip 控件为提示框控件，点击序列控件后会显示当前点击的坐标信息，它的属性名与作用如下表所示：

| 属性名 | 作用                                  |
| ------ | ------------------------------------- |
| line   | 标记线的参数，如"{show:true}"         |
| symbol | 标记点的参数，如"{show:true, size:3}" |
| tip    | 提示文本的参数，如"{show:true}"       |

#### pie_slice 控件

pie_slice 为进度圆环控件，该控件需要放置到容器中，它的属性名和作用如下表所示：

| 属性名                 | 作用                            |
| ---------------------- | ------------------------------- |
| value                  | 值，缺省为0                     |
| max                    | 最大值，缺省为100               |
| start_angle            | 起始角度（单位为度，缺省为-90） |
| inner_radius           | 环线的厚度，缺省为8             |
| unit                   | 单元，缺省无                    |
| counter_clock_wise     | 是否为逆时针方向，缺省为FALSE   |
| show_text              | 是否显示文本                    |
| is_exploded            | 是否扩展为扇形，缺省为FALSE     |
| explode_distancefactor | 扩展距离                        |
| x_to                   | 扩展距离后x坐标                 |
| y_to                   | 扩展距离后y坐标                 |
| press                  | 是否按下鼠标                    |
| is_semicircle          | 是否画拱形                      |

关于chart_view 控件的其他用法，请参考 [chart_view 其他用法](./docs/chart_view_other_funciton.md)。

