# AMT630HV100 dc002 分支 — AWTK 依赖全量审计报告

> 项目: `github.com/ballonJourn/AMT630HV100`  分支: `dc002`
> 审计日期: 2026-05-20
> 目标: 评估将 AWTK GUI 框架替换为 LVGL 的可行性与影响范围

---

## 一、总体统计

| 指标 | 数值 |
|------|------|
| AWTK 库源文件数 (lib/awtk/) | 3,170 个 .c/.h/.cpp |
| AWTK 预编译静态库 | awtk_rgb565.a (33MB) + nanovg.a (1.2MB) |
| HCN UI层 AWTK 依赖源文件 (F类) | 108 个 .c/.h |
| HCN 第三方 AWTK 控件 (G类) | 66 个 .c/.h |
| AWTK XML UI 布局文件 (H类) | 8 个 .xml |
| BSP/驱动层 AWTK 条件编译点 | 21 处 |
| AWTK API 在 UI 层调用总计 | 661+ 处 (详见下方) |

### AWTK API 调用分布 (HCN_DC001/src/ 内)

| API 类别 | 调用次数 |
|----------|---------|
| `widget_t` 类型引用 | 142 |
| `ret_t` 返回类型 | 297 |
| `widget_lookup()` 控件查找 | 38 |
| `widget_set_text()` 文本设置 | 35 |
| `widget_on()` / `EVT_*` 事件绑定 | 36 |
| `navigator_to()` / `navigator_replace()` 页面路由 | 24 |
| `window_manager()` 窗口管理 | 19 |
| `widget_animator*` 动画控制 | 16 |
| `locale_info` / `tr_text` 国际化 | 15 |
| `widget_set_visible()` 可见性 | 10 |
| `timer_add()` 定时器 | 10 |
| `idle_queue()` 跨线程投递 | 2 |
| 仪表专用控件 (image_value/progress_circle/gauge_pointer/slide_menu) | 17 |

---

## 二、全量文件清单 (按影响层级分类)

### 【A类】构建/链接配置 (IAR 项目文件)

影响: 需要全面重构 IAR 工程配置

```
proj/awtk.ewp                      ← AWTK 专用 IAR 工程 (977个源文件引用)
proj/awtk.ewd                      ← AWTK 调试配置
proj/awtk.ewt                      ← AWTK 工程模板
proj/amt630hv100_awtk.icf           ← AWTK 链接脚本 (内存布局)
```

对比参照:
```
proj/lvgl.ewp                      ← LVGL 专用 IAR 工程
proj/lvgl.ewd                      ← LVGL 调试配置
proj/amt630hv100_lvgl.icf           ← LVGL 链接脚本
```

### 【B类】系统入口/调度层

影响: 条件编译开关，决定整个系统走哪条 GUI 路径

```
main.c                              ← 第68行: #if defined(AWTK) → mainCREATE_AWTK_DEMO
                                       第74行: extern int main_awtk(void)
                                       第185行: main_awtk() 调用入口

app/main_awtk.c                     ← 1022行, AWTK 主线程, 包含:
                                       - lv_indev_data_t 兼容结构定义 (第143-170行)
                                       - SendTouchInputEvent() dummy函数
                                       - SendKeypadInputEventFromISR() → send_keypad_event_isr
                                       - WiFi/BT/CarLink 初始化
                                       - hcn_mw_init() 中间件初始化
                                       - ReadRomFile() 资源读取
                                       - gui_app_start() AWTK GUI 启动
                                       - xTaskCreate("awtk", 32768 words 栈)

FreeRTOSConfig.h                     ← 第58行: #elif defined(AWTK) → configTOTAL_HEAP_SIZE
                                       第83行: #ifdef AWTK → 堆大小配置
```

### 【C类】BSP/驱动层 (含 `#ifdef AWTK` 条件编译)

影响: 触摸屏/LCD/按键驱动需要适配不同的输入事件投递方式

```
ArkmicroFiles/libcpu-amt630hv100/source/lcd.c
    第104行: #if defined(AWTK) || LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
    第144行: #if defined(AWTK) && defined(WITH_VGCANVAS)
    第255行: #if defined(AWTK)
    第958行: #ifdef AWTK
    第1073行: #if defined(AWTK) && defined(WITH_VGCANVAS)
    第1172行: #if defined(AWTK) && defined(WITH_VGCANVAS)

ArkmicroFiles/libboard-amt630hv100/include/board.h
    第67行: #elif defined(AWTK) → OSD_WIDTH/OSD_HEIGHT 定义

ArkmicroFiles/libboard-amt630hv100/source/touch.c
    第10行: #if !defined(VG_ONLY) && !defined(AWTK)

ArkmicroFiles/libboard-amt630hv100/source/touchscreen/gt657x.c
    第11行: #if !defined(VG_ONLY) && !defined(AWTK)
    第143行: #ifdef AWTK → 触摸事件投递走 AWTK 路径
    第331行: #ifdef AWTK
    第376行: #ifdef AWTK

ArkmicroFiles/libboard-amt630hv100/source/touchscreen/ft6336.c
    第70行: #if !defined(VG_ONLY) && !defined(AWTK)
    第197行: #ifdef AWTK
    第237行: #ifdef AWTK
    第259行: #ifdef AWTK

ArkmicroFiles/libboard-amt630hv100/source/touchscreen/gt9xx.c
    第11行: #if !defined(VG_ONLY) && !defined(AWTK)
    第138行: #ifdef AWTK
    第315行: #ifdef AWTK
    第359行: #ifdef AWTK

ArkmicroFiles/libboard-amt630hv100/source/keypad.c
    第10行: #if !defined(VG_ONLY) && !defined(AWTK)

lib/vg_driver/vg_driver_wrapper.c
    第10行: #ifdef AWTK
    第13行: #if !defined(VG_ONLY) && !defined(AWTK)
    第201行: #elif defined(AWTK)

lib/vg_driver/vg_driver.h
    (AWTK 相关类型引用)
```

### 【D类】CarLink/WiFi 集成层

影响: 手机互联回调需要适配不同 UI 框架的刷新机制

```
app/carlink/EC/src/carlink_ec.c
    第62行: #ifdef AWTK
    第803行: #ifdef AWTK

app/carlink/EC-orig/src/carlink_ec.c
    第749行: #ifdef AWTK
```

### 【E类】LVGL/VG 互斥守卫文件 (含 `!defined(AWTK)`)

影响: 这些文件在 AWTK 模式下不编译，切换后需要激活

```
app/main_lvgl.c                      ← 1614行, LVGL主线程
    第7行: #if !defined(VG_ONLY) && !defined(AWTK)

app/xinbas/xinbas_demo.c
    第2行: #if !defined(VG_ONLY) && !defined(AWTK)

app/myFont24.c
    第11行: #if !defined(VG_ONLY) && !defined(AWTK)

app/myFont26.c
    第11行: #if !defined(VG_ONLY) && !defined(AWTK)

app/pointer_halo/pointer_halo.c
    第15行: #if !defined(VG_ONLY) && !defined(AWTK)

app/pointer_halo/vg_font.c
    第7行: #if !defined(VG_ONLY) && !defined(AWTK)
```

### 【F类】HCN 应用 UI 层 (全量 AWTK 深度依赖) — 108 个文件

这是迁移的核心工作量所在。每个文件 `#include "awtk.h"` 并直接使用 AWTK widget tree API。

#### F1: 应用入口 (2 文件)
```
app/hcn/ui/HCN_DC001/src/app_main.c        ← #include "awtk_main.inc", gui_app_start 入口
app/hcn/ui/HCN_DC001/src/application.c      ← custom_widgets_register(), navigator_to(APP_START_PAGE)
```

#### F2: 通用导航/路由 (2 文件)
```
app/hcn/ui/HCN_DC001/src/common/navigator.c   ← 页面导航封装
app/hcn/ui/HCN_DC001/src/common/navigator.h
```

#### F3: 业务逻辑层 (14 文件)
```
app/hcn/ui/HCN_DC001/src/logic/bluetooth_logic.c
app/hcn/ui/HCN_DC001/src/logic/buletooth_logic.h
app/hcn/ui/HCN_DC001/src/logic/hcn_global.c
app/hcn/ui/HCN_DC001/src/logic/hcn_global.h
app/hcn/ui/HCN_DC001/src/logic/hcn_logic.c
app/hcn/ui/HCN_DC001/src/logic/hcn_logic.h
app/hcn/ui/HCN_DC001/src/logic/hcn_selfcheck.c
app/hcn/ui/HCN_DC001/src/logic/hcn_selfcheck.h
app/hcn/ui/HCN_DC001/src/logic/mileage_calc.c
app/hcn/ui/HCN_DC001/src/logic/mileage_calc.h
app/hcn/ui/HCN_DC001/src/logic/navigation_view_logic.c
app/hcn/ui/HCN_DC001/src/logic/navigation_view_logic.h
app/hcn/ui/HCN_DC001/src/logic/signal_view_logic.c
app/hcn/ui/HCN_DC001/src/logic/signal_view_logic.h
app/hcn/ui/HCN_DC001/src/logic/speed_view_logic.c
app/hcn/ui/HCN_DC001/src/logic/speed_view_logic.h
```

#### F4: 页面初始化 (5 文件)
```
app/hcn/ui/HCN_DC001/src/pages/component.c     ← 通用组件初始化, widget_foreach
app/hcn/ui/HCN_DC001/src/pages/home_page.c     ← 主页初始化
app/hcn/ui/HCN_DC001/src/pages/device_page.c   ← 设备页初始化
app/hcn/ui/HCN_DC001/src/pages/link_page.c     ← 互联页初始化
app/hcn/ui/HCN_DC001/src/pages/update_page.c   ← 升级页初始化
```

#### F5: 数据代理层 (14 文件)
```
app/hcn/ui/HCN_DC001/src/proxy/bluetooth_data.c
app/hcn/ui/HCN_DC001/src/proxy/bluetooth_data.h
app/hcn/ui/HCN_DC001/src/proxy/mirror_data.c
app/hcn/ui/HCN_DC001/src/proxy/mirror_data.h
app/hcn/ui/HCN_DC001/src/proxy/vehicle_argument.c
app/hcn/ui/HCN_DC001/src/proxy/vehicle_argument.h
app/hcn/ui/HCN_DC001/src/proxy/vehicle_data.c
app/hcn/ui/HCN_DC001/src/proxy/vehicle_data.h
app/hcn/ui/HCN_DC001/src/proxy/vehicle_mile.c
app/hcn/ui/HCN_DC001/src/proxy/vehicle_mile.h
app/hcn/ui/HCN_DC001/src/proxy/vehicle_ota.c
app/hcn/ui/HCN_DC001/src/proxy/vehicle_ota.h
app/hcn/ui/HCN_DC001/src/proxy/vehicle_time.c
app/hcn/ui/HCN_DC001/src/proxy/vehicle_time.h
```

#### F6: 视图管理器 (2 文件)
```
app/hcn/ui/HCN_DC001/src/view/view_manager.c   ← 核心! 页面切换/按键分发/idle_queue跨线程
app/hcn/ui/HCN_DC001/src/view/view_manager.h
```

#### F7: 主页视图组 (30 文件)
```
app/hcn/ui/HCN_DC001/src/view/home_view/common.h
app/hcn/ui/HCN_DC001/src/view/home_view/home_view_interface.h
app/hcn/ui/HCN_DC001/src/view/home_view/home_page_key.c       ← 主页按键处理
app/hcn/ui/HCN_DC001/src/view/home_view/home_page_key.h
app/hcn/ui/HCN_DC001/src/view/home_view/speed_view.c          ← 速度表盘 (gauge_pointer, progress_circle)
app/hcn/ui/HCN_DC001/src/view/home_view/speed_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/power_view.c          ← 功率显示 (slider, image_value)
app/hcn/ui/HCN_DC001/src/view/home_view/power_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/dock_view.c           ← 侧边栏 (slide_view切换)
app/hcn/ui/HCN_DC001/src/view/home_view/dock_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/clock_view.c          ← 时钟显示
app/hcn/ui/HCN_DC001/src/view/home_view/clock_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/mileage_view.c        ← 里程显示 (TRIP/ODO)
app/hcn/ui/HCN_DC001/src/view/home_view/mileage_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/electrical_view.c     ← 电量显示 (progress_bar, animation)
app/hcn/ui/HCN_DC001/src/view/home_view/electrical_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/signal_view.c         ← 信号/指示灯 (GPS/BT/ABS/ECU/TCS等)
app/hcn/ui/HCN_DC001/src/view/home_view/signal_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/info_view.c           ← 骑行信息 (时间/距离)
app/hcn/ui/HCN_DC001/src/view/home_view/info_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/navigation_view.c     ← 导航显示
app/hcn/ui/HCN_DC001/src/view/home_view/navigation_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/music_view.c          ← 音乐播放 (hscroll_label)
app/hcn/ui/HCN_DC001/src/view/home_view/music_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/dock_music_ex_view.c  ← 音乐展开视图
app/hcn/ui/HCN_DC001/src/view/home_view/dock_music_ex_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/phone_view.c          ← 蓝牙电话
app/hcn/ui/HCN_DC001/src/view/home_view/phone_view.h
app/hcn/ui/HCN_DC001/src/view/home_view/animation_ctrl.c      ← 仪表盘动画控制 (timer_add)
app/hcn/ui/HCN_DC001/src/view/home_view/animation_ctrl.h
app/hcn/ui/HCN_DC001/src/view/home_view/music_page_key.c      ← 音乐页按键
app/hcn/ui/HCN_DC001/src/view/home_view/music_page_key.h
```

#### F8: 设置视图组 (20 文件)
```
app/hcn/ui/HCN_DC001/src/view/set_view/set_view_interface.h
app/hcn/ui/HCN_DC001/src/view/set_view/set_page_key.c         ← 设置页按键
app/hcn/ui/HCN_DC001/src/view/set_view/set_page_key.h
app/hcn/ui/HCN_DC001/src/view/set_view/setting_menu.c         ← 设置菜单 (scroll_view)
app/hcn/ui/HCN_DC001/src/view/set_view/setting_menu.h
app/hcn/ui/HCN_DC001/src/view/set_view/brightness.c           ← 亮度设置
app/hcn/ui/HCN_DC001/src/view/set_view/brightness.h
app/hcn/ui/HCN_DC001/src/view/set_view/bt_connect.c           ← 蓝牙连接
app/hcn/ui/HCN_DC001/src/view/set_view/bt_connect.h
app/hcn/ui/HCN_DC001/src/view/set_view/cycling_energy.c       ← 骑行能耗 (chart_view!)
app/hcn/ui/HCN_DC001/src/view/set_view/cycling_energy.h
app/hcn/ui/HCN_DC001/src/view/set_view/device.c               ← 设备信息
app/hcn/ui/HCN_DC001/src/view/set_view/device.h
app/hcn/ui/HCN_DC001/src/view/set_view/display.c              ← 日/夜模式切换
app/hcn/ui/HCN_DC001/src/view/set_view/display.h
app/hcn/ui/HCN_DC001/src/view/set_view/language.c             ← 语言切换 (locale_info)
app/hcn/ui/HCN_DC001/src/view/set_view/language.h
app/hcn/ui/HCN_DC001/src/view/set_view/setting_clock.c        ← 时间设置 (timer_add闪烁)
app/hcn/ui/HCN_DC001/src/view/set_view/setting_clock.h
app/hcn/ui/HCN_DC001/src/view/set_view/unit.c                 ← 单位切换 (km/mile)
app/hcn/ui/HCN_DC001/src/view/set_view/unit.h
```

#### F9: 互联视图组 (6 文件)
```
app/hcn/ui/HCN_DC001/src/view/link_view/link_view.c           ← QR码显示 (qr widget)
app/hcn/ui/HCN_DC001/src/view/link_view/link_view.h
app/hcn/ui/HCN_DC001/src/view/link_view/link_view_logic.c     ← timer_add 50ms刷新
app/hcn/ui/HCN_DC001/src/view/link_view/link_view_logic.h
app/hcn/ui/HCN_DC001/src/view/link_view/link_page_key.c
app/hcn/ui/HCN_DC001/src/view/link_view/link_page_key.h
```

#### F10: 升级视图组 (4 文件)
```
app/hcn/ui/HCN_DC001/src/view/update_view/update_view.c
app/hcn/ui/HCN_DC001/src/view/update_view/update_view.h
app/hcn/ui/HCN_DC001/src/view/update_view/update_logic.c      ← timer_add 100ms刷新
app/hcn/ui/HCN_DC001/src/view/update_view/update_logic.h
```

#### F11: 设备视图组 (4 文件)
```
app/hcn/ui/HCN_DC001/src/view/device_view/device_view.c
app/hcn/ui/HCN_DC001/src/view/device_view/device_view.h
app/hcn/ui/HCN_DC001/src/view/device_view/device_logic.c      ← timer_add 100ms刷新
app/hcn/ui/HCN_DC001/src/view/device_view/device_logic.h
```

### 【G类】HCN 第三方 AWTK 控件 — 66 个文件

这些控件 100% 构建在 AWTK widget 体系上，无法直接在 LVGL 中使用。

#### G1: awtk-widget-chart-view (图表控件, 用于骑行能耗)
```
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view_register.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view_register.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_data.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_fifo.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_fifo.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_fifo_default.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_fifo_default.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_fifo_event.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/base/series_fifo_event.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/axis.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/axis.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/axis_p.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/axis_p.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/axis_types.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/bar_series.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/bar_series.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/bar_series_minmax.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/bar_series_minmax.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/chart_animator.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/chart_animator.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/chart_utils.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/chart_utils.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/chart_view.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/chart_view.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/line_series.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/line_series.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/line_series_colorful.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/line_series_colorful.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/series.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/series.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/series_p.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/series_p.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/series_types.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/tooltip.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/tooltip.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/tooltip_types.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/x_axis.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/x_axis.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/y_axis.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/chart_view/y_axis.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/pie_slice/pie_slice.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-chart-view/src/pie_slice/pie_slice.h
```

#### G2: awtk-widget-qr (QR 码控件, 用于手机互联)
```
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr_register.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr_register.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qr.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qr.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrencode.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrencode.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrencode_inner.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrinput.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrinput.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrspec.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/qrspec.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/bitstream.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/bitstream.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/config.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/mask.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/mask.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/mmask.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/mmask.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/mqrspec.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/mqrspec.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/rsecc.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/rsecc.h
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/split.c
app/hcn/ui/HCN_DC001/3rd/awtk-widget-qr/src/qr/split.h
```

### 【H类】AWTK XML UI 设计资源 — 8 个文件

这些是 AWTK Designer 生成的 XML 声明式布局，LVGL 中不存在等价格式，需要全部转写为 C 代码。

```
app/hcn/ui/HCN_DC001/design/default/ui/home_page.xml      ← 主页 (仪表盘/速度/电量/导航/音乐/设置)
app/hcn/ui/HCN_DC001/design/default/ui/device_page.xml    ← 设备页
app/hcn/ui/HCN_DC001/design/default/ui/link_page.xml      ← 互联页
app/hcn/ui/HCN_DC001/design/default/ui/update_page.xml    ← 升级页
app/hcn/ui/HCN_DC001/design/default/ui/component.xml      ← 公共组件

app/hcn/ui/HCN_DC001/design/default/strings/strings.xml   ← 多语言字符串表 (中文/英文)
app/hcn/ui/HCN_DC001/design/default/styles/default.xml    ← 日间主题样式
app/hcn/ui/HCN_DC001/design/night/styles/default.xml      ← 夜间主题样式
```

### 【I类】AWTK 项目配置 / 二进制资源

```
app/hcn/ui/HCN_DC001/project.json    ← AWTK Designer 工程配置 (1024x600, BGR565, 依赖 awtk>=2210)
app/hcn/ui/HCN_DC001/res/rom.bin     ← AWTK 打包资源文件 (图片/字体/UI/样式 all-in-one)
app/hcn/ui/HCN_DC001/res/rommaker.exe ← 资源打包工具
```

### 【J类】AWTK 框架库本体 (仅计入口级文件, 完整库 3170 个文件)

```
lib/awtk/awtk/src/awtk.h                    ← 总入口头文件
lib/awtk/awtk/src/awtk_global.c             ← tk_init() / tk_run() / tk_exit()
lib/awtk/awtk/src/awtk_config.h             ← 编译配置
lib/awtk/awtk/src/platforms/freertos/        ← FreeRTOS 平台适配 (mutex/semaphore/thread)
lib/awtk/awtk/src/platforms/ark/             ← AMT630 芯片专用适配 (g2d/fs/platform)
lib/awtk/awtk/3rd/xm/src/main_loop_xm.c     ← AMT630 主循环适配
lib/awtk/awtk/3rd/xm/src/native_window_xm.c ← AMT630 原生窗口适配
lib/awtk/awtk/3rd/xm/src/vgcanvas_nanovg_vg.c ← VG 硬件加速画布
lib/awtk/awtk_rgb565.a                       ← 预编译库 (33MB, RGB565模式)
lib/awtk/nanovg.a                            ← NanoVG 向量图形库 (1.2MB)
```

---

## 三、关键风险矩阵

| 风险项 | 严重等级 | 说明 |
|--------|---------|------|
| 速度表盘精度丢失 | **致命** | gauge_pointer 角度映射 -135°~+135° 需精确重写 |
| 跨线程竞态 | **致命** | idle_queue → lv_async_call 语义差异导致数据竞争 |
| 内存溢出 | **高** | AWTK栈32768 vs LVGL栈2048, 链接脚本完全不同 |
| 触摸屏失灵 | **高** | BSP层3种触屏驱动均有 #ifdef AWTK 分支 |
| QR码功能丧失 | **中** | awtk-widget-qr 需替换为 LVGL 兼容库 |
| 图表功能丧失 | **中** | chart_view 需用 lv_chart 重写 |
| 主题切换失效 | **中** | AWTK 3主题系统 vs LVGL 主题机制完全不同 |
| 多语言失效 | **中** | strings.xml + tr_text → LVGL i18n 方案 |
| 动画效果退化 | **中** | XML内嵌动画 → lv_anim_t 手动编码 |

---

## 四、38 阶段里程碑计划 (M001-M038)

详见前次分析（上一轮对话）中的完整 Phase 1~10 计划。

---

## 五、结论

涉及修改/替换的文件总计: **A类4 + B类3 + C类9 + D类2 + E类6 + F类108 + G类66 + H类8 + I类3 = 209 个文件**, 外加 AWTK 库本体 3170 个文件的移除和 LVGL 库的引入。

这不是"设置"AWTK 为 LVGL 的简单操作，而是一次涉及 BSP→中间件→UI→资源 全栈的架构级重构。
