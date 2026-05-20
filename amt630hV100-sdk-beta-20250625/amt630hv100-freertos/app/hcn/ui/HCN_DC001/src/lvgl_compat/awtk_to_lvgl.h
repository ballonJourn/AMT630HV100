/**
 * @file awtk_to_lvgl.h
 * @brief AWTK → LVGL 7.11.0 类型与宏兼容层
 *
 * 本文件为 HCN DC002 AWTK→LVGL 迁移项目的核心兼容头文件。
 * 所有原先 #include "awtk.h" 的文件改为 #include "lvgl_compat/awtk_to_lvgl.h"
 *
 * 设计原则:
 *   1. typedef 尽量薄, 只做类型别名, 不引入运行时开销
 *   2. 宏映射保持语义一致, 有差异处加 FIXME 注释
 *   3. 函数声明仅放 wrapper, 实现在 .c 文件中
 *
 * @date  2026-05-20
 * @note  M001 里程碑
 */

#ifndef __AWTK_TO_LVGL_H__
#define __AWTK_TO_LVGL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 * 1. LVGL 主头文件
 * ====================================================================== */
#include "lvgl/lvgl.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================
 * 2. 核心类型映射
 * ====================================================================== */

/** AWTK widget_t → LVGL lv_obj_t */
typedef lv_obj_t widget_t;

/**
 * AWTK ret_t → 整数返回值
 * AWTK 使用枚举: RET_OK=0, RET_FAIL, RET_REPEAT, RET_REMOVE 等
 * LVGL 7.x 使用 lv_res_t: LV_RES_OK=0, LV_RES_INV=1 (只有两个值)
 * 为保持业务层 switch/if 语义, 使用 int 作为底层类型
 */
typedef int ret_t;

#define RET_OK          (0)
#define RET_FAIL        (-1)
#define RET_BAD_PARAMS  (-2)
#define RET_NOT_FOUND   (-3)

/* timer/task 回调返回值: 控制是否继续重复 */
#define RET_REPEAT      (1)   /* 定时器继续执行 */
#define RET_REMOVE      (2)   /* 定时器执行一次后移除 */

/** AWTK bool_t → C99 bool */
typedef bool bool_t;
#ifndef TRUE
#define TRUE  true
#endif
#ifndef FALSE
#define FALSE false
#endif

/* ======================================================================
 * 3. 事件类型映射
 * ====================================================================== */

/**
 * AWTK event_t 是一个通用事件结构, 包含 type 字段
 * LVGL 7.x 事件通过回调参数 lv_event_t (枚举) 传入
 * 这里定义一个 shim 结构供 on_key_event 等回调使用
 */
typedef struct _event_shim_t {
    uint32_t type;        /* 事件类型 */
    lv_obj_t *target;     /* 触发控件 */
} event_t;

typedef struct _key_event_shim_t {
    event_t e;            /* 基础事件 */
    uint32_t key;         /* 按键码 */
} key_event_t;

/* AWTK 事件类型常量 */
#define EVT_KEY_DOWN        (0x1001)
#define EVT_KEY_UP          (0x1002)
#define EVT_KEY_LONG_PRESS  (0x1003)
#define EVT_CLICK           (0x1004)
#define EVT_VALUE_CHANGED   (0x1005)
#define EVT_REQUEST_CLOSE_WINDOW (0x1006)

/* AWTK 事件回调签名 */
typedef ret_t (*event_func_t)(void *ctx, event_t *e);

/* ======================================================================
 * 4. 定时器/异步兼容类型
 * ====================================================================== */

/**
 * AWTK timer_info_t: 定时器回调参数
 * 在 LVGL 中对应 lv_task_t
 */
typedef struct _timer_info_compat_t {
    void     *ctx;        /* 用户上下文 */
    uint32_t  id;         /* 定时器 ID */
} timer_info_t;

typedef ret_t (*timer_func_t)(const timer_info_t *info);

/**
 * AWTK idle_info_t: idle回调参数
 */
typedef struct _idle_info_compat_t {
    void *ctx;            /* 用户上下文 (常用于传递按键 ID) */
} idle_info_t;

typedef ret_t (*idle_func_t)(const idle_info_t *idle);

/* ======================================================================
 * 5. 按键码映射
 * ====================================================================== */

/**
 * AWTK TK_KEY_* → LVGL LV_KEY_* 或自定义
 * view_manager.c 中使用 TK_KEY_w/s/a/d 作为调试按键
 */
#define TK_KEY_w   LV_KEY_UP
#define TK_KEY_s   LV_KEY_DOWN
#define TK_KEY_a   LV_KEY_LEFT
#define TK_KEY_d   LV_KEY_RIGHT

/* ======================================================================
 * 6. 工具宏映射
 * ====================================================================== */

#ifndef tk_min
#define tk_min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef tk_max
#define tk_max(a, b)  ((a) > (b) ? (a) : (b))
#endif

/** AWTK tk_str_eq → strcmp */
#define tk_str_eq(a, b)  (strcmp((a), (b)) == 0)

/** AWTK tk_snprintf → snprintf (LVGL 7.x 内置 lv_snprintf 但签名相同) */
#define tk_snprintf  snprintf

/** AWTK time_now_s → 秒级时间戳 */
static inline uint64_t time_now_s(void)
{
    /* 使用 LVGL tick (毫秒) 转换为秒 */
    return (uint64_t)(lv_tick_get() / 1000u);
}

/** AWTK WIDGET(iter) 强制转换宏 */
#define WIDGET(x)  ((lv_obj_t *)(x))

/** AWTK return_value_if_fail 断言宏 */
#define return_value_if_fail(expr, val)  do { if (!(expr)) return (val); } while(0)

/* ======================================================================
 * 7. 控件操作兼容 API (声明, 实现在各 compat .c 文件中)
 *
 * 这些函数是迁移的核心: 将 AWTK 的命名查找/属性设置 API
 * 桥接到 LVGL 的对象树 API
 * ====================================================================== */

/* --- widget_registry.h 提供 --- */
extern lv_obj_t *widget_lookup(lv_obj_t *parent, const char *name, bool_t recursive);
extern void      widget_reg_add(const char *name, lv_obj_t *obj);
extern lv_obj_t *widget_reg_find(const char *name);
extern void      widget_reg_clear(void);

/* --- 属性设置 wrapper (在各 view 文件中逐步实现) --- */
static inline void widget_set_text_utf8(lv_obj_t *obj, const char *text)
{
    if (obj) lv_label_set_text(obj, text);
}

static inline void widget_set_value_int(lv_obj_t *obj, int value)
{
    if (obj == NULL) return;
    /* 对 label: 设置数字文本; 对 bar/slider: 设置值 */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(obj, buf);
}

static inline void widget_set_visible(lv_obj_t *obj, bool visible)
{
    if (obj) lv_obj_set_hidden(obj, !visible);
}

static inline int widget_count_children(lv_obj_t *obj)
{
    if (obj == NULL) return 0;
    return (int)lv_obj_count_children(obj);
}

static inline lv_obj_t *widget_get_child(lv_obj_t *obj, int index)
{
    if (obj == NULL) return NULL;
    /* LVGL 7.x: lv_obj_get_child 遍历是反序, 需要封装 */
    int total = lv_obj_count_children(obj);
    if (index < 0 || index >= total) return NULL;

    lv_obj_t *child = lv_obj_get_child(obj, NULL);
    /* LVGL 7.x child遍历: 从最后一个开始, 向前 */
    int reverse_idx = total - 1 - index;
    for (int i = 0; i < reverse_idx && child != NULL; i++) {
        child = lv_obj_get_child(obj, child);
    }
    return child;
}

static inline void widget_set_state(lv_obj_t *obj, const char *state)
{
    /* AWTK state 是字符串: "normal", "selected" 等 */
    /* LVGL 7.x 使用 lv_obj_set_state / lv_obj_clear_state */
    if (obj == NULL || state == NULL) return;
    if (strcmp(state, "selected") == 0) {
        lv_obj_set_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(obj, LV_STATE_CHECKED);
    }
}

/* --- image操作 --- */
/**
 * AWTK image_set_image(obj, name) 通过资源名设置图片
 * LVGL 需要 lv_img_set_src(obj, &img_dsc) 或文件路径
 * 这个函数需要在 M036 资源转换后实现完整映射
 * 目前仅声明
 */
extern void image_set_image(lv_obj_t *obj, const char *name);

/* --- gauge_pointer (速度指针) --- */
extern void gauge_pointer_set_image(lv_obj_t *obj, const char *name);

/* --- progress_bar --- */
static inline void progress_bar_set_value(lv_obj_t *obj, int value)
{
    if (obj) lv_bar_set_value(obj, value, LV_ANIM_ON);
}

/* --- slider --- */
static inline void slider_set_value(lv_obj_t *obj, int value)
{
    if (obj) lv_slider_set_value(obj, value, LV_ANIM_ON);
}

/* --- style wrapper --- */
static inline void widget_set_style_str(lv_obj_t *obj, const char *style_id, const char *value)
{
    (void)obj;
    (void)style_id;
    (void)value;
    /* TODO: M028 主题层 */
}

/* --- AWTK style/prop 常量 --- */
#define STYLE_ID_ICON  "icon"
#define WIDGET_PROP_STATE_FOR_STYLE "state_for_style"

/* --- value_t (AWTK通用值容器) --- */
typedef struct {
    int    type;
    char   str_val[64];
    int    int_val;
} value_t;

static inline const char *value_str(const value_t *v)
{
    return (v != NULL) ? v->str_val : "";
}

static inline ret_t widget_get_prop(lv_obj_t *obj, const char *prop, value_t *v)
{
    (void)obj; (void)prop;
    if (v) {
        memset(v, 0, sizeof(value_t));
        strncpy(v->str_val, "normal", sizeof(v->str_val) - 1);
    }
    return RET_OK;
}

/* --- tk_str_cmp (AWTK) → strcmp --- */
#define tk_str_cmp(a, b)  strcmp((a), (b))

/* --- progress_bar_set_max --- */
static inline void progress_bar_set_max(lv_obj_t *obj, int max_val)
{
    if (obj) lv_bar_set_range(obj, 0, max_val);
}

/* --- series_push (AWTK chart_view 第三方控件) --- */
static inline void series_push(lv_obj_t *obj, float *value, int count)
{
    /* TODO M035: 替换为 lv_chart API */
    (void)obj; (void)value; (void)count;
}

static inline void widget_invalidate_force(lv_obj_t *obj, void *unused)
{
    (void)unused;
    if (obj) lv_obj_invalidate(obj);
}

/* --- slide_menu (AWTK 仪表专用) --- */
static inline void slide_menu_set_value(lv_obj_t *obj, int value)
{
    /* AWTK slide_menu 是水平滑动选择控件, LVGL中用 roller 或自定义 */
    /* TODO: M024 布局转写时实现 */
    (void)obj;
    (void)value;
}

/* --- pages (AWTK 多页容器) --- */
typedef struct {
    int active;
} pages_t;

#define PAGES(x) ((pages_t *)(NULL)) /* TODO: M024 */

static inline void pages_set_active(lv_obj_t *obj, int page)
{
    /* TODO: M024 布局转写时实现 */
    (void)obj;
    (void)page;
}

/* --- slide_view --- */
static inline void slide_view_set_active_ex(lv_obj_t *obj, int page, bool_t anim)
{
    /* TODO: M024 布局转写时实现 */
    (void)obj;
    (void)page;
    (void)anim;
}

/* --- widget_foreach (遍历子控件) --- */
typedef ret_t (*tk_visit_t)(void *ctx, const void *iter);

static inline ret_t widget_foreach(lv_obj_t *obj, tk_visit_t visit, void *ctx)
{
    if (obj == NULL || visit == NULL) return RET_FAIL;
    lv_obj_t *child = lv_obj_get_child(obj, NULL);
    while (child != NULL) {
        visit(ctx, child);
        child = lv_obj_get_child(obj, child);
    }
    return RET_OK;
}

/* --- widget_on (事件绑定) --- */
/* LVGL: lv_obj_set_event_cb() 只能设一个回调, 不如AWTK灵活 */
/* 需要自建事件分发器 */
extern uint32_t widget_on(lv_obj_t *obj, uint32_t type, event_func_t on_event, void *ctx);

/* --- widget_get_prop_bool --- */
static inline bool_t widget_get_prop_bool(lv_obj_t *obj, const char *prop, bool_t defval)
{
    (void)obj;
    (void)prop;
    return defval;
}

/* --- widget_dispatch_simple_event --- */
static inline ret_t widget_dispatch_simple_event(lv_obj_t *obj, uint32_t type)
{
    (void)obj;
    (void)type;
    /* TODO: M023 */
    return RET_OK;
}

/* --- widget_is_dialog --- */
static inline bool_t widget_is_dialog(lv_obj_t *obj)
{
    (void)obj;
    return FALSE; /* HCN项目中无模态对话框 */
}

/* --- AWTK常量 --- */
#define WIDGET_PROP_SINGLE_INSTANCE  "single_instance"
#define NAVIGATOR_PROP_DIALOG_TO_MODAL "dialog_to_modal"
#define STYLE_ID_FG_COLOR  "normal:fg_color"

/* ======================================================================
 * 8. 定时器/异步兼容 API (声明, 实现在 timer_compat.c)
 * ====================================================================== */

extern uint32_t timer_add(timer_func_t cb, void *ctx, uint32_t interval_ms);
extern void     timer_remove(uint32_t id);
extern bool_t   timer_find(uint32_t id);

/**
 * idle_queue: 从任意线程安全投递到GUI线程执行
 * 实现使用 FreeRTOS Queue + lv_task 轮询
 */
extern ret_t idle_queue(idle_func_t cb, void *ctx);

/* ======================================================================
 * 9. 动画兼容 API (声明, 实现在 anim_compat.c)
 * ====================================================================== */

extern void widget_animate_value_to(lv_obj_t *obj, int target_value, uint32_t duration_ms);
extern ret_t widget_start_animator(lv_obj_t *obj, const char *name);
extern ret_t widget_stop_animator(lv_obj_t *obj, const char *name);

/* ======================================================================
 * 10. 窗口管理兼容 API (声明, 实现在 screen_manager.c)
 * ====================================================================== */

extern lv_obj_t *window_manager(void);
extern lv_obj_t *window_manager_get_top_window(lv_obj_t *wm);
extern lv_obj_t *window_manager_get_top_main_window(lv_obj_t *wm);
extern lv_obj_t *widget_child(lv_obj_t *wm, const char *name);
extern lv_obj_t *window_open_and_close(const char *name, lv_obj_t *to_close);
extern ret_t    window_manager_close_window_force(lv_obj_t *wm, lv_obj_t *win);
extern ret_t    window_manager_switch_to(lv_obj_t *wm, lv_obj_t *curr, lv_obj_t *target, bool_t close);
extern ret_t    window_manager_back_to_home(lv_obj_t *wm);
extern ret_t    window_manager_back(lv_obj_t *wm);
extern ret_t    dialog_modal(lv_obj_t *win);

/* ======================================================================
 * 11. 国际化兼容 (声明)
 * ====================================================================== */

extern ret_t locale_info_change(void *info, const char *lang, const char *country);

static inline void *locale_info(void)
{
    return NULL; /* TODO: M028 i18n */
}

/**
 * locale_info_tr — 翻译字符串
 * AWTK: locale_info_tr(locale_info(), "key") → 返回翻译后的字符串
 * LVGL: 暂时直接返回 key 本身 (英文透传)
 * TODO M028: 实现多语言字符串表
 */
static inline const char *locale_info_tr(void *info, const char *key)
{
    (void)info;
    return (key != NULL) ? key : "";
}

/* ======================================================================
 * 12. AWTK 资源管理器桩 (仅为编译通过, 功能在LVGL中不需要)
 * ====================================================================== */

/* assets_manager 类型桩 */
typedef struct { const char *theme; } assets_manager_t;
typedef struct { uint32_t size; uint8_t *data; } asset_info_t;
typedef struct { int w; int h; } bitmap_t;

#define ASSET_TYPE_IMAGE      0
#define ASSET_TYPE_IMAGE_PNG  1

static inline assets_manager_t *assets_manager(void)
{
    static assets_manager_t s_am = { "default" };
    return &s_am;
}

/* image_manager 桩 — LVGL 不使用此机制 */
static inline void *image_manager(void) { return NULL; }

static inline ret_t image_manager_get_bitmap(void *im, const char *name, bitmap_t *bmp)
{
    (void)im; (void)name; (void)bmp;
    return RET_OK;
}

static inline ret_t image_manager_unload_bitmap(void *im, bitmap_t *bmp)
{
    (void)im; (void)bmp;
    return RET_OK;
}

/* assets_manager 操作桩 */
static inline const asset_info_t *assets_manager_ref(assets_manager_t *am, int type, const char *name)
{
    (void)am; (void)type; (void)name;
    return NULL;
}

static inline void assets_manager_unref(assets_manager_t *am, const asset_info_t *info)
{
    (void)am; (void)info;
}

static inline void assets_manager_clear_cache_ex(assets_manager_t *am, int type, const char *name)
{
    (void)am; (void)type; (void)name;
}

static inline ret_t assets_manager_add_data(assets_manager_t *am, const char *name,
                                             int type, int subtype,
                                             uint8_t *data, uint32_t size)
{
    (void)am; (void)name; (void)type; (void)subtype; (void)data; (void)size;
    return RET_OK;
}

static inline asset_info_t *assets_manager_load_file(assets_manager_t *am, int type, const char *name)
{
    (void)am; (void)type; (void)name;
    return NULL;
}

static inline ret_t assets_manager_add(assets_manager_t *am, asset_info_t *info)
{
    (void)am; (void)info;
    return RET_OK;
}

static inline void widget_invalidate(lv_obj_t *obj, void *unused)
{
    (void)unused;
    if (obj) lv_obj_invalidate(obj);
}

#ifdef __cplusplus
}
#endif

#endif /* __AWTK_TO_LVGL_H__ */
