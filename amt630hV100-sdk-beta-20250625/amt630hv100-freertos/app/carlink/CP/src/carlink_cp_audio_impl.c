#include <FreeRTOS_POSIX.h>
#include <pthread.h>

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include <task.h>
#include "os_adapt.h"
#include "carlink_cp_priv.h"
#include "carplay.h"
#include "board.h"
#include "audio.h"


struct cp_audio_ctx
{
	int handle; 
	int type; 
	int rate; 
	int bits; 
	int channels;

	int start;
	int input;

	struct audio_device *play_handle;
	struct audio_device *rec_handle;

	pthread_t play_pid;
	pthread_t *play_pid_ptr;
	pthread_t rec_pid;
	pthread_t *rec_pid_ptr;
};

struct audio_node
{
	struct cp_audio_ctx entity;
	int isUse;
};
static struct audio_node gnode[8];
static pthread_mutex_t g_node_lock = {       \
        .xIsInitialized = pdFALSE,           \
        .xMutex = { { 0 } },                 \
        .xTaskOwner = NULL,                  \
        .xAttr = { .iType = 0 }              \
    };

uint64_t	UpTicks( void );


static struct cp_audio_ctx* get_free_audio_node()
{
	int i;
	struct cp_audio_ctx *pctx = NULL;
	struct audio_node* ptr= gnode;

	pthread_mutex_lock(&g_node_lock);
	for (i = 0; i < ARRAY_SIZE(gnode); i++) {
		if (!ptr[i].isUse) {
			ptr[i].isUse = 1;
			pctx = &ptr[i].entity;
			break;
		}
	}
	pthread_mutex_unlock(&g_node_lock);
	return pctx;
}

static struct cp_audio_ctx* get_audio_node_by_handle(int handle)
{
	int i;
	struct cp_audio_ctx *pctx = NULL;
	struct audio_node* ptr= gnode;

	pthread_mutex_lock(&g_node_lock);
	for (i = 0; i < ARRAY_SIZE(gnode); i++) {
		if (ptr[i].isUse) {
			struct cp_audio_ctx *tmp = &ptr[i].entity;
			if (tmp->handle == handle) {
				pctx = tmp;
				break;
			}
		}
	}
	pthread_mutex_unlock(&g_node_lock);
	return pctx;
}


static void release_audio_node(struct cp_audio_ctx *pctx)
{
	int i;
	struct audio_node* ptr= gnode;
	if (NULL == pctx)
		return;

	memset((void*)pctx, 0, sizeof(struct cp_audio_ctx));
	pthread_mutex_lock(&g_node_lock);
	for (i = 0; i < ARRAY_SIZE(gnode); i++) {
		if (pctx == &ptr[i].entity) {
			ptr[i].isUse = 0;
			break;
		}
	}
	pthread_mutex_unlock(&g_node_lock);
}

static void * _AudioStreamRecordThread( void *inArg )
{
	struct cp_audio_ctx* pctx = (struct cp_audio_ctx*)inArg;
	(void)pctx;
	return NULL;
}


static void * _AudioStreamPlayThread( void *inArg )
{
	struct cp_audio_ctx* pctx = (struct cp_audio_ctx*)inArg;
	uint8_t buffer[960];
	int buffer_len = 960;
	int frame;

	frame = 960 / (pctx->channels * pctx->channels / 8);

	process_play_stream(pctx->handle, buffer, buffer_len, frame, UpTicks());

	while(pctx->start) {
		process_play_stream(pctx->handle, buffer, buffer_len, frame, UpTicks());
		audio_dev_write(pctx->play_handle, buffer, buffer_len);
	}
	return NULL;
}


int carlink_cp_audio_start(int handle, int type, int rate, int bits, int channels)
{
	int ret = -1;
	struct cp_audio_ctx* pctx = NULL;
	struct audio_caps caps = {0};
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 4096 * 12);

	pctx = get_free_audio_node();
	if (NULL == pctx) {
		return -1;
	}
	
	pctx->bits 		= bits;
	pctx->channels 	= channels;
	pctx->rate 		= rate;
	pctx->type 		= type;
	pctx->handle 	= handle;

	if (type == AUDIO_TELEPHONE || type == AUDIO_RECOGNITION) {
		pctx->input = 1;
	}

	pctx->play_handle = audio_dev_open(AUDIO_FLAG_REPLAY);
	caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = rate;
    caps.udata.config.channels   = channels;
    caps.udata.config.samplebits = bits;
    audio_dev_configure(pctx->play_handle, &caps);
	
	pctx->start = 1;
	if (pctx->input) {
		pctx->rec_handle = audio_dev_open(AUDIO_FLAG_RECORD);
		caps.main_type               = AUDIO_TYPE_OUTPUT;
	    caps.sub_type                = AUDIO_DSP_PARAM;
	    caps.udata.config.samplerate = rate;
	    caps.udata.config.channels   = channels;
	    caps.udata.config.samplebits = bits;
	    audio_dev_configure(pctx->play_handle, &caps);
		ret = pthread_create( &pctx->rec_pid, &attr, _AudioStreamRecordThread, (void*)pctx );
		if (ret == 0) {
			pctx->rec_pid_ptr = &pctx->rec_pid;
		}
	}

	ret = pthread_create( &pctx->play_pid, &attr, _AudioStreamPlayThread, (void*)pctx );
	if (ret == 0) {
		pctx->play_pid_ptr = &pctx->play_pid;
	}

//exit:
	return ret;
}

void carlink_cp_audio_stop(int handle, int type)
{
	struct cp_audio_ctx *pctx = NULL;

	pctx = get_audio_node_by_handle(handle);

	if (NULL == pctx || !pctx->start)
		return;
	
	pctx->start = 0;

	if (pctx->play_pid_ptr) {
		pthread_join(pctx->play_pid, NULL);
		pctx->play_pid_ptr = NULL;
	}

	if (pctx->rec_pid_ptr) {
		pthread_join(pctx->rec_pid, NULL);
		pctx->rec_pid_ptr = NULL;
	}

	release_audio_node(pctx);
}

