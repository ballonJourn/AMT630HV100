
#include <FreeRTOS_POSIX.h>
#include <task.h>
#include <pthread.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "ff_sfdisk.h"
#include "carlink_video.h"
#include "os_adapt.h"

#include "carlink_common.h"

static QueueHandle_t     carlink_event_queue = NULL;
static struct ICalinkEventCallbacks* gcarlink_event_cbs[8];
static int  gcarlink_event_cbs_count;
static int g_comm_init_flag = 0;
static pthread_mutex_t carlink_com_locker =
{
        .xIsInitialized = pdFALSE,
        .xMutex = { { 0 } },
        .xTaskOwner = NULL,
        .xAttr = { .iType = 0 }
};

void pthread_key_system_init();

void carlink_rfcomm_data_read_hook(void* buf, int len)
{
	struct ICalinkEventCallbacks *pcb;
	int i;
	for (i = 0; i < gcarlink_event_cbs_count; i++) {
		pcb = gcarlink_event_cbs[i];
		if (pcb && pcb->onEvent) {
			pcb->rfcomm_data_read(pcb->cb_ctx, buf, len);
		}
	}
}


void carlink_notify_event(struct carlink_event *ev)
{
	if (CARLINK_EVENT_NONE != ev->type && NULL != carlink_event_queue) {
		xQueueSend(carlink_event_queue, ev, 0);
	}
}

void carlink_notify_event_isr(struct carlink_event *ev)
{
	if (CARLINK_EVENT_NONE != ev->type && NULL != carlink_event_queue) {
		xQueueSendFromISR(carlink_event_queue, ev, 0);
	}
}

void carlink_send_key_event(uint8_t key, bool pressed)
{
	struct carlink_event ev = {0};
	ev.type = CARLINK_EVENT_KEY_EVENT;
	ev.disable_filter = true;
	ev.u.para[0] = key;
	ev.u.para[1] = (uint8_t)pressed;

	carlink_notify_event_isr(&ev);
}


static void carlink_event_proc(void* param)
{
	struct carlink_event ev;
	struct ICalinkEventCallbacks *pcb;
	int i;

	(void)param;

	if (NULL == carlink_event_queue)
		return;

    while(1) {
        if (xQueueReceive(carlink_event_queue, &ev, portMAX_DELAY) != pdPASS) {
            printf("%s xQueueReceive err!\r\n", __func__);
            continue;
        }

		for (i = 0; i < gcarlink_event_cbs_count; i++) {
			pcb = gcarlink_event_cbs[i];
			if (pcb && pcb->onEvent) {
				pcb->onEvent(pcb->cb_ctx, &ev);
			}
		}
    }
}

void carlink_register_event_callbacks(const struct ICalinkEventCallbacks *pcb)
{
	if (gcarlink_event_cbs_count < 8) {
		gcarlink_event_cbs[gcarlink_event_cbs_count++] = (struct ICalinkEventCallbacks *)pcb;
	} else {
		
	}
}

int carlink_common_init()
{
	//FF_Disk_t *sfdisk = NULL;
	BaseType_t ret = -1;

	pthread_mutex_lock(&carlink_com_locker);
	if (g_comm_init_flag) {
		goto exit;
	}

	carlink_ey_video_init();

	carlink_event_queue = xQueueCreate(16, sizeof(struct carlink_event));
	if (NULL == carlink_event_queue) {
		printf("%s:%d failed\n", __func__, __LINE__);
		goto exit;
	}
	
	pthread_key_system_init();
#if 0
	sfdisk = FF_SFDiskInit("/sf");
	if (!sfdisk) {
		printf("FF_SFDiskInit fail.\r\n");
		//return;
	}
#endif
	ret = xTaskCreate(carlink_event_proc, "cl_ev_proc", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
	g_comm_init_flag = 1;
	
exit:
	if (ret == -1) {
	}
	pthread_mutex_unlock(&carlink_com_locker);

	return ret;
	
}