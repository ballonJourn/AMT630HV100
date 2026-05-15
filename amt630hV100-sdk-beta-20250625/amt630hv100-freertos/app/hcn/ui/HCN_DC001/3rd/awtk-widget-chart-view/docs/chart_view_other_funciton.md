# chart_view 控件的其他用法

## 改变控件的显示风格

chart_view 控件可以通过修改 style 来改变控件的显示风格，各控件的样式名称与对应的作用如下所示

#### line_series 控件

line_series 可以通过以下名称修改：

| 样式名                    | 作用                                     |
| ------------------------- | ---------------------------------------- |
| line_border_color         | 用于设置序列曲线的颜色                   |
| line_border_width         | 用于设置序列曲线的宽度                   |
| area_color                | 用于设置序列曲线与坐标轴围成的区域的颜色 |
| symbol_bg_image           | 用于设置序列点的背景图片                 |
| symbol_bg_image_draw_type | 用于设置序列点的背景图片的显示方式       |
| symbol_bg_color           | 用于设置序列点的背景颜色                 |
| symbol_border_color       | 用于设置序列点的边框颜色                 |
| symbol_border_width       | 用于设置序列点的边框宽度                 |
| symbol_round_radius       | 用于设置序列点的圆角                     |

#### line_series_colorful 控件

line_series_colorful 可以通过以下名称修改：

| 样式名                    | 作用                               |
| ------------------------- | ---------------------------------- |
| line_border_width         | 用于设置序列曲线的宽度             |
| symbol_bg_image           | 用于设置序列点的背景图片           |
| symbol_bg_image_draw_type | 用于设置序列点的背景图片的显示方式 |
| symbol_border_color       | 用于设置序列点的边框颜色           |
| symbol_border_width       | 用于设置序列点的边框宽度           |
| symbol_round_radius       | 用于设置序列点的圆角               |

#### bar_series 与 bar_series_minmax 控件

bar_series 和 bar_series_minmax 可以通过以下名称修改：

| 样式名                     | 作用                             |
| -------------------------- | -------------------------------- |
| fg_image                   | 用于设置柱条的填充图片           |
| fg_image_draw_type         | 用于设置柱条的填充图片的显示方式 |
| fg_color                   | 用于设置柱条的填充颜色           |
| border_color               | 用于设置柱条的边框颜色           |
| border_width               | 用于设置柱条的边框宽度           |
| round_radius               | 用于设置柱条的圆角               |
| margin_right、margin_right | 用于设置垂直柱条两侧的留白边距   |
| margin_top、margin_bottom  | 用于设置水平柱条两侧的留白边距   |

#### pie_slice 控件

pie_slice 可以通过以下名称修改：

| 样式名     | 作用                           |
| ---------- | ------------------------------ |
| fg_color   | 用于设置圆环的填充颜色         |
| text_color | 用于设置圆环进度的提示文本颜色 |

#### tooltip 控件

tooltip 可以通过以下名称修改：

| 样式名                    | 作用                                   |
| ------------------------- | -------------------------------------- |
| fg_color                  | 用于设置表示位置的直线的颜色           |
| fg_image                  | 用于设置表示位置的直线的图片           |
| fg_image_draw_type        | 用于设置表示位置的直线的图片的显示方式 |
| bg_color                  | 用于设置提示框的背景颜色               |
| border_color              | 用于设置提示框的边框颜色               |
| border_width              | 用于设置提示框的边框宽度               |
| round_radius              | 用于设置提示框的圆角                   |
| text_color                | 用于设置提示信息的文本颜色             |
| spacer                    | 用于设置提示信息的文本的行距           |
| font_name                 | 用于设置提示信息的文本字体             |
| font_size                 | 用于设置提示信息的文本字体大小         |
| symbol_bg_image           | 用于设置标记点的背景图片               |
| symbol_bg_image_draw_type | 用于设置标记点的背景图片的显示方式     |
| symbol_bg_color           | 用于设置标记点的背景颜色               |
| symbol_border_color       | 用于设置标记点的边框颜色               |
| symbol_border_width       | 用于设置标记点的边框宽度               |
| symbol_round_radius       | 用于设置标记点的圆角                   |
| margin、margin_top等      | 用于设置提示框内文本与边框之间的间距   |

#### axis 控件

x_axis 和 y_axis 可以通过以下名称修改：

| 样式名                     | 作用                           |
| -------------------------- | ------------------------------ |
| spacer                     | 用于设置刻度值与轴线之间的间距 |
| font_name                  | 用于设置刻度值的字体           |
| font_size                  | 用于设置刻度值的字体大小       |
| text_color                 | 用于设置刻度值的文本颜色       |
| fg_color                   | 用于设置轴线的颜色             |
| fg_image                   | 用于设置轴线的图片             |
| fg_image_draw_type         | 用于设置轴线的图片的显示方式   |
| tick_color                 | 用于设置刻度线的颜色           |
| tick_image                 | 用于设置刻度线的图片           |
| tick_image_draw_type       | 用于设置刻度线的图片的显示方式 |
| split_line_color           | 用于设置分割线的颜色           |
| split_line_image           | 用于设置分割线的图片           |
| split_line_image_draw_type | 用于设置分割线的图片的显示方式 |

## 如何操作 series 中的数据

series 类中提供了一些接口让用户可以获取其中的数据或者增删其中的数据，各接口的定义如下所示：

```c
/**
 * @method series_set
 * 设置指定位置的序列点。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {uint32_t} index 序列点在fifo中的位置。
 * @param {const void*} data 序列点数据。
 * @param {uint32_t} nr 序列点数量。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t series_set(widget_t* widget, uint32_t index, const void* data, uint32_t nr);

/**
 * @method series_rset
 * 设置指定位置（反向）的序列点。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {uint32_t} index 序列点在fifo中的位置。
 * @param {const void*} data 序列点数据。
 * @param {uint32_t} nr 序列点数量。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t series_rset(widget_t* widget, uint32_t index, const void* data, uint32_t nr);

/**
 * @method series_push
 * 在尾巴追加多个序列点。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {const void*} data 序列点数据。
 * @param {uint32_t} nr 序列点数量。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t series_push(widget_t* widget, const void* data, uint32_t nr);

/**
 * @method series_clear
 * 清除全部序列点。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t series_clear(widget_t* widget);

/**
 * @method series_at
 * 返回特定位置的序列点数据。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {uint32_t} index 序列点在fifo中的位置。
 *
 * @return {void*} 如果找到，返回特定位置的序列点数据，否则返回NULL。
 */
void* series_at(widget_t* widget, uint32_t index);
```

下面以 bar_series_minmax 控件为例子，简单展示一下如何添加数值，其他接口的用法也大致相同

```c
int size = 2;
void* buffer = TKMEM_CALLOC(size, sizeof(series_data_minmax_t));
series_data_minmax_t* b = (series_data_minmax_t*)buffer;
b[0].min = 10;
b[0].max = 30;
b[1].min = 40;
b[1].max = 60;
/* 此处的 series 为 bar_series_minmax 控件 */
series_push(series, buffer, size);
```

## 格式化 tooltip 显示的数值

可以通过 series_set_tooltip_format 函数设置一个格式化 tooltip 数值的函数，该函数的注释及简易例子如下所示，具体演示可以查看 demo 中的 tooltip_format 界面：

```c
typedef ret_t (*series_tooltip_format_t)(void* ctx, const void* data, wstr_t* str);

/**
 * @method series_set_tooltip_format
 * 设置提示信息格式化。
 * @annotation ["scriptable"]
 * @param {widget_t*} widget widget对象。
 * @param {series_tooltip_format_t} format 提示信息格式化回调。
 * @param {void*} ctx 格式化时的上下文。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t series_set_tooltip_format(widget_t* widget, series_tooltip_format_t format, void* ctx);

/* 示例 */
static ret_t tooltip_format(void* ctx, const void* data, wstr_t* str) {
  widget_t* series = WIDGET(ctx);
  /* 获取当前序列控件标题 */
  const wchar_t* title = series_get_title(series);
  /* 将 data 数据强转为 int 并初始化给 str*/
  wstr_from_int(str, (int)(*((float*)data)));
  /* 组合标题与数据 */
  if (title != NULL && wcslen(title) > 0) {
    wstr_insert(str, 0, L": ", wcslen(L": "));
    wstr_insert(str, 0, title, wcslen(title));
  }
}
series_set_tooltip_format(series, tooltip_format, series);
```

