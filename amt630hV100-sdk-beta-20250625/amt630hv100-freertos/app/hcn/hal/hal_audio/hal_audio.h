/**
*
* @file hal_audio.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/06
* @author zjc
*
*/
#ifndef __HAL_AUDIO_H__
#define __HAL_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "audio.h"

int hal_audio_set_volume(uint32_t v);
void hal_audio_write_audio_buf(struct audio_device *audio, const void *buffer, size_t size);
void hal_audio_init();
void hal_audio_uninit();

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HAL_AUDIO_H__