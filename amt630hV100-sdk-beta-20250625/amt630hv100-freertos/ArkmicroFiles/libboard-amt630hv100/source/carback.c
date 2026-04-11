#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "chip.h"
#include "board.h"

static TaskHandle_t carback_task = NULL;
//static SemaphoreHandle_t carback_mutex;
static int carback_status = 0;

void notify_enter_carback(void)
{
	if (carback_task)
		xTaskNotify(carback_task, 1, eSetValueWithOverwrite);
}

void notify_exit_carback(void)
{
	if (carback_task)
		xTaskNotify(carback_task, 0, eSetValueWithOverwrite);
}

int get_carback_status(void)
{
	int status;

	portENTER_CRITICAL();
	status = carback_status;
	portEXIT_CRITICAL();
	
	return status;
}

static void carback_thread(void *param)
{
	uint32_t ulNotifiedValue;

	//carback_mutex = xSemaphoreCreateMutex();
	
#if	defined(VIDEO_DECODER_RN6752) 		
	rn6752_init();
#elif defined(VIDEO_DECODER_ARK7116)
	ark7116_init();
#endif

	for (;;) {
		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
			0xffffffff, /* Reset the notification value to 0 on exit. */
			&ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
			portMAX_DELAY);

		if (ulNotifiedValue && !carback_status) {
			printf("enter carback.\n");
			ItuConfigPara para = {0};

			para.itu601 = 1;
			para.out_format = ITU_YUV420;
			para.yuv_type = ITU_Y_UV;
			para.in_width = VIN_WIDTH;
			para.in_height = VIN_HEIGHT;
			para.in_height = VIN_HEIGHT - 2;

#ifdef REVERSE_TRACK
			para.out_width = 704;
			para.out_height = 440;
#else
			para.out_width = LCD_WIDTH;
			para.out_height = LCD_HEIGHT;
#endif
			itu_config(&para);
			itu_start();
			carback_status = 1;
		} else if (!ulNotifiedValue && carback_status) {
			printf("exit carback.\n");
			itu_stop();
			carback_status = 0;
		}
	}
}

static void carback_test_thread(void *param)
{
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(10000));
		notify_enter_carback();
		vTaskDelay(pdMS_TO_TICKS(30000));
		notify_exit_carback();
	}
}

int carback_init(void)
{
	/* Create a task to play animation */
	if (xTaskCreate(carback_thread, "carback", configMINIMAL_STACK_SIZE, NULL,
		configMAX_PRIORITIES - 3, &carback_task) != pdPASS) {
		printf("create carback task fail.\n");
		carback_task = NULL;
		return -1;
	}

	xTaskCreate(carback_test_thread, "ctest", configMINIMAL_STACK_SIZE, NULL,
		configMAX_PRIORITIES / 2, NULL);

	return 0;	
}
