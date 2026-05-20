/**
 * @file widget_registry.c
 * @brief 控件名称→lv_obj_t* 注册表实现
 *
 * 使用简单的线性数组 + 字符串比较。在256条目规模下, 线性查找
 * 的开销远小于一次 LVGL 渲染帧, 不需要哈希表的复杂度。
 *
 * @date  2026-05-20
 * @note  M002 里程碑
 */

#include "widget_registry.h"
#include <string.h>
#include <stdio.h>

/** 控件名称最大长度 (含终止符) */
#define WIDGET_NAME_MAX_LEN  (32)

/** 注册表条目 */
typedef struct {
    char       name[WIDGET_NAME_MAX_LEN];
    lv_obj_t  *obj;
} widget_reg_entry_t;

/** 注册表存储 (静态分配, 避免堆碎片) */
static widget_reg_entry_t s_registry[WIDGET_REG_MAX_ENTRIES];
static int s_registry_count = 0;

void widget_reg_add(const char *name, lv_obj_t *obj)
{
    if (name == NULL || obj == NULL) return;

    /* 先查找是否已存在, 如存在则覆盖 */
    for (int i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i].name, name) == 0) {
            s_registry[i].obj = obj;
            return;
        }
    }

    /* 新增条目 */
    if (s_registry_count >= WIDGET_REG_MAX_ENTRIES) {
        printf("[widget_reg] ERROR: registry full (%d entries), cannot add '%s'\n",
               WIDGET_REG_MAX_ENTRIES, name);
        return;
    }

    size_t len = strlen(name);
    if (len >= WIDGET_NAME_MAX_LEN) {
        printf("[widget_reg] WARNING: name '%s' truncated to %d chars\n",
               name, WIDGET_NAME_MAX_LEN - 1);
        len = WIDGET_NAME_MAX_LEN - 1;
    }

    memcpy(s_registry[s_registry_count].name, name, len);
    s_registry[s_registry_count].name[len] = '\0';
    s_registry[s_registry_count].obj = obj;
    s_registry_count++;
}

lv_obj_t *widget_reg_find(const char *name)
{
    if (name == NULL) return NULL;

    for (int i = 0; i < s_registry_count; i++) {
        if (strcmp(s_registry[i].name, name) == 0) {
            return s_registry[i].obj;
        }
    }

    return NULL;
}

void widget_reg_clear(void)
{
    s_registry_count = 0;
    memset(s_registry, 0, sizeof(s_registry));
}

lv_obj_t *widget_lookup(lv_obj_t *parent, const char *name, bool recursive)
{
    (void)parent;
    (void)recursive;
    return widget_reg_find(name);
}

void widget_reg_dump(void)
{
    printf("[widget_reg] === Registry Dump (%d/%d entries) ===\n",
           s_registry_count, WIDGET_REG_MAX_ENTRIES);
    for (int i = 0; i < s_registry_count; i++) {
        printf("  [%3d] %-32s -> %p\n",
               i, s_registry[i].name, (void *)s_registry[i].obj);
    }
    printf("[widget_reg] === End Dump ===\n");
}
