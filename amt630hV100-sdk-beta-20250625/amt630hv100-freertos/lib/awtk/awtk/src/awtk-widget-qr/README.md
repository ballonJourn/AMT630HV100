# awtk-widget-qr

qr 控件。用于显示二维码控件。

![](docs/images/ui.jpg)

* 本项目使用 [libqrencode](https://github.com/fukuchi/libqrencode) 生成二维码。

* 本项目使用 [awtk-widget-generator](https://github.com/zlgopen/awtk-widget-generator) 生成代码框架。

## 准备

1. 获取 awtk 并编译

```
git clone https://github.com/zlgopen/awtk.git
cd awtk; scons; cd -
```

2. 获取 awtk-widget-qr 并编译
```
git clone https://github.com/zlgopen/awtk-widget-qr.git
cd awtk-widget-qr; scons
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

* 编译PC版本

```
scons
```

* 编译LINUX FB版本

```
scons LINUX_FB=true
```

> 完整编译选项请参考[编译选项](https://github.com/zlgopen/awtk-widget-generator/blob/master/docs/build_options.md)


3. 运行

```
./bin/demo
```

## 如何使用二维码控件

### 设置需要转成二维码的字符串

qr 控件 `value` 属性内容就是转成二维码的字符串内容。

在界面描述文件设置：

```xml
<window theme="main" name="main">
  <qr name="qr" x="c" y="m" w="50%" h="50%" value="https://github.com/zlgopen/awtk"/>
</window>
```

或在代码中设置：

```c
widget_t* win = window_open("main");
widget_t* qr = widget_lookup(win, "qr", TRUE);
qr_set_value(qr, "https://github.com/zlgopen/awtk");
```

按照上述例子设置完成后，用手机扫该二维码就会打开[ https://github.com/zlgopen/awtk ](https://github.com/zlgopen/awtk)网址。

### 设置二维码中的颜色与图片

添加图片资源到项目中：

![](./design/default/images/x1/message.png)

在样式文件中设置 qr 控件样式在对应状态设置 `fg_color` ，`bg_color`，`bg_image` 属性：

```xml
<qr>
  <style name="default">
    <!-- fg_color 为二维码颜色，bg_color 为二维码背景色 -->
    <normal fg_color="black" bg_color="white" bg_image="message"/>

    <!-- disable 状态用于模拟 被扫描 状态 -->
    <!-- 不设置 bg_image 属性，则该状态二维码中间没有图片 -->
    <disable fg_color="gray" bg_color="#f0f0f0"/>
  </style>
</qr>
```

从上述例子得到的效果为：

正常状态：

![](./docs/images/normal_style.png)

被扫描状态：

![](./docs/images/scanned_style.png)

**注意**：图片大小不可以大于二维码的 `45%` ，否则图片将会被裁剪：

图片资源：

![](./design/default/images/x3/message.png)

得到的二维码：

![](./docs/images/clip_image.png)

qr 控件的属性名及作用如下表所示：

| 属性名 | 作用                   |
| ------ | ---------------------- |
| value  | 要转换为二维码的字符串 |

## 文档

[完善自定义控件](https://github.com/zlgopen/awtk-widget-generator/blob/master/docs/improve_generated_widget.md)
