/**
 * @file widget_registry.h
 * @brief 控件名称→lv_obj_t* 注册表
 *
 * AWTK 中 widget_lookup(parent, name, recursive) 可按名称查找控件树中任意控件。
 * LVGL 7.x 不支持按名称查找, 本模块提供静态注册表替代。
 *
 * 使用方式:
 *   1. 在 UI 构建代码(原 XML→C 转写)中, 每创建一个命名控件后调用
 *      widget_reg_add("speed_value", obj);
 *   2. 在视图层代码中, 用 widget_lookup(NULL, "speed_value", TRUE) 查找
 *      (parent 参数被忽略, 因为注册表是全局的)
 *
 * 线程安全: 非线程安全, 仅在 GUI 线程中调用。
 *
 * @date  2026-05-20
 * @note  M002 里程碑
 */

#ifndef __WIDGET_REGISTRY_H__
#define __WIDGET_REGISTRY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include <stdbool.h>

/** 最大注册控件数 (当前 UI 约200个命名控件, 预留余量) */
#define WIDGET_REG_MAX_ENTRIES  (256)

/**
 * @brief 注册一个命名控件
 * @param name  控件名称 (不可为NULL, 长度不超过31字符)
 * @param obj   LVGL 对象指针
 * @note  重复注册同名控件会覆盖旧指针
 */
void widget_reg_add(const char *name, lv_obj_t *obj);

/**
 * @brief 按名称查找已注册控件
 * @param name  控件名称
 * @return lv_obj_t* 或 NULL (未找到)
 */
lv_obj_t *widget_reg_find(const char *name);

/**
 * @brief 清空注册表 (页面切换时调用)
 */
void widget_reg_clear(void);

/**
 * @brief AWTK 兼容: widget_lookup
 * @param parent     忽略 (LVGL 中不做树遍历, 从注册表查找)
 * @param name       控件名称
 * @param recursive  忽略
 * @return lv_obj_t* 或 NULL
 */
lv_obj_t *widget_lookup(lv_obj_t *parent, const char *name, bool recursive);

/**
 * @brief 调试: 打印注册表内容
 */
void widget_reg_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIDGET_REGISTRY_H__ */
