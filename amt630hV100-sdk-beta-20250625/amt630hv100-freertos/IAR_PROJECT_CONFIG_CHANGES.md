# IAR 工程配置变更清单 (手动在 IAR IDE 中执行)

## 1. 新工程或修改 awtk.ewp

### CCDefines 修改
**移除:**
- AWTK
- HMI_AWTK
- XM_HMI_HOST
- USE_GUI_MAIN
- LCD=VG_GPU
- NANOVG_BACKEND=VG
- 所有 WITH_NANOVG*, WITH_VGCANVAS, NANOVG_*, WITH_VG_GPU
- 所有 WITH_STB_IMAGE, STBTT_STATIC, STB_IMAGE_STATIC
- WITH_LIBPNG_IMAGE, PNG_NO_CONFIG_H
- WITH_FT_FONT, USE_TTF_FONT_DATA_CACHE
- WITH_ASSET_LOADER, WITH_FS_RES, WITH_ASSET_LOADER_ZIP
- WITH_IME_PINYIN, __WITH_IME_SPINYIN, __WITH_IME_NULL
- ENABLE_MEM_LEAK_CHECK
- WITH_UNICODE_BREAK, WITH_DESKTOP_STYLE
- WITH_DATA_READER_WRITER
- HAS_PTHREAD=1, HAVE_CONFIG_H, HAVE_CLOCK_GETTIME
- HAS_GET_TIME_US64
- LZ4_DISABLE_DEPRECATE_WARNINGS, MINIZ_NO_STDIO, MINIZ_NO_TIME
- __inline=inline
- _WITH_BITMAP_BGR565, _WITH_FB_BGR565=1
- VG_DRAW_IMAGE_DIRECT, VG_DRAW_IMAGE_REPEAT
- DIRTY_RECTS_CLIP_SUPPORT, VGCANVAS_DIRTY_RECTS_SUPPORT
- __WITH_G2D

**保留:**
- AMT630HV100
- VG_DRIVER
- FREERTOS=1
- POWERPAC

**自动定义 (通过 hcn_config.h):**
- HCN_SCREEN_ENABLE (已在 hcn_config.h 中 #define)

### CCIncludePath2 修改
**移除:** 所有 lib\awtk\ 路径 (约15条)

**添加:**
- `$PROJ_DIR$\..\app\hcn\ui\HCN_DC001\src`
- `$PROJ_DIR$\..\lib\LittlevGL`
- `$PROJ_DIR$\..\lib\LittlevGL\lvgl`
- `$PROJ_DIR$\..\app\hcn\ui\HCN_DC001\3rd`

### 链接脚本
改为: `$PROJ_DIR$\amt630hv100_hcn_lvgl.icf`

### 源文件组
**移除:** lib/awtk/ 下所有 .c 文件 (约3170个)
**添加:**
- `app/main_hcn_lvgl.c`
- `app/hcn/ui/HCN_DC001/src/lvgl_compat/*.c` (4个文件)
- `app/hcn/ui/HCN_DC001/src/ui_build/*.c`
- lib/LittlevGL/ 下的 LVGL 源文件 (或使用预编译的 lvgl.a)
