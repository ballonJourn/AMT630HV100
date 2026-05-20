/**
 * @file ui_page_stubs.c
 * @brief 页面 screen init 路由 — 转发到各页面的 ui_xxx_page.c 实现
 *
 * M024-M027 全部完成:
 *   ui_home_page.c   (M024) — 仪表盘主页       ✅
 *   ui_link_page.c   (M025) — 手机互联页       ✅
 *   ui_device_page.c (M026) — 设备信息页       ✅
 *   ui_update_page.c (M027) — OTA升级页        ✅
 *
 * 本文件不再包含任何 stub 实现。
 * 各页面的 ui_xxx_page_init() 函数在各自的 .c 文件中定义。
 * screen_manager 通过函数指针调用这些入口。
 *
 * 如果编译器/链接器需要一个统一的声明点, 保留本文件作为
 * extern 声明 + 注册辅助（未来可扩展）。
 *
 * @date  2026-05-20
 */

/*
 * 各页面 init 函数的 extern 声明 (定义在各自的 ui_xxx_page.c 中):
 *
 * extern int ui_home_page_init(lv_obj_t *screen, void *ctx);   // ui_home_page.c
 * extern int ui_link_page_init(lv_obj_t *screen, void *ctx);   // ui_link_page.c
 * extern int ui_device_page_init(lv_obj_t *screen, void *ctx); // ui_device_page.c
 * extern int ui_update_page_init(lv_obj_t *screen, void *ctx); // ui_update_page.c
 *
 * 这些函数由 screen_manager 的注册表直接引用,
 * 在 application.c 的 screen_mgr_register() 中注册。
 */
