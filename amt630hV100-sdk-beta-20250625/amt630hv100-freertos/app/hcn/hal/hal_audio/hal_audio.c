/**
*
* @file hal_audio.c
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
#include <stdio.h>
#include "audio.h"
#include "board.h"
#include "hal_audio/hal_audio.h"


#if 0
static  struct audio_caps caps = {0};
#endif

static	struct audio_device *audio;

int hal_audio_set_volume(uint32_t v) {

    //需要验证芯片自带的volume 和芯片 外设音频控制
    
    return 0;
}

void hal_audio_write_audio_buf(struct audio_device *audio, const void *buffer, size_t size) {
    
    audio_dev_write(audio, buffer, size);

}

void hal_audio_init() {

    printf("audio init aw88082 \n");
    /*1.此处添加外设芯片初始化接口：
     * aw88082qnr芯片外设初始化,寄存器配置
    */
     aw88082_init();

    
    /*2.内部codec的基本初始化操作
    * I2S0为输出：设置采样率、通道、采样位数等音频参数信息 
    */
#if 0
    audio = audio_dev_open(AUDIO_FLAG_REPLAY);
	if (!audio)
	{
		printf("Open audio device fail.\n");
		return;
	}

   
    caps.main_type               = AUDIO_TYPE_OUTPUT;                           /* 输出类型（播放设备 ）*/
    caps.sub_type                = AUDIO_DSP_PARAM;                             /* 设置所有音频参数信息 */
    caps.udata.config.samplerate = 48000;                                       /* 采样率 */
    caps.udata.config.channels   = 2;                                           /* 采样通道 */
    caps.udata.config.samplebits = 16;                                          /* 采样位数 */
    audio_dev_configure(audio, &caps);

    aw_set_volume(0x0F);
#endif
}


void hal_audio_uninit() {
    /* 关闭 Audio 设备 */
    audio_dev_close(audio, AUDIO_FLAG_REPLAY);
}
