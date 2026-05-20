# AMT630HV100 dc002 — AWTK→LVGL 迁移 38 里程碑执行计划

> 项目: `github.com/ballonJourn/AMT630HV100`  分支: `dc002` → 新分支: `dc002-lvgl`
> 基于 commit: `0eaf219b` (AMT630HV100_AWTK_Audit_Report.md)
> 目标: 将整个 GUI 框架从 AWTK 替换为 LVGL 7.11.0 (已内置于 lib/LittlevGL/)
> LVGL 线程栈: 2048 words | AWTK 线程栈: 32768 words

---

## 总体架构对照

| 层级 | AWTK 当前 | LVGL 目标 | 迁移难度 |
|------|-----------|-----------|---------|
| 构建系统 | proj/awtk.ewp + awtk.icf | proj/lvgl.ewp + lvgl.icf | ★★☆ |
| 系统入口 | main_awtk.c (1022行) | 新 main_hcn_lvgl.c | ★★★ |
| FreeRTOS配置 | #ifdef AWTK → 23.5MB heap | 移除AWTK宏, 适配LVGL heap | ★★☆ |
| BSP/LCD | lcd.c 6处 #ifdef AWTK | 统一为LVGL路径(OSD_WIDTH/HEIGHT) | ★★★ |
| 触摸屏驱动 | XM_TpEventProc → AWTK事件 | lv_indev_data_t → Queue | ★★★ |
| 按键驱动 | send_keypad_event_isr → idle_queue | lv_indev_data_t → Queue → lv_group | ★★★ |
| CarLink集成 | set_qr_text_buf(AWTK路径) | 保留函数, 移到新main文件 | ★★☆ |
| UI公共头 | common.h → #include "awtk.h" | common.h → #include "lvgl.h" + 兼容层 | ★★★★ |
| 页面导航 | navigator.c → window_open_and_close | lv_scr_load_anim / screen管理器 | ★★★★ |
| 控件查找 | widget_lookup(name, recursive) | 自建name→obj注册表 | ★★★★ |
| 视图管理器 | view_manager.c 337行 | 重写: lv_group + key dispatch | ★★★★ |
| 速度表盘 | gauge_pointer + progress_circle + image_value | lv_meter + lv_img + lv_anim | ★★★★★ |
| QR码 | awtk-widget-qr (24文件) | qrencode库 + lv_canvas | ★★★ |
| 图表 | awtk-widget-chart-view (42文件) | lv_chart | ★★★ |
| XML布局 | 8个.xml + AWTK Designer | 全部转写为C代码 lv_obj_create() | ★★★★★ |
| 资源系统 | rom.bin + rommaker.exe | LVGL lv_img_dsc_t + lv_font | ★★★★ |
| 主题/样式 | styles/default.xml + night/ | lv_theme + lv_style | ★★★ |
| 国际化 | strings.xml + locale_info | 自建i18n表(与proxy层无关) | ★★☆ |
| 动画 | widget_animator* + XML内嵌animation | lv_anim_t | ★★★ |

---

## 38 阶段里程碑 (M001-M038)

### Phase 1: 基础设施 (M001-M005) — 第1位Claude

**M001 - 创建LVGL兼容层头文件**
- 新建: `app/hcn/ui/HCN_DC001/src/lvgl_compat/awtk_to_lvgl.h`
- 定义所有AWTK类型→LVGL类型的typedef映射:
  - `widget_t` → `lv_obj_t`
  - `ret_t` → `lv_res_t` + 枚举常量映射 (RET_OK→LV_RES_OK, RET_FAIL→LV_RES_INV, RET_REPEAT等)
  - `event_t` → `lv_event_t` 回调签名适配
  - `key_event_t` → lv_indev_data_t 按键映射
  - `bool_t` → `bool`
  - `timer_info_t` → `lv_task_t`
  - `idle_info_t` → `lv_async_cb_t` 参数
- 定义兼容宏:
  - `tk_min()` → `LV_MIN()` (若不存在则自定义)
  - `tk_max()` → `LV_MAX()`
  - `tk_str_eq()` → `strcmp()==0`
  - `tk_snprintf()` → `lv_snprintf()` 或 `snprintf()`
  - `TK_KEY_w/s/a/d` → `LV_KEY_*` 或自定义keycode
  - `EVT_KEY_DOWN` → `LV_EVENT_KEY`
  - `EVT_KEY_LONG_PRESS` → 自定义长按检测
  - `time_now_s()` → `lv_tick_get()/1000` 或 FreeRTOS `xTaskGetTickCount()`
- 文件位置: `app/hcn/ui/HCN_DC001/src/lvgl_compat/awtk_to_lvgl.h`

**M002 - 创建控件注册表(替代widget_lookup)**
- 新建: `app/hcn/ui/HCN_DC001/src/lvgl_compat/widget_registry.c/.h`
- AWTK的`widget_lookup(parent, name, recursive)`在LVGL中不存在
- 实现: 静态哈希表/数组 name→lv_obj_t* 映射
- API:
  - `void widget_reg_add(const char* name, lv_obj_t* obj)`
  - `lv_obj_t* widget_reg_find(const char* name)`
  - `void widget_reg_clear(void)`
- 容量: 预分配256条(当前UI约200个命名控件)
- 文件位置: `app/hcn/ui/HCN_DC001/src/lvgl_compat/widget_registry.c/.h`

**M003 - 创建页面管理器(替代AWTK navigator)**
- 新建: `app/hcn/ui/HCN_DC001/src/lvgl_compat/screen_manager.c/.h`
- 替代: `navigator.c` 中的 `navigator_to()`, `navigator_replace()`, `navigator_switch_to()`, `navigator_back()`
- 实现:
  - 屏幕栈 (最大深度4: home→link/device/update)
  - `screen_mgr_to(const char* name)` → 创建新screen并load
  - `screen_mgr_replace(const char* name)` → load并删除前一个
  - `screen_mgr_back()` → 回退栈
  - `screen_mgr_get_top_name()` → 返回当前screen名
- 每个screen的init回调通过注册表关联
- 文件位置: `app/hcn/ui/HCN_DC001/src/lvgl_compat/screen_manager.c/.h`

**M004 - 创建定时器/异步兼容层(替代timer_add和idle_queue)**
- 新建: `app/hcn/ui/HCN_DC001/src/lvgl_compat/timer_compat.c/.h`
- `timer_add(cb, ctx, interval)` → `lv_task_create(cb_wrapper, interval, LV_TASK_PRIO_MID, ctx)`
- `timer_remove(id)` → `lv_task_del(task)`
- `timer_find(id)` → 在内部数组查找
- `idle_queue(cb, ctx)` → `lv_async_call(cb_wrapper, ctx)`
  - **关键风险**: AWTK `idle_queue`是线程安全的(从ISR/其他线程投递到GUI线程)
  - LVGL `lv_async_call` 在v7中不保证线程安全
  - 解决: 使用FreeRTOS Queue作为中间层, 在lv_task中轮询
- `RET_REPEAT` / `RET_REMOVE` → 通过wrapper返回值控制lv_task是否继续
- 文件位置: `app/hcn/ui/HCN_DC001/src/lvgl_compat/timer_compat.c/.h`

**M005 - 创建动画兼容层(替代widget_animator)**
- 新建: `app/hcn/ui/HCN_DC001/src/lvgl_compat/anim_compat.c/.h`
- `widget_animate_value_to(obj, value, duration)` → `lv_anim_t` + `lv_anim_start()`
- `widget_animator_*` XML声明式动画 → C代码 `lv_anim_set_*()` 序列
- gauge_pointer角度动画: -135°~+135° 映射必须精确保留
- 文件位置: `app/hcn/ui/HCN_DC001/src/lvgl_compat/anim_compat.c/.h`

---

### Phase 2: 构建系统切换 (M006-M008) — 第2位Claude

**M006 - 修改IAR工程配置**
- 修改: `proj/awtk.ewp` → 复制为新工程或修改预定义宏
- 核心操作: 
  - 移除CCDefines中的`AWTK`, `HMI_AWTK`, `XM_HMI_HOST`, `USE_GUI_MAIN`, `LCD=VG_GPU`, `NANOVG_BACKEND=VG`等
  - 保留: `AMT630HV100`, `VG_DRIVER`(如需VG加速)
  - 添加: 无需添加特殊宏(LVGL模式是不定义AWTK时的默认路径)
- 更新链接脚本引用: `amt630hv100_awtk.icf` → `amt630hv100_lvgl.icf`
- 更新源文件列表: 移除lib/awtk/下3170个文件, 添加lib/LittlevGL/下文件

**M007 - 修改FreeRTOSConfig.h堆配置**
- 修改: `FreeRTOSConfig.h`
- 当前: `#elif defined(AWTK)` → `HCN_configTOTAL_HEAP_SIZE` (23.5MB)
- 目标: 不定义AWTK后, 走`#else`分支
  - `#ifdef VG_DRIVER` + `!AWTK` + `!VG_ONLY` → `configTOTAL_HEAP_SIZE = (10+32)MB`
  - 可能不够! AWTK用23.5MB, 需要评估LVGL+HCN业务实际需求
  - **建议**: 新增 `#elif defined(HCN_LVGL)` 分支, 设16MB (LVGL远小于AWTK内存需求)
- 修改 `hcn_config.h`: 将 `HCN_AWTK_HEAP_SIZE` 改为 `HCN_LVGL_HEAP_SIZE` 或移除

**M008 - 修改链接脚本**
- 基于: `proj/amt630hv100_lvgl.icf` (已存在)
- 验证内存布局是否与M007的heap配置匹配
- AWTK版ROM区: 0x20000080-0x2063ffff (约6.25MB)
- LVGL版可能ROM区更小(无nanovg), 释放给RAM

---

### Phase 3: 系统入口层 (M009-M012) — 第3位Claude

**M009 - 修改main.c条件编译**
- 修改: `main.c` 第68-84行
- 当前: `#if defined(AWTK)` → `mainCREATE_AWTK_DEMO`
- 目标: 不定义AWTK宏后自动走`#else` → `mainCREATE_LVGL_DEMO`, 调用`main_lvgl()`
- **注意**: 现有`main_lvgl.c`是SDK的demo入口(xinbas_demo/haoke_demo), 不是HCN业务
- 需要新建: `app/main_hcn_lvgl.c` 作为HCN+LVGL的入口

**M010 - 创建main_hcn_lvgl.c (核心入口)**
- 新建: `app/main_hcn_lvgl.c`
- 从`main_awtk.c`(1022行)迁移, 结合`main_lvgl.c`(1614行)的LVGL初始化模式:
  - 保留: WiFi/BT/CarLink/OTA/SDMMC/USB初始化 (从main_awtk.c)
  - 保留: hcn_mw_init() 中间件初始化
  - 保留: ReadRomFile() (注意: rom.bin是AWTK资源, 需替换)
  - 替换: gui_app_start() → lv_init() + hal_init() + hcn_lvgl_ui_start()
  - 保留: set_qr_text_buf() / get_qr_text_buf()
  - 替换: xTaskCreate("awtk", 32768) → xTaskCreate("lvgl", 4096) [LVGL栈需求远小于AWTK]
  - 保留: SendKeypadInputEventFromISR() → 使用Queue投递
  - 移除: dummy SendTouchInputEvent() → 使用真正的LVGL touch handler
- 条件编译: `#if !defined(VG_ONLY) && !defined(AWTK)` (与现有main_lvgl.c同条件)
- 文件位置: `app/main_hcn_lvgl.c`

**M011 - 修改main_lvgl.c**
- 修改: `app/main_lvgl.c`
- 当前: 编译守卫 `#if !defined(VG_ONLY) && !defined(AWTK)` → 不定义AWTK后自动编译
- 问题: main_lvgl.c中的demo入口(xinbas_demo/haoke_demo)会与HCN冲突
- 方案: 在 main_lvgl.c 的demo调用处添加 `#ifndef HCN_SCREEN_ENABLE` 守卫
- 或: 在新 main_hcn_lvgl.c 中完全替代 main_lvgl() 函数

**M012 - 修改board.h**
- 修改: `ArkmicroFiles/libboard-amt630hv100/include/board.h` 第67行
- 当前: `#elif defined(AWTK)` → LCD_WIDTH=HCN_LCD_WIDTH, LCD_BPP=HCN_LCD_BPP
- 目标: 不定义AWTK后走`#else`分支, LCD_WIDTH=1024, LCD_HEIGHT=600, LCD_BPP=16
- **严重问题**: HCN需要LCD_BPP=32, 但`#else`默认是16!
- 解决: 添加 `#elif defined(HCN_SCREEN_ENABLE)` 分支, 引用hcn_config.h的值

---

### Phase 4: BSP/驱动层适配 (M013-M017) — 第4位/第5位Claude

**M013 - 修改lcd.c**
- 修改: `ArkmicroFiles/libcpu-amt630hv100/source/lcd.c`
- 6处 `#ifdef AWTK`:
  - 第104行: `FB_COUNT` → LVGL也需要3帧缓冲(双缓冲+VG), 改为 `#if defined(AWTK) || defined(HCN_SCREEN_ENABLE)`
  - 第144/1073/1172行: `WITH_VGCANVAS` 相关 → LVGL不使用nanovg, 走#else
  - 第255行: `ark_lcs_get_osd_area()` → LVGL也需要此函数, 改为 `#if defined(AWTK) || defined(HCN_SCREEN_ENABLE)`
  - 第958行: `HD=OSD_WIDTH` → 同理, 改为 `#if defined(AWTK) || defined(HCN_SCREEN_ENABLE)`

**M014 - 修改gt657x.c触摸屏驱动**
- 修改: `ArkmicroFiles/libboard-amt630hv100/source/touchscreen/gt657x.c`
- 第143行: `#ifdef AWTK` → `XM_TpEventProc` → 改为走`#else`分支(LVGL lv_indev_data_t)
- 第331行: `#ifdef AWTK` → touch up事件 → 同理
- 第376行: `#ifdef AWTK` → 同理
- **关键**: 移除AWTK宏后自动走LVGL路径, 这些位置不需要修改!

**M015 - 修改ft6336.c触摸屏驱动**
- 修改: `ArkmicroFiles/libboard-amt630hv100/source/touchscreen/ft6336.c`
- 同M014逻辑, 3处 `#ifdef AWTK` → 移除宏后自动走LVGL路径

**M016 - 修改gt9xx.c触摸屏驱动**
- 修改: `ArkmicroFiles/libboard-amt630hv100/source/touchscreen/gt9xx.c`
- 同M014逻辑, 3处 `#ifdef AWTK`

**M017 - 修改vg_driver_wrapper.c 和 keypad.c / touch.c**
- 修改: `lib/vg_driver/vg_driver_wrapper.c` (第10/13/201行)
- 修改: `ArkmicroFiles/libboard-amt630hv100/source/keypad.c` (第10行)
- 修改: `ArkmicroFiles/libboard-amt630hv100/source/touch.c` (第10行)
- 这些文件的`!defined(AWTK)`守卫: 移除AWTK宏后自动激活, 验证逻辑即可

---

### Phase 5: CarLink/WiFi集成层 (M018-M019) — 第6位Claude

**M018 - 修改carlink_ec.c**
- 修改: `app/carlink/EC/src/carlink_ec.c`
- 第62行: `#ifdef AWTK` → `void set_qr_text_buf(const char *str)` 声明
  - 改为: 无条件声明(函数定义移至main_hcn_lvgl.c)
- 第803行: `#ifdef AWTK` → `set_qr_text_buf(UrlData)` 调用
  - 改为: 无条件调用

**M019 - 修改carlink_ec.c (EC-orig)**
- 修改: `app/carlink/EC-orig/src/carlink_ec.c`
- 同M018逻辑

---

### Phase 6: UI公共层改造 (M020-M023) — 第7位/第8位Claude

**M020 - 修改common.h (UI公共头)**
- 修改: `app/hcn/ui/HCN_DC001/src/view/home_view/common.h`
- 当前: `#include "awtk.h"`
- 目标: `#include "lvgl.h"` + `#include "lvgl_compat/awtk_to_lvgl.h"`
- 保留所有业务常量(SPEED_MAX, RPM_MAX, ANGLE_MAX等)
- 保留所有业务枚举(unit_e, drv_mode_e, gear_e)
- 保留所有页面名称常量(HOME_PAGE, LINK_PAGE等)

**M021 - 修改application.c (应用初始化)**
- 修改: `app/hcn/ui/HCN_DC001/src/application.c`
- 移除: `#include "awtk.h"`, qr_register.h, chart_view_register.h
- 重写 `application_init()`:
  - 移除: `custom_widgets_register()` (QR/chart不再需要AWTK注册)
  - 替换: `navigator_to(APP_START_PAGE)` → `screen_mgr_to("home_page")`
- 重写 `application_exit()`: 清理LVGL资源

**M022 - 修改app_main.c**
- 修改: `app/hcn/ui/HCN_DC001/src/app_main.c`
- 当前: `#include "awtk.h"`, `#include "awtk_main.inc"`, assets.inc
- 这个文件在LVGL模式下不再需要(它是AWTK的入口桩)
- 方案: 用 `#ifdef AWTK` 包裹整个文件, 或者直接弃用

**M023 - 重写navigator.c (页面导航)**
- 修改: `app/hcn/ui/HCN_DC001/src/common/navigator.c`
- 当前: 99行, 依赖 window_manager(), window_open_and_close(), widget_child() 等
- 重写为screen_manager的thin wrapper, 保持接口不变:
  - `navigator_to(name)` → `screen_mgr_to(name)`
  - `navigator_replace(name)` → `screen_mgr_replace(name)`
  - `navigator_switch_to(name, close)` → `screen_mgr_switch(name, close)`
  - `navigator_back()` → `screen_mgr_back()`
  - `navigator_global_widget_on()` → lv_group全局事件
- navigator.h 接口保持不变, 下游108个文件无需改include

---

### Phase 7: XML布局→C代码转写 (M024-M028) — 第9位~第13位Claude

**M024 - 转写home_page.xml → C代码**
- 新建: `app/hcn/ui/HCN_DC001/src/ui_build/ui_home_page.c/.h`
- 将XML中的控件树用 lv_obj_create() 系列API重建
- 关键控件映射:
  - `<window>` → `lv_obj_create(NULL, NULL)` (screen)
  - `<view>` → `lv_cont_create(parent, NULL)` + lv_obj_set_pos/size
  - `<label>` → `lv_label_create(parent, NULL)` + lv_label_set_text
  - `<image>` → `lv_img_create(parent, NULL)` + lv_img_set_src
  - `<progress_bar>` → `lv_bar_create(parent, NULL)`
  - `<progress_circle>` → `lv_arc_create(parent, NULL)` (近似, 需要自定义绘制)
  - `<image_value>` → `lv_img_create` + 数字图片切换逻辑
  - `<gauge_pointer>` → `lv_img_create` + lv_img_set_angle() + lv_img_set_pivot()
  - `<slide_menu>` → `lv_roller_create` 或自定义
  - `<pages>` → `lv_tabview_create` 或手动screen切换
  - `<slide_view>` → `lv_page_create` + 手势控制
- 每个命名控件创建后调用 `widget_reg_add(name, obj)`
- **XML中的animation属性**: 暂时忽略, 在M005动画层中处理

**M025 - 转写link_page.xml → C代码**
- 新建: `app/hcn/ui/HCN_DC001/src/ui_build/ui_link_page.c/.h`
- QR码控件 → lv_canvas + qrencode库绘制

**M026 - 转写device_page.xml → C代码**
- 新建: `app/hcn/ui/HCN_DC001/src/ui_build/ui_device_page.c/.h`

**M027 - 转写update_page.xml → C代码**
- 新建: `app/hcn/ui/HCN_DC001/src/ui_build/ui_update_page.c/.h`

**M028 - 转写component.xml + 样式/主题**
- 新建: `app/hcn/ui/HCN_DC001/src/ui_build/ui_styles.c/.h`
- 将 styles/default.xml (日间) + night/styles/default.xml (夜间) → lv_style_t 结构
- 创建: `app/hcn/ui/HCN_DC001/src/ui_build/ui_theme.c/.h`
- 实现: `void ui_set_theme_day(void)` / `void ui_set_theme_night(void)`

---

### Phase 8: 视图层重写 (M029-M033) — 第14位~第18位Claude

**M029 - 重写view_manager.c**
- 修改: `app/hcn/ui/HCN_DC001/src/view/view_manager.c`
- 替换AWTK依赖:
  - `widget_t*` → `lv_obj_t*`
  - `window_manager_get_top_window(window_manager())` → `screen_mgr_get_top_name()`
  - `widget_on(window_manager(), EVT_KEY_DOWN, ...)` → lv_group + LV_EVENT_KEY
  - `slide_view_set_active_ex()` → lv_tabview_set_tab_act() 或自定义
  - `pages_set_active()` → screen切换
  - `idle_queue(on_idle_queue, id)` → FreeRTOS Queue → lv_async
  - `time_now_s()` → lv_tick_get()/1000
- HCN_KEY_DISPATCH宏: 替换tk_str_eq为strcmp
- view_manager.h: `widget_t*` → `lv_obj_t*`

**M030 - 重写speed_view.c/power_view.c (仪表盘核心)**
- 修改: `app/hcn/ui/HCN_DC001/src/view/home_view/speed_view.c`
- **致命风险控件**:
  - `gauge_pointer_set_image()` → `lv_img_set_angle()` + `lv_img_set_pivot()` 
    - 角度映射: AWTK `-135°~+135°` → LVGL `0~3600` (0.1度单位) 
    - 转换: `lvgl_angle = (awtk_angle + 135) * 10`  **必须验证!**
  - `progress_circle` → `lv_arc` (start_angle=135°, 范围270°)
  - `image_value` → 数字图片拼接显示 (speed_0.png ~ speed_9.png)
  - `widget_animate_value_to()` → `lv_anim_set_values()` + `lv_anim_start()`
  - `slide_menu_set_value()` → 自定义roller或label组切换
- 修改: `app/hcn/ui/HCN_DC001/src/view/home_view/power_view.c`
  - `slider_set_value()` → `lv_slider_set_value()`

**M031 - 重写signal_view.c/electrical_view.c/mileage_view.c**
- 修改: signal_view.c (指示灯图标切换)
  - `image_set_image(obj, name)` → `lv_img_set_src(obj, &img_desc)`
  - 需要资源名→lv_img_dsc_t的映射表
- 修改: electrical_view.c (电量条)
  - `progress_bar_set_value()` → `lv_bar_set_value()`
  - `widget_set_style_str(STYLE_ID_FG_COLOR)` → `lv_style_set_bg_color()`
  - 动画: `widget_start_animator()` → `lv_anim_start()`
- 修改: mileage_view.c (TRIP/ODO显示)
  - `widget_set_text_utf8()` → `lv_label_set_text()`

**M032 - 重写clock_view/dock_view/info_view/music_view/phone_view/navigation_view**
- 修改: 12个文件 (6个view的.c/.h)
- clock_view: label文本设置, 冒号闪烁
- dock_view: 侧边栏图标状态切换 (icon selected/normal)
- info_view: 骑行时间/距离显示
- music_view: `hscroll_label` → `lv_label` + `LV_LABEL_LONG_SROLL_CIRC`
- phone_view: 蓝牙电话状态
- navigation_view: 导航图标/距离显示

**M033 - 重写全部set_view组 (设置页面)**
- 修改: 20个文件 (10个setting子页的.c/.h)
- setting_menu.c: `scroll_view` → `lv_list_create()` + 滚动
- brightness.c: `slider_set_value()` → `lv_slider_set_value()`
- cycling_energy.c: `chart_view` → `lv_chart_create()` + `lv_chart_set_point_count()`
- language.c: `locale_info_change()` → 自建i18n切换
- display.c: 日/夜主题切换 → `lv_theme_set_act()` 或style热替换
- setting_clock.c: `timer_add` 闪烁逻辑 → `lv_task_create()`

---

### Phase 9: 第三方控件替换 (M034-M035) — 第19位/第20位Claude

**M034 - 替换awtk-widget-qr → qrencode + lv_canvas**
- 新建: `app/hcn/ui/HCN_DC001/3rd/qr_lvgl/qr_lvgl.c/.h`
- 从awtk-widget-qr中提取纯qrencode算法部分(qrencode.c, qrinput.c, qrspec.c, bitstream.c, mask.c, rsecc.c, split.c等)
- 这些文件是纯C算法, 不依赖AWTK!
- 新增LVGL渲染: `lv_obj_t* qr_lvgl_create(lv_obj_t* parent, const char* text, int size)`
  - 使用 `lv_canvas_create()` + 像素绘制
- 移除: qr_register.c/.h (AWTK widget注册)

**M035 - 替换awtk-widget-chart-view → lv_chart**
- cycling_energy页使用chart_view绘制骑行能耗图表
- 替换为LVGL内置`lv_chart`:
  - `chart_view_create()` → `lv_chart_create()`
  - `series_fifo` 数据源 → `lv_chart_set_point_count()` + `lv_chart_set_next()`
  - axis → `lv_chart` 内置坐标
  - 移除42个chart_view源文件

---

### Phase 10: 资源/集成/验证 (M036-M038) — 第21位~第25位Claude

**M036 - 图片资源转换**
- AWTK: rom.bin打包 → rommaker.exe 打包所有PNG为二进制
- LVGL: 每张PNG → `lv_img_dsc_t` C数组 (使用LVGL Image Converter)
- 清单: bg_cricle, bg_halo_0/1/2, bg_center, pointer_0/1/2, speed_0~9, dashboard_progress_0/1/2, 所有dock_*图标, top_*指示灯图标, gear_*图标, unit_*图标, drv_mode_*图标, 所有设置页图标
- 工具: LVGL Online Image Converter 或 lv_img_conv
- 输出: `app/hcn/ui/HCN_DC001/res_lvgl/` 目录下大量.c文件

**M037 - 字体资源转换**
- AWTK: TTF/位图字体打包在rom.bin
- LVGL: 使用lv_font_conv将TTF转为`lv_font_t` C数组
- 需要: 24号/26号/33号/35号/44号/51号字体 (从XML中提取的font_size)

**M038 - 全链路集成测试**
- 编译: IAR全工程编译, 解决链接错误
- 验证项:
  1. 启动→自检动画→主页显示
  2. 速度表盘指针角度精度 (0-199 km/h)
  3. 电量条颜色阈值切换 (10%红/20%黄/绿)
  4. 日/夜主题切换
  5. 触摸屏响应
  6. 按键导航: UP/DOWN/SET/BACK
  7. 页面切换: 主页→互联→设备→升级→返回
  8. QR码生成与显示
  9. 蓝牙电话接听/挂断
  10. OTA升级页面
  11. 音乐滚动标签
  12. CarPlay/Android Auto回调
  13. 里程/TRIP/ODO显示
  14. 内存: 运行30分钟无泄漏 (FreeRTOS heap监控)
  15. 栈溢出: vApplicationStackOverflowHook不触发

---

## Claude轮次分配

| Claude# | 里程碑 | 工作内容 | 预估修改文件数 |
|---------|--------|---------|--------------|
| 第1位 | M001-M005 | 兼容层5个新文件 | 5新建 |
| 第2位 | M006-M008 | 构建系统 | 3修改 |
| 第3位 | M009-M012 | 系统入口 | 3修改+1新建 |
| 第4位 | M013-M014 | LCD+触摸(gt657x) | 2修改 |
| 第5位 | M015-M017 | 触摸(ft6336/gt9xx)+VG+keypad | 4修改 |
| 第6位 | M018-M019 | CarLink | 2修改 |
| 第7位 | M020-M021 | common.h+application.c | 2修改 |
| 第8位 | M022-M023 | app_main.c+navigator.c | 2修改 |
| 第9位 | M024 | home_page.xml→C | 1新建(大) |
| 第10位 | M025 | link_page.xml→C | 1新建 |
| 第11位 | M026 | device_page.xml→C | 1新建 |
| 第12位 | M027 | update_page.xml→C | 1新建 |
| 第13位 | M028 | styles/theme→C | 2新建 |
| 第14位 | M029 | view_manager重写 | 2修改 |
| 第15位 | M030 | speed_view/power_view | 4修改 |
| 第16位 | M031 | signal/electrical/mileage | 6修改 |
| 第17位 | M032(前半) | clock/dock/info | 6修改 |
| 第18位 | M032(后半) | music/phone/navigation | 6修改 |
| 第19位 | M033(前半) | menu/brightness/energy/clock_set | 8修改 |
| 第20位 | M033(后半) | bt/language/unit/display/device | 10修改 |
| 第21位 | M034 | QR码控件 | 1新建+24纯算法保留 |
| 第22位 | M035 | chart控件 | 1新建 |
| 第23位 | M036(前半) | 图片资源转换 | ~50新建 |
| 第24位 | M036(后半) | 图片资源转换 | ~50新建 |
| 第25位 | M037 | 字体资源转换 | ~6新建 |
| 第26位~第28位 | M038 | 集成调试1(编译通过) | 多文件修复 |
| 第29位~第31位 | M038 | 集成调试2(基础功能) | 多文件修复 |
| 第32位~第34位 | M038 | 集成调试3(指针精度/动画) | 多文件修复 |
| 第35位~第36位 | M038 | 集成调试4(主题/i18n/QR) | 多文件修复 |
| 第37位~第38位 | M038 | 集成调试5(CarLink/OTA/内存) | 多文件修复 |

---

## 批判性分析 (以Knuth《计算机程序设计艺术》作者视角)

### 1. 用户角度的潜在Bug

1. **仪表盘指针抖动/偏移** (致命): AWTK gauge_pointer使用anchor_y=1.0和image旋转, 角度范围-135°~+135°。LVGL 7.x的lv_img_set_angle()使用0.1度单位(0~3600), 且旋转中心点(pivot)设置方式不同。如果映射公式有off-by-one或浮点截断, 用户看到的速度指针可能偏离刻度。**必须用示波器级精度验证每个速度值对应的像素位置。**

2. **触摸失灵/延迟** (高): AWTK模式下触摸通过`XM_TpEventProc`直接投递到AWTK事件系统(同步)。LVGL模式下通过`xQueueSend` + `lv_indev_read_cb`轮询(异步)。如果LVGL task周期过长或Queue满, 用户会感到触摸迟钝或丢点。

3. **按键响应丢失** (高): 当前`idle_queue`保证ISR按键事件安全投递到GUI线程。LVGL 7.x的`lv_async_call`不是ISR安全的。如果兼容层实现不当, 快速按键可能丢失, 用户无法操作菜单。

4. **日/夜主题切换闪屏** (中): AWTK的主题切换是声明式的(style资源热替换), LVGL需要逐个控件更新style。如果实现为同步遍历(200+控件), 可能出现0.5-1s的逐个控件闪烁。

5. **QR码生成失败** (中): qrencode纯算法部分无问题, 但canvas渲染在1024x600分辨率下的像素对齐可能导致QR码扫描失败。

6. **音乐标题滚动卡顿** (低): AWTK hscroll_label有硬件加速, LVGL的`LV_LABEL_LONG_SROLL_CIRC`是纯软件实现, 在长文本时可能造成帧率下降。

7. **里程精度丢失** (低): mileage_calc.c中的ODO/TRIP计算不依赖AWTK(proxy层零AWTK依赖), 但显示刷新频率从AWTK的timer_add(50ms)改为lv_task后, 如果任务优先级配置不当, 可能出现显示更新滞后。

### 2. 系统角度的潜在Bug

1. **栈溢出** (致命): AWTK任务栈32768 words(128KB), LVGL默认2048 words(8KB)。HCN的UI层(108个文件)函数调用深度+局部变量可能远超8KB。特别是speed_view.c中的snprintf格式化(128字节局部buffer)和cycling_energy.c的图表数据(可能有大数组)。**必须将LVGL任务栈设为至少8192 words(32KB)**, 并在M038阶段用`uxTaskGetStackHighWaterMark()`验证。

2. **堆碎片化导致OOM** (高): AWTK使用自己的内存管理器(tk_mem), LVGL使用lv_mem(默认从静态数组分配)。如果LVGL的LV_MEM_SIZE配置不足, 或者频繁创建/销毁screen导致碎片, 系统会在运行数小时后OOM。**必须配置LV_MEM_SIZE≥2MB, 并开启LV_MEM_CUSTOM使用FreeRTOS heap4。**

3. **竞态条件** (高): AWTK的`idle_queue`有内置mutex保护。替换为FreeRTOS Queue后, 如果忘记在lv_task轮询中加锁(LVGL不是线程安全的), 多个ISR同时写Queue + GUI线程读Queue可能导致数据损坏。**所有LVGL API调用必须在同一个线程中, 且Queue操作使用xQueueSendFromISR/xQueueReceive。**

4. **DMA缓冲区对齐** (中): AWTK使用3帧缓冲(FB_COUNT=3)且可能有特殊的DMA对齐要求。LVGL默认使用2个刷新缓冲区。如果lcd.c中FB_COUNT改为2但VG硬件加速仍需3帧, 会导致画面撕裂。

5. **ROM空间不足** (中): AWTK的rom.bin是压缩打包的; LVGL的C数组图片资源未压缩, 可能导致Flash容量不足。需要评估32MB SPI NOR Flash的剩余空间。

6. **链接脚本内存映射错误** (中): amt630hv100_awtk.icf定义ROM区到0x2063ffff(~6.25MB), 如果切换到amt630hv100_lvgl.icf, 其ROM/RAM边界不同, 可能导致HCN业务代码+数据+堆的总量超出物理内存。

7. **看门狗超时** (低): AWTK主循环在VG模式下有自己的帧节拍; LVGL的`lv_task_handler()`调用频率取决于FreeRTOS任务调度。如果GUI任务被高优先级任务(如WiFi/BT)长时间抢占, 看门狗可能触发重启。
