/**
 * @file hcn_dc002_config.h
 * @brief HCN_DC002 (1024x600) 专用配置覆盖
 * 编译DC002时，通过 -DHCN_DC002_ENABLE 激活
 */
#ifndef __HCN_DC002_CONFIG_H__
#define __HCN_DC002_CONFIG_H__

#ifdef HCN_DC002_ENABLE

#undef  HCN_LCD_WIDTH
#define HCN_LCD_WIDTH               1024

#undef  HCN_LCD_HEIGHT
#define HCN_LCD_HEIGHT              600

#undef  OSD_WIDTH
#define OSD_WIDTH                   1024

#undef  OSD_HEIGHT
#define OSD_HEIGHT                  600

#undef  HCN_LCD_EC_WIDTH
#define HCN_LCD_EC_WIDTH            (1024)

#undef  HCN_LCD_EC_HEIGHT
#define HCN_LCD_EC_HEIGHT           (600)

/* LCD时序 - 根据实际1024x600屏幕datasheet调整! */
#undef  HCN_LCD_TIMING_VBP
#define HCN_LCD_TIMING_VBP          23

#undef  HCN_LCD_TIMING_VFP
#define HCN_LCD_TIMING_VFP          12

#undef  HCN_LCD_TIMING_VSW
#define HCN_LCD_TIMING_VSW          10

#undef  HCN_LCD_TIMING_HBP
#define HCN_LCD_TIMING_HBP          160

#undef  HCN_LCD_TIMING_HFP
#define HCN_LCD_TIMING_HFP          160

#undef  HCN_LCD_TIMING_HSW
#define HCN_LCD_TIMING_HSW          70

#undef  HCN_LCD_CLK_FREQ
#define HCN_LCD_CLK_FREQ            51200000

/* 内存: 1024x600 framebuffer比800x480大1.6x */
#undef  HCN_configTOTAL_HEAP_SIZE
#define HCN_configTOTAL_HEAP_SIZE   (((size_t)((26.5) * 1024 * 1024)))

#undef  HCN_VG_HEAP_SIZE
#define HCN_VG_HEAP_SIZE            ((14) * 1024 * 1024)

#undef  HCN_AWTK_HEAP_SIZE
#define HCN_AWTK_HEAP_SIZE          ((22) * 1024 * 1024)

#endif /* HCN_DC002_ENABLE */
#endif /* __HCN_DC002_CONFIG_H__ */
