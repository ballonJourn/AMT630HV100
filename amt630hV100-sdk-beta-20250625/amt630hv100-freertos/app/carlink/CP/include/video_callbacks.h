#ifndef _VIDEO_CALLBACKS_H
#define _VIDEO_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif


typedef int (*video_start_callback_f)(void *ctx);
typedef void (*video_stop_callback_f)(void *ctx);
typedef int (*video_proc_data_callback_f)(void *ctx, char *buf, int len);

typedef struct video_callbacks
{
	video_start_callback_f video_start_callback;
	video_stop_callback_f video_stop_callback;
	video_proc_data_callback_f video_proc_data_callback;
	void *ctx;
}video_callbacks_t;

void video_register_callbacks(void *cb);

#ifdef __cplusplus
}
#endif
#endif
