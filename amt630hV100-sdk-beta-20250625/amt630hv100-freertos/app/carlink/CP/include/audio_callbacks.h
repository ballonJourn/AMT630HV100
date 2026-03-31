#ifndef _AUDIO_CALLBACKS_H
#define _AUDIO_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*audio_start_callback_f)(void *ctx, int handle, int type, int rate, int bits, int channels);
typedef void (*audio_stop_callback_f)(void *ctx, int handle, int type);
typedef void ( *audio_reset_DSP_mode_callback_f)(void *ctx, int mode);

typedef struct audio_callbacks
{
	audio_start_callback_f audio_start_callback;
	audio_stop_callback_f audio_stop_callback;
        audio_reset_DSP_mode_callback_f   audio_reset_DSP_mode_callback;
	void *ctx;
}audio_callbacks_t;

void audio_register_callbacks(void *cb);

#ifdef __cplusplus
}
#endif
#endif
