/**
*
* @file hcn_adc_key.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/25 09:21
* @author och
*
*/

#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "key_module/hcn_adc_key.h"
#include "key_module/hcn_key_common.h"
#include "log/hcn_log.h"
#include "dashboard_state/hcn_dev_state.h"

#ifdef HCN_ADC_KEY_ENABLE

#define HCN_KEYPAD_INPUT_LEN (64)
#define HCN_KEYPAD_PERIOD (20)
#define KEY_LONG_PRESS_TIME (2000)  ///< 2s
#define GET_OS_TIME()    xTaskGetTickCount()

static QueueHandle_t keypad_mq = NULL;
static uint32_t key_tick = 0;
static bool is_long_press = false;

void send_keypad_event_isr(int key, int state) {
    keypad_event_t indata = {0};
    indata.key_type = key;
    indata.state = state;
    xQueueSendFromISR(keypad_mq, &indata, 0);
}

void send_keypad_event(keypad_event_t *indata) {
    xQueueSend(keypad_mq, indata, 0);
}

static void keypad_input_process(keypad_event_t *data) {
    
    if (get_check_self_state() == CHECK_SELF_STATE_SUCCESS) {
        if (!key_tick) {
            key_tick = GET_OS_TIME();
        }
        if (data->state){
            ///< press
            if (!is_long_press && key_tick) {
                if (GET_OS_TIME() - key_tick > KEY_LONG_PRESS_TIME 
                    && ADC_KEY_UP == data->key_type) {
                    is_long_press = true;
                    send_key_event(UP_KEY_LONG_PR);
                    printf("long press up key.\n");
                } 
                else if(GET_OS_TIME() - key_tick > KEY_LONG_PRESS_TIME 
                        && ADC_KEY_DOWN == data->key_type) {
                    is_long_press = true; 
                    send_key_event(MODE_KEY_LONG_PR);
                    printf("long press down key.\n");
                }
                else if(GET_OS_TIME() - key_tick > KEY_LONG_PRESS_TIME 
                        && ADC_KEY_ESC == data->key_type) {
                    is_long_press = true;     
                    send_key_event(SET_KEY_LONG_PR);
                    printf("long press enter key.\n");
                }
                else if(GET_OS_TIME()- key_tick > KEY_LONG_PRESS_TIME 
                    && ADC_KEY_LEFT == data->key_type) {
                    is_long_press = true;   
                    send_key_event(BACK_KEY_LONG_PR);
                    printf("long press back key\n");
                }
            }
        } else {
            ///< release
            key_tick = 0;
            if (!is_long_press) {
                if (ADC_KEY_UP == data->key_type) {	
                    send_key_event(UP_KEY_SHORT_PR);
                    printf("short press up key\n");
                } else if (ADC_KEY_DOWN == data->key_type){	
                    send_key_event(MODE_KEY_SHORT_PR);
                    printf("short press down key.\n"); 
                } else if (ADC_KEY_ESC == data->key_type) {	
                    send_key_event(SET_KEY_SHORT_PR);
                    printf("short press enter key.\n");
                } else if(ADC_KEY_LEFT == data->key_type) { 
                    send_key_event(BACK_KEY_SHORT_PR);                       
                    printf("short press back key\n");
                }
            }
            is_long_press = false;
        }
    }
}

static void keypad_thread(void *param) {
    keypad_event_t data;

    for (;;) {
    if (xQueueReceive(keypad_mq, &data, portMAX_DELAY) == pdPASS) {
        keypad_input_process(&data);
     }
  }
}

int hcn_keypad_msg_init(void) {
    keypad_mq = xQueueCreate(HCN_KEYPAD_INPUT_LEN, sizeof(keypad_event_t));
    if (keypad_mq == NULL) {
        hcn_log_error("create keypad message queue fail.\n");
        vQueueDelete(keypad_mq);
        keypad_mq = NULL;

        return -1;
    }

    if (xTaskCreate(keypad_thread, "keypad_thread", configMINIMAL_STACK_SIZE * 2,
                    NULL, configMAX_PRIORITIES / 3, NULL) != pdPASS) {
        hcn_log_error("create keypad task fail.\n");

        vQueueDelete(keypad_mq);
        keypad_mq = NULL;             

        return -1;
    }

    hcn_log_info("hcn keypad init ok!\n");

    return 0;
}

#endif