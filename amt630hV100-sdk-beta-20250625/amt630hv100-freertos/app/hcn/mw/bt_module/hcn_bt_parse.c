/**
*
* @file hcn_bt_parse.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/26 09:35
* @author och
*
*/

#include <string.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "task.h"
#include "timers.h"
#include "bt_module/hcn_bt_parse.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"
#include "carlink_cb/hcn_carlink_cb.h"

//#define BT_STR_DEBUG 
#define DOWNLOAD_TIMER_PERIOD_MS   (3000)
#define BT_TASK_QUEUE_LENGTH      (10)

static QueueHandle_t bt_task_queue = NULL;
static TimerHandle_t dowload_timer = NULL;
static bt_data_t g_bt_data = {0};

static void download_bt_phonebook_timer(TimerHandle_t xTimer) {
    hcn_log_info("bt download phonebook timer!\n");
}

static void on_bt_dev_state_change(const char *state_str) {
    if (state_str == NULL) {
        hcn_log_error("\r\nbt state_str is NULL!\r\n");
        return;
    }

    int dev_state = atoi(state_str);
    g_bt_data.btDevState = (dev_state_e)dev_state;
    hcn_log_info("bt device state change:%d\r\n", dev_state);
}

static void on_phone_book_num(const char *str) {
    if (str) {
        int pb_count = atoi(str);
        g_bt_data.btBookCount = (uint16_t)pb_count;
    }
}

static void on_bt_str_parse(char *at_str) {
    if (at_str == NULL) {
        hcn_log_error("\r\nbt at_str is NULL!\r\n");
        return;
    }

    ///< 无对应回复格式的AT指令直接丢弃
    if (strchr(at_str, '=') == NULL) {
        hcn_log_error("\r\nbt at_str format error!\r\n");
        return;
    }

#ifdef BT_STR_DEBUG
    hcn_log_info("bt str:%s", at_str);
#endif

    char prama_data[UART_BT_MSG_MAX_LEN] = {0};
    char cmd_str[20] = {0};

    strcpy(prama_data, strchr(at_str, '=') + 1);
    int pos = strchr(at_str, '=') - (strchr(at_str, '+'));

    char *temp_data = strchr(at_str, '+');
    if (substring(cmd_str, temp_data, 0, pos) == NULL) {
        hcn_log_error("\r\nbt substring cmd_str failed!\r\n");
        return;
    }

    printf("\r\npos:%d, cmd_str:%s, prama_data:%s\r\n", pos, cmd_str, prama_data);

    if (strstr(cmd_str, "+PAGE")) {
        g_bt_data.btConnHCNtability = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+DEVSTAT")) {
        hcn_log_info("bt device state:%s\r\n", prama_data);
        on_bt_dev_state_change(prama_data);
    } else if (strstr(cmd_str, "+PBCNT")) {
        on_phone_book_num(prama_data);
    } else if (strstr(cmd_str, "+HFPSIG")) {
        int signal =  atoi(prama_data);
         if (signal >= 0 && signal <= 5 && g_bt_data.btSignal != signal) {
            g_bt_data.btSignal = signal;
         }
    }
}

static void bt_msg_parse_task(void *param) {
    char *bt_msg;

    for (;;) {
        if (xQueueReceive(bt_task_queue, &bt_msg, portMAX_DELAY) != pdPASS) {
            hcn_log_error("\r\nbt_msg_parse_task xQueueReceive failed!\r\n");
            continue;
        }

        on_bt_str_parse(bt_msg);
        vPortFree(bt_msg);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

int bt_msg_task_add(char *bt_msg, uint16_t len) {
    if (bt_task_queue == NULL) {
        hcn_log_error("bt task queue is NULL!\n");
        return -1;
    }

    char *msg = pvPortMalloc(len + 1);
    if (msg == NULL) {
        hcn_log_error("pvPortMalloc bt msg failed!\n");
        return -1;
    }

    memset(msg, 0, len + 1);
    memcpy(msg, bt_msg, len);
    msg[len] = '\0';

    if (xQueueSend(bt_task_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        hcn_log_error("bt_msg_task_add xQueueSend failed!\n");
        vPortFree(msg);
        return -1;
    }

    return 0;
}

int bt_module_init(void) {
    if (bt_task_queue) {
        hcn_log_info("bt module has initialized");
        return 0;
    }

    if (bt_task_queue == NULL) {
        bt_task_queue = xQueueCreate(BT_TASK_QUEUE_LENGTH, sizeof(char*));
        if (bt_task_queue == NULL) {
            hcn_log_error("Failed to create BT task queue");
            return -1;
        }
    }

    if (dowload_timer == NULL) {
        dowload_timer = xTimerCreate("bt_download_timer",
                                     pdMS_TO_TICKS(DOWNLOAD_TIMER_PERIOD_MS),
                                     pdFALSE,
                                     (void*)0,
                                     download_bt_phonebook_timer);
        if (dowload_timer == NULL) {
            vQueueDelete(bt_task_queue);
            bt_task_queue = NULL;
            hcn_log_error("Failed to create BT download timer\n");
            return -1;
        }

        if (xTaskCreate(bt_msg_parse_task, "bt_msg_parse_task", 
                        configMINIMAL_STACK_SIZE * 4, NULL, 
                        configMAX_PRIORITIES / 4, NULL) != pdPASS) {
            xTimerDelete(dowload_timer, 0);
            dowload_timer = NULL;

            vQueueDelete(bt_task_queue);
            bt_task_queue = NULL;

            hcn_log_error("Failed to create BT message task\n");
            return -1;
        }
    }

    hcn_log_info("\r\nbt module init ok!\r\n");

    return 0;
}
