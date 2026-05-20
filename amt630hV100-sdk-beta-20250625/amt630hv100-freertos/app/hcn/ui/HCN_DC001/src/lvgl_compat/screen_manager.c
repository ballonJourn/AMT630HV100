/**
 * @file screen_manager.c
 * @brief LVGL 屏幕/页面管理器实现
 *
 * 核心数据结构:
 *   1. 屏幕初始化回调注册表 (s_screen_registry): name -> init_fn
 *   2. 屏幕栈 (s_screen_stack): 记录当前导航路径
 *
 * 页面切换策略:
 *   - screen_mgr_to():     创建新screen, 入栈, lv_scr_load()
 *   - screen_mgr_replace(): 创建新screen, 替换栈顶, 删除旧screen
 *   - screen_mgr_back():   出栈, 激活前一个screen, 删除当前screen
 *   - screen_mgr_switch(): 带close参数的切换
 *
 * 线程安全: 仅在 GUI 线程中调用。
 *
 * @date  2026-05-20
 * @note  M003 里程碑
 */

#include "screen_manager.h"
#include "widget_registry.h"
#include <string.h>
#include <stdio.h>

/* ======================================================================
 * 内部数据结构
 * ====================================================================== */

typedef struct {
    char               name[SCREEN_NAME_MAX];
    screen_init_func_t init_fn;
} screen_reg_entry_t;

typedef struct {
    char       name[SCREEN_NAME_MAX];
    lv_obj_t  *screen;
} screen_stack_entry_t;

#define SCREEN_REG_MAX  (16)

static screen_reg_entry_t   s_screen_registry[SCREEN_REG_MAX];
static int                  s_screen_reg_count = 0;

static screen_stack_entry_t s_screen_stack[SCREEN_STACK_MAX];
static int                  s_stack_top = -1;

static lv_obj_t            *s_wm_dummy = NULL;

/* ======================================================================
 * 内部辅助
 * ====================================================================== */

static screen_init_func_t find_init_fn(const char *name)
{
    for (int i = 0; i < s_screen_reg_count; i++) {
        if (strcmp(s_screen_registry[i].name, name) == 0)
            return s_screen_registry[i].init_fn;
    }
    return NULL;
}

static lv_obj_t *create_screen(const char *name, void *ctx)
{
    screen_init_func_t init_fn = find_init_fn(name);
    if (init_fn == NULL) {
        printf("[screen_mgr] ERROR: no init_fn for '%s'\n", name);
        return NULL;
    }

    lv_obj_t *scr = lv_obj_create(NULL, NULL);
    if (scr == NULL) {
        printf("[screen_mgr] ERROR: lv_obj_create failed for '%s'\n", name);
        return NULL;
    }

    lv_obj_set_size(scr, LV_HOR_RES_MAX, LV_VER_RES_MAX);

    int ret = init_fn(scr, ctx);
    if (ret != 0) {
        printf("[screen_mgr] WARNING: init_fn '%s' returned %d\n", name, ret);
    }

    return scr;
}

static void copy_name(char *dst, const char *src)
{
    size_t len = strlen(src);
    if (len >= SCREEN_NAME_MAX) len = SCREEN_NAME_MAX - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

/* ======================================================================
 * 公共 API
 * ====================================================================== */

void screen_mgr_register(const char *name, screen_init_func_t init_fn)
{
    if (name == NULL || init_fn == NULL) return;

    for (int i = 0; i < s_screen_reg_count; i++) {
        if (strcmp(s_screen_registry[i].name, name) == 0) {
            s_screen_registry[i].init_fn = init_fn;
            return;
        }
    }

    if (s_screen_reg_count >= SCREEN_REG_MAX) {
        printf("[screen_mgr] ERROR: registry full (%d)\n", SCREEN_REG_MAX);
        return;
    }

    copy_name(s_screen_registry[s_screen_reg_count].name, name);
    s_screen_registry[s_screen_reg_count].init_fn = init_fn;
    s_screen_reg_count++;
}

void screen_mgr_init(void)
{
    s_stack_top = -1;
    memset(s_screen_stack, 0, sizeof(s_screen_stack));

    if (s_wm_dummy == NULL) {
        s_wm_dummy = lv_obj_create(NULL, NULL);
    }
}

int screen_mgr_to(const char *name)
{
    return screen_mgr_to_with_ctx(name, NULL);
}

int screen_mgr_to_with_ctx(const char *name, void *ctx)
{
    if (name == NULL) return -1;

    widget_reg_clear();

    lv_obj_t *scr = create_screen(name, ctx);
    if (scr == NULL) return -1;

    /* 栈满保护: 移除栈底 */
    if (s_stack_top >= SCREEN_STACK_MAX - 1) {
        printf("[screen_mgr] WARNING: stack full, removing bottom\n");
        if (s_screen_stack[0].screen) {
            lv_obj_del(s_screen_stack[0].screen);
        }
        memmove(&s_screen_stack[0], &s_screen_stack[1],
                sizeof(screen_stack_entry_t) * (SCREEN_STACK_MAX - 1));
        s_stack_top = SCREEN_STACK_MAX - 2;
    }

    s_stack_top++;
    copy_name(s_screen_stack[s_stack_top].name, name);
    s_screen_stack[s_stack_top].screen = scr;

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);

    printf("[screen_mgr] -> '%s' (depth %d)\n", name, s_stack_top + 1);
    return 0;
}

int screen_mgr_replace(const char *name)
{
    if (name == NULL) return -1;

    widget_reg_clear();

    lv_obj_t *scr = create_screen(name, NULL);
    if (scr == NULL) return -1;

    if (s_stack_top >= 0) {
        copy_name(s_screen_stack[s_stack_top].name, name);
        s_screen_stack[s_stack_top].screen = scr;
        /* old_auto_del=true: LVGL deletes old screen after animation */
        lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
    } else {
        s_stack_top = 0;
        copy_name(s_screen_stack[0].name, name);
        s_screen_stack[0].screen = scr;
        lv_scr_load(scr);
    }

    printf("[screen_mgr] replace -> '%s'\n", name);
    return 0;
}

int screen_mgr_switch(const char *name, bool close_current)
{
    if (close_current) {
        return screen_mgr_replace(name);
    } else {
        return screen_mgr_to(name);
    }
}

int screen_mgr_back(void)
{
    if (s_stack_top <= 0) {
        printf("[screen_mgr] WARNING: at root, cannot back\n");
        return -1;
    }

    lv_obj_t *old_scr = s_screen_stack[s_stack_top].screen;
    s_screen_stack[s_stack_top].screen = NULL;
    s_screen_stack[s_stack_top].name[0] = '\0';
    s_stack_top--;

    widget_reg_clear();

    lv_obj_t *prev_scr = s_screen_stack[s_stack_top].screen;
    if (prev_scr) {
        lv_scr_load_anim(prev_scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
        lv_obj_del_async(old_scr);
    }

    printf("[screen_mgr] <- '%s' (depth %d)\n",
           s_screen_stack[s_stack_top].name, s_stack_top + 1);
    return 0;
}

int screen_mgr_back_to_home(void)
{
    if (s_stack_top <= 0) return 0;

    for (int i = s_stack_top; i > 0; i--) {
        if (s_screen_stack[i].screen) {
            lv_obj_del(s_screen_stack[i].screen);
        }
        s_screen_stack[i].screen = NULL;
        s_screen_stack[i].name[0] = '\0';
    }

    s_stack_top = 0;
    widget_reg_clear();

    lv_obj_t *home = s_screen_stack[0].screen;
    if (home) {
        lv_scr_load(home);
    }

    printf("[screen_mgr] -> home '%s'\n", s_screen_stack[0].name);
    return 0;
}

const char *screen_mgr_get_top_name(void)
{
    if (s_stack_top < 0) return NULL;
    return s_screen_stack[s_stack_top].name;
}

lv_obj_t *screen_mgr_get_top_screen(void)
{
    if (s_stack_top < 0) return NULL;
    return s_screen_stack[s_stack_top].screen;
}

void screen_mgr_close(const char *name)
{
    if (name == NULL) return;

    for (int i = s_stack_top; i >= 0; i--) {
        if (strcmp(s_screen_stack[i].name, name) == 0) {
            if (s_screen_stack[i].screen) {
                lv_obj_del(s_screen_stack[i].screen);
            }
            for (int j = i; j < s_stack_top; j++) {
                s_screen_stack[j] = s_screen_stack[j + 1];
            }
            s_screen_stack[s_stack_top].screen = NULL;
            s_screen_stack[s_stack_top].name[0] = '\0';
            s_stack_top--;
            break;
        }
    }
}

/* ======================================================================
 * AWTK 兼容 wrapper
 * ====================================================================== */

lv_obj_t *window_manager(void)
{
    return s_wm_dummy;
}

lv_obj_t *window_manager_get_top_window(lv_obj_t *wm)
{
    (void)wm;
    return screen_mgr_get_top_screen();
}

lv_obj_t *window_manager_get_top_main_window(lv_obj_t *wm)
{
    (void)wm;
    return screen_mgr_get_top_screen();
}

lv_obj_t *widget_child(lv_obj_t *wm, const char *name)
{
    (void)wm;
    return widget_reg_find(name);
}

lv_obj_t *window_open_and_close(const char *name, lv_obj_t *to_close)
{
    (void)to_close;
    screen_mgr_replace(name);
    return screen_mgr_get_top_screen();
}

int window_manager_close_window_force(lv_obj_t *wm, lv_obj_t *win)
{
    (void)wm;
    if (win) lv_obj_del(win);
    return 0;
}

int window_manager_switch_to(lv_obj_t *wm, lv_obj_t *curr,
                             lv_obj_t *target, bool close)
{
    (void)wm;
    (void)curr;
    (void)target;
    (void)close;
    /* Full implementation deferred to M023 when navigator.c is rewritten */
    return 0;
}

int window_manager_back_to_home(lv_obj_t *wm)
{
    (void)wm;
    return screen_mgr_back_to_home();
}

int window_manager_back(lv_obj_t *wm)
{
    (void)wm;
    return screen_mgr_back();
}

int dialog_modal(lv_obj_t *win)
{
    (void)win;
    return 0;
}
