/**
*
* @file hcn_bt_song_art_cover.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/11/05 12:04
* @author och
*
*/
#ifndef __HCN_BT_SONG_ART_COVER_H__
#define __HCN_BT_SONG_ART_COVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "carlink_cb/hcn_carlink_cb.h"

/**
 * @brief  根据专辑图片序号解析图片
 * @param  photo_index 图片序号
 * @return 无
 */
void hcn_parse_avrcp_abulm_cover(bt_music_song_art_cover_t * art_cover, 
                                int photo_index);

/**
 * @brief  释放专辑图片buffer(蓝牙连接断开的时候)
 * @param  无 
 * @return 无
 */
void hcn_free_song_art_cover_buff(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_BT_SONG_ART_COVER_H__