/**
*
* @file hcn_carlink_def.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/26 14:13
* @author och
*
*/
#ifndef __HCN_CARLINK_DEF_H__
#define __HCN_CARLINK_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hcn_config.h"

#ifdef  HCN_CARLINK_ENABLE  
   
///< 亿联配置： 启用：投屏参数配置，主要针对异性区域投屏，安全区域投屏
//#define HCN_CARLINK_SAFEAREA_ENABLE

#ifdef HCN_CARLINK_SAFEAREA_ENABLE
#define HCN_EC_screenPhysicsWidth 3.892913
#define HCN_EC_screenPhysicsHeight 3.705512

/**
 ECVideoView ec_v_view = {
    0,
    {1, 0, 0, HCN_LCD_EC_WIDTH, HCN_LCD_EC_HEIGHT}, // viewArea
    {1, 98, 147, 348, 270}  // safeArea
    
    mirrorCfg.viewConfig.viewGroup = &ec_v_view;    
    mirrorCfg.viewConfig.initArea = 1 ;
    mirrorCfg.viewConfig.viewCount = 1;
};
ECVideoView:viewArea 视图区域参数
ECVideoArea:present 如果存在，即为1，否则为0
ECVideoArea:originXPixels 视图区域相对显示器X轴偏移量
ECVideoArea:originYPixels 视图区域相对显示器Y轴偏移量
ECVideoArea:widthPixels 视图区域像素宽
ECVideoArea:heightPixels 视图区域像素高 */

#define HCN_EC_VIDEO_viewArea_present 1
#define HCN_EC_VIDEO_viewArea_originXPixels 0
#define HCN_EC_VIDEO_viewArea_originYPixels 0
#define HCN_EC_VIDEO_viewArea_widthPixels   (HCN_LCD_EC_WIDTH)
#define HCN_EC_VIDEO_viewArea_heightPixels  (HCN_LCD_EC_HEIGHT)

/**
ECVideoView:safeArea 安全区域参数
ECVideoArea:present 如果存在，即为1，否则为0
ECVideoArea:originXPixels 安全区域相对视图区域X轴偏移量
ECVideoArea:originYPixels 安全区域区域相对视图区域Y轴偏移量
ECVideoArea:widthPixels 安全区域区域像素宽
ECVideoArea:heightPixels 安全区域区域像素高 

mirrorCfg.viewConfig.initArea
mirrorCfg.viewConfig.viewCount
* */

#define HCN_EC_VIDEO_safeArea_present 1
#define HCN_EC_VIDEO_safeArea_originXPixels  (40)
#define HCN_EC_VIDEO_safeArea_originYPixels  (110)
#define HCN_EC_VIDEO_safeArea_widthPixels  (400)
#define HCN_EC_VIDEO_safeArea_heightPixels (270)

#define HCN_EC_VIEW_CONFIG_initArea  1  
#define HCN_EC_VIEW_CONFIG_viewCount  1

#endif

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_CARLINK_DEF_H__