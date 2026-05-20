/**
 * @file anim_compat.h
 * @brief AWTK widget_animator → LVGL lv_anim 兼容层
 *
 * AWTK 动画系统:
 *   - widget_animate_value_to(obj, value, duration) → 渐变到目标值
 *   - widget_start_animator(obj, name)  → 启动命名动画
 *   - widget_stop_animator(obj, name)   → 停止命名动画
 *   - XML 声明式: <animation delay="..." auto_start="true" .../>
 *
 * LVGL 7.x 动画系统:
 *   - lv_anim_t + lv_anim_init() + lv_anim_set_*() + lv_anim_start()
 *   - 完全 C 代码驱动, 无声明式
 *
 * 仪表盘关键动画:
 *   1. 速度指针: gauge_pointer 角度渐变
 *      AWTK: -135° ~ +135° (直接角度)
 *      LVGL: 0 ~ 3600 (0.1度单位)
 *      转换: lvgl_angle = (awtk_angle + 135) * 10
 *
 *   2. 功率表: slider 值渐变
 *   3. 自检动画: 指针从0扫到满后归0
 *
 * @date  2026-05-20
 * @note  M005 里程碑
 */

#ifndef __ANIM_COMPAT_H__
#define __ANIM_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include <stdint.h>

/**
 * @brief 初始化动画兼容层
 */
void anim_compat_init(void);

/**
 * @brief 对控件做值渐变动画 (AWTK widget_animate_value_to 兼容)
 * @param obj           目标控件
 * @param target_value  目标值
 * @param duration_ms   动画时长 (毫秒)
 *
 * 语义: 从控件当前值平滑过渡到 target_value。
 * 对 label: 无实际动画效果, 直接设置值。
 * 对 bar/slider/arc: 使用 lv_anim_t 驱动。
 */
void widget_animate_value_to(lv_obj_t *obj, int target_value, uint32_t duration_ms);

/**
 * @brief 启动命名动画 (AWTK widget_start_animator 兼容)
 * @param obj   目标控件
 * @param name  动画名 (如 "opacity", "rotate" 等)
 * @return 0=成功, -1=不支持
 *
 * @note HCN 项目中命名动画仅在自检阶段使用 (animation_ctrl.c)
 */
int widget_start_animator(lv_obj_t *obj, const char *name);

/**
 * @brief 停止命名动画
 * @param obj   目标控件
 * @param name  动画名
 * @return 0=成功
 */
int widget_stop_animator(lv_obj_t *obj, const char *name);

/* ======================================================================
 * 仪表盘专用: 指针角度动画
 * ====================================================================== */

/**
 * @brief 设置指针角度 (带动画)
 * @param obj           指针图片控件 (lv_img)
 * @param target_angle  AWTK 角度 (-135 ~ +135)
 * @param duration_ms   动画时长
 *
 * 内部转换:
 *   AWTK  角度:   -135°  0°   +135°
 *   对应速度:       0   100    199
 *   LVGL 0.1度:     0°  1350  2700  (= (awtk + 135) * 10)
 *
 * 注意: LVGL lv_img_set_angle() 使用 0.1度, 顺时针, 0°=12点方向
 *       AWTK gauge_pointer 使用度, 逆时针?, 需要根据实际效果调整
 */
void gauge_pointer_animate_angle(lv_obj_t *obj, int target_angle_awtk,
                                  uint32_t duration_ms);

/**
 * @brief 直接设置指针角度 (无动画)
 * @param obj    指针图片控件
 * @param angle  AWTK 角度 (-135 ~ +135)
 */
void gauge_pointer_set_angle(lv_obj_t *obj, int angle_awtk);

/**
 * AWTK→LVGL 角度转换
 * @param awtk_angle  AWTK 角度 (-135 ~ +135)
 * @return LVGL 0.1度值
 */
static inline int16_t awtk_angle_to_lvgl(int awtk_angle)
{
    /**
     * AWTK gauge_pointer:
     *   -135° = 最小值 (7点半方向)
     *   0°    = 正上方 (12点方向)
     *   +135° = 最大值 (4点半方向)
     *
     * LVGL lv_img_set_angle:
     *   0     = 无旋转 (图片原始方向)
     *   900   = 顺时针90° (3点方向)
     *   1800  = 180° (6点方向)
     *   2700  = 270° (9点方向)
     *
     * 假设指针图片原始方向朝上 (12点),
     * 那么 AWTK -135° = 顺时针 -135° = LVGL (360-135)*10 = 2250
     * 但这取决于图片实际朝向, 需要实测验证!
     *
     * 安全起见, 使用最简单的直接映射:
     *   LVGL角度 = AWTK角度 * 10
     * 并在 M038 集成测试阶段用实际图片验证。
     */
    return (int16_t)(awtk_angle * 10);
}

#ifdef __cplusplus
}
#endif

#endif /* __ANIM_COMPAT_H__ */
