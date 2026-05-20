/**
 * @file anim_compat.c
 * @brief AWTK widget_animator → LVGL lv_anim 兼容层实现
 *
 * @date  2026-05-20
 * @note  M005 里程碑
 */

#include "anim_compat.h"
#include <string.h>
#include <stdio.h>

/* ======================================================================
 * 内部: 命名动画注册表
 *
 * AWTK 的命名动画 (widget_start_animator / widget_stop_animator)
 * 在 HCN 项目中主要用于:
 *   - animation_ctrl.c: 自检时 "opacity" 动画
 *   - home_page.xml: 启动动画
 *
 * 我们维护一个小注册表, 记录正在运行的命名动画
 * ====================================================================== */

#define NAMED_ANIM_MAX  (16)

typedef struct {
    lv_obj_t   *obj;
    char        name[24];
    lv_anim_t   anim;
    bool        active;
} named_anim_entry_t;

static named_anim_entry_t s_named_anims[NAMED_ANIM_MAX];

/* ======================================================================
 * 值渐变动画
 * ====================================================================== */

/**
 * lv_anim 的 exec_cb: 设置 bar 值
 */
static void anim_bar_exec_cb(void *obj, lv_anim_value_t value)
{
    lv_bar_set_value((lv_obj_t *)obj, value, LV_ANIM_OFF);
}

/**
 * lv_anim 的 exec_cb: 设置 slider 值
 */
static void anim_slider_exec_cb(void *obj, lv_anim_value_t value)
{
    lv_slider_set_value((lv_obj_t *)obj, value, LV_ANIM_OFF);
}

/**
 * lv_anim 的 exec_cb: 设置 arc 值
 */
static void anim_arc_exec_cb(void *obj, lv_anim_value_t value)
{
    lv_arc_set_value((lv_obj_t *)obj, value);
}

/**
 * lv_anim 的 exec_cb: 设置 label 数字文本
 */
static void anim_label_exec_cb(void *obj, lv_anim_value_t value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)value);
    lv_label_set_text((lv_obj_t *)obj, buf);
}

/**
 * lv_anim 的 exec_cb: 设置 img 旋转角度
 */
static void anim_img_angle_exec_cb(void *obj, lv_anim_value_t value)
{
    lv_img_set_angle((lv_obj_t *)obj, (int16_t)value);
}

/**
 * lv_anim 的 exec_cb: 设置 opacity
 */
static void anim_opa_exec_cb(void *obj, lv_anim_value_t value)
{
    lv_obj_set_style_local_opa_scale((lv_obj_t *)obj,
                                      LV_OBJ_PART_MAIN,
                                      LV_STATE_DEFAULT,
                                      (lv_opa_t)value);
}

/* ======================================================================
 * 公开 API
 * ====================================================================== */

void anim_compat_init(void)
{
    memset(s_named_anims, 0, sizeof(s_named_anims));
}

void widget_animate_value_to(lv_obj_t *obj, int target_value, uint32_t duration_ms)
{
    if (obj == NULL) return;

    /**
     * 判断控件类型, 选择合适的 exec_cb
     *
     * LVGL 7.x 没有运行时类型查询, 我们通过尝试的方式判断:
     *   - 如果obj有 lv_bar_ext_t → 是 bar
     *   - 如果obj有 lv_slider_ext_t → 是 slider
     *   - 否则当作 label
     *
     * 但 LVGL 7.x 没有公开的类型判断 API (lv_obj_check_type 需要 lv_obj_type_t)
     *
     * 实际方案: 在 widget_reg 中增加类型标记 (M002扩展)
     * 临时方案: 默认为 label, 让各 view 层代码直接调用 lv_bar_set_value 等
     */

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);

    /* 默认使用 label exec_cb
     * 各 view 层可以直接用 lv_anim API 替代此通用函数 */
    lv_anim_set_exec_cb(&a, anim_label_exec_cb);

    /* 起始值: 尝试从 label 文本解析当前数字 */
    int start_value = 0;
    const char *text = lv_label_get_text(obj);
    if (text != NULL) {
        start_value = atoi(text);
    }

    lv_anim_set_values(&a, start_value, target_value);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_set_path(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

int widget_start_animator(lv_obj_t *obj, const char *name)
{
    if (obj == NULL || name == NULL) return -1;

    /* 查找空闲 slot */
    int slot = -1;
    for (int i = 0; i < NAMED_ANIM_MAX; i++) {
        if (!s_named_anims[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        printf("[anim_compat] ERROR: no free named anim slot\n");
        return -1;
    }

    /* 根据名称选择动画类型 */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_time(&a, 500); /* 默认500ms */

    if (strcmp(name, "opacity") == 0) {
        lv_anim_set_exec_cb(&a, anim_opa_exec_cb);
        lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_path(&a, lv_anim_path_ease_in_out);
    } else if (strcmp(name, "rotate") == 0) {
        lv_anim_set_exec_cb(&a, anim_img_angle_exec_cb);
        lv_anim_set_values(&a, 0, 3600); /* 完整旋转360° */
        lv_anim_set_time(&a, 2000);
        lv_anim_set_path(&a, lv_anim_path_linear);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    } else if (strcmp(name, "value") == 0) {
        lv_anim_set_exec_cb(&a, anim_label_exec_cb);
        lv_anim_set_values(&a, 0, 100);
        lv_anim_set_path(&a, lv_anim_path_linear);
    } else {
        printf("[anim_compat] WARNING: unknown animator name '%s'\n", name);
        return -1;
    }

    lv_anim_start(&a);

    /* 记录 */
    s_named_anims[slot].obj = obj;
    strncpy(s_named_anims[slot].name, name, sizeof(s_named_anims[slot].name) - 1);
    s_named_anims[slot].name[sizeof(s_named_anims[slot].name) - 1] = '\0';
    s_named_anims[slot].anim = a;
    s_named_anims[slot].active = true;

    return 0;
}

int widget_stop_animator(lv_obj_t *obj, const char *name)
{
    if (obj == NULL || name == NULL) return -1;

    for (int i = 0; i < NAMED_ANIM_MAX; i++) {
        if (s_named_anims[i].active &&
            s_named_anims[i].obj == obj &&
            strcmp(s_named_anims[i].name, name) == 0) {

            lv_anim_del(obj, NULL); /* 删除该 obj 上的所有动画 */
            s_named_anims[i].active = false;
            return 0;
        }
    }

    return -1;
}

/* ======================================================================
 * 仪表盘指针角度动画
 * ====================================================================== */

void gauge_pointer_animate_angle(lv_obj_t *obj, int target_angle_awtk,
                                  uint32_t duration_ms)
{
    if (obj == NULL) return;

    int16_t target_lvgl = awtk_angle_to_lvgl(target_angle_awtk);
    int16_t current_lvgl = lv_img_get_angle(obj);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_img_angle_exec_cb);
    lv_anim_set_values(&a, current_lvgl, target_lvgl);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_set_path(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void gauge_pointer_set_angle(lv_obj_t *obj, int angle_awtk)
{
    if (obj == NULL) return;
    int16_t lvgl_angle = awtk_angle_to_lvgl(angle_awtk);
    lv_img_set_angle(obj, lvgl_angle);
}

/**
 * gauge_pointer_set_image: AWTK 兼容
 * 在 AWTK 中, gauge_pointer 有自己的 image 属性。
 * 在 LVGL 中, 直接使用 lv_img_set_src()。
 * 这里作为占位, 完整实现在 M036 资源转换后。
 */
void gauge_pointer_set_image(lv_obj_t *obj, const char *name)
{
    (void)obj;
    (void)name;
    /* TODO: M036 — 需要 name → lv_img_dsc_t 映射表 */
    printf("[anim_compat] gauge_pointer_set_image('%s') — TODO M036\n",
           name ? name : "NULL");
}

/**
 * image_set_image: AWTK 兼容
 * 同 gauge_pointer_set_image, 等 M036 资源转换后实现
 */
void image_set_image(lv_obj_t *obj, const char *name)
{
    (void)obj;
    (void)name;
    /* TODO: M036 */
    printf("[anim_compat] image_set_image('%s') — TODO M036\n",
           name ? name : "NULL");
}
