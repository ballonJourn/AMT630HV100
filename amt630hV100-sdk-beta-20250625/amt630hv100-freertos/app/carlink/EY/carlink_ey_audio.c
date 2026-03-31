#include <stdio.h>
#include "carlink_ey_audio.h"

static int ey_bt_play_state_callback(BT_PLAY_STATE_E state, unsigned short samplerate, unsigned char channel)
{
    printf("\r\ney_bt_play_state_callback state %d samplerate %d channel %d\r\n", state, samplerate, channel);
    return 0;
}

static int ey_bt_a2dp_pcm_data_callback(unsigned char* buffer, unsigned short length)
{
    return 0;
}

static int ey_bt_hfp_spk_pcm_data_callback(unsigned char* buffer, unsigned short length)
{
    
    return 0;
}

#if 0
static int ey_bt_hfp_mic_pcm_data_callback(unsigned char* buffer, unsigned short length)
{
    return 0;
}
#endif

int carlink_ey_audio_init()
{
    bt_sw_cfg_t bt_sw_cfg = {0};

    bt_sw_cfg.play_state_cb       = ey_bt_play_state_callback;
    bt_sw_cfg.a2dp_cb               = ey_bt_a2dp_pcm_data_callback;
    bt_sw_cfg.hfp_spk_cb           = ey_bt_hfp_spk_pcm_data_callback;
    //bt_sw_cfg.hfp_mic_cb           = ey_bt_hfp_mic_pcm_data_callback;

    fsc_bt_register_pcm_interface((void*)&bt_sw_cfg);
    return 0;
}