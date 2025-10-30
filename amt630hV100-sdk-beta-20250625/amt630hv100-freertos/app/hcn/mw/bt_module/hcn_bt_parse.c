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
#include "console.h"
#include "bt_module/hcn_bt_parse.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"
#include "config/hcn_config.h"
#include "carlink_cb/hcn_carlink_cb.h"

//#define BT_STR_DEBUG 
#define DOWNLOAD_TIMER_PERIOD_MS   (3000)
#define BT_TASK_QUEUE_LENGTH       (10)

static QueueHandle_t bt_task_queue = NULL;
static bt_phone_book_t *bt_phonebook = NULL;

static bt_data_t g_bt_data = {0};
static bt_call_t g_bt_call = {0};

static int g_pb_count = 0;
static uint32_t last_switch_tick = 0;

static int bt_send_cmd(const char *buff) {
    char buffer[UART_BT_SEND_BUF_LEN] = {0};

    uint16_t len = (uint16_t)strlen(buff);
    if (len > (UART_BT_SEND_BUF_LEN - 2)) {
        hcn_log_error("The command send is too long!\r\n");
        return -1;
    }

    memcpy(buffer, buff, len);
    buffer[len] = 0x0D;
    buffer[len + 1] = 0x0A;

    hcn_log_info("bt send cmd buff:%s\r\n", buff);

    return console_send_atcmd(buffer, len + 2);
}

static void hcn_bt_switch(void) {
    if (xTaskGetTickCount() - last_switch_tick > pdMS_TO_TICKS(500)) {
        last_switch_tick = xTaskGetTickCount();
        if (g_bt_data.btSwitchState == 1) {
            ///< 当前是打开状态，就关闭
            bt_send_cmd("AT+PAGE=0");
        } else if (g_bt_data.btSwitchState == 0) {
            ///< 当前是关闭状态，就打开
             bt_send_cmd("AT+PAGE=1");
        }
    } 
}

void hcn_bt_switch_state(bool state) {
    if (g_bt_data.btSwitchState != state) {
        hcn_bt_switch();
    }
}

void hcn_bt_download_book() {
    static uint32_t timer = 0;
    if ((g_bt_data.btConnected == 1) && (xTaskGetTickCount() - timer > 5000)) {
        char buff[64] = {0};
        hcn_log_info("\r\n start download phone book!\r\n");
        snprintf(buff, sizeof(buff), "AT+PBDOWN=1,%d", BT_PHONE_BOOK_MAX_NUM);
        bt_send_cmd(buff);
        timer = xTaskGetTickCount();
    }
}

void hcn_bt_pick_up() {
    if (g_bt_call.btHfpState == INCOMING_CALL) {
        bt_send_cmd("AT+HFPANSW");
    }
}

void hcn_bt_hung_up() {
    if (g_bt_call.btHfpState >= OUTGOING_CALL) {
        bt_send_cmd("AT+HFPCHUP");
    }   
}

bool hcn_bt_is_Call() {
    return (g_bt_call.btHfpState >= OUTGOING_CALL ? true : false);
}

const bt_call_t* hcn_bt_get_call() {
    return &g_bt_call;
}

const bt_data_t* hcn_bt_get_data() {
    return &g_bt_data;
}

const char* hcn_bt_get_name() {
    extern bool carlink_ble_mac_addr_is_ready();
    if (carlink_ble_mac_addr_is_ready()) {
        return g_bt_data.btDevName;
    } else {
        return "HCN-NONE";
    }
}

static void clean_bt_phone_book(void) {
    g_pb_count = 0;
    hcn_log_info("\r\nClean phone book!\r\n");
    if (g_bt_data.btBookCount > 0) {
        g_bt_data.btBookCount = 0;
        if (bt_phonebook) {
            vPortFree(bt_phonebook);
            bt_phonebook = NULL;
        }
    }
}

static void find_name_contact(char *call1, char *call2) {
    if (call1 && (strcmp(g_bt_call.btCallNumber1, call1) != 0)) {
        ///< 如果号码有变化，遍历电话本，查看是否有匹配的号码，查找到联系人
        if (bt_phonebook && (g_bt_data.btPbState != PB_STATE_DOWNDING)) {
            for (int i = 0; i < g_bt_data.btBookCount; i++) {
                if (strcmp(bt_phonebook[i].number, call1) == 0) {
                    strcpy(g_bt_call.btCallPerson1, bt_phonebook[i].name);
                    break;
                }
            }
        }

        strncpy(g_bt_call.btCallNumber1, call1, 
                strlen(call1) < TEXT_PARAM_LEN ? \
                strlen(call1) : TEXT_PARAM_LEN - 1);
    }

    if (call2 && (strcmp(g_bt_call.btCallNumber1, call2) != 0)) {
        ///< 如果号码有变化，遍历电话本，查看是否有匹配的号码，查找到联系人
        if (bt_phonebook && (g_bt_data.btPbState != PB_STATE_DOWNDING)) {
            for (int i = 0; i < g_bt_data.btBookCount; i++) {
                if (strcmp(bt_phonebook[i].number, call2) == 0) {
                    strcpy(g_bt_call.btCallPerson2, bt_phonebook[i].name);
                    break;
                }
            }
        }

         strncpy(g_bt_call.btCallNumber2, call2, 
                strlen(call2) < TEXT_PARAM_LEN ? \
                strlen(call2) : TEXT_PARAM_LEN - 1);
    }
}

static void on_bt_dev_state_change(const char *state_str) {
    if (state_str == NULL) {
        hcn_log_error("\r\nbt state_str is NULL!\r\n");
        return;
    }

    int dev_state = atoi(state_str);
    g_bt_data.btDevState = dev_state;
    hcn_log_info("bt device state change:%d\r\n", dev_state);
}

static void on_phone_book_num(int count) {
    ///< +PBCNT 解析电话本条目数，count是条目数，一个条目下面可能有多个子条目，加上50
    int pb_count = count + 50;
    if (pb_count > BT_PHONE_BOOK_MAX_NUM) {
        pb_count = BT_PHONE_BOOK_MAX_NUM;
    }

    g_bt_data.btBookCount = 0;

    hcn_log_info("\r\npbcnt:%d, pb_count:%d\r\n", count, pb_count);
    if (pb_count != g_pb_count) {
        if (bt_phonebook) {
            vPortFree(bt_phonebook); 
            bt_phonebook = NULL;
        }

        g_pb_count = pb_count;
        bt_phonebook = pvPortMalloc(sizeof(bt_phone_book_t)*g_pb_count);
        if (bt_phonebook == NULL) {
            hcn_log_error("\r\npvPortMalloc phone book num failed!\r\n");
        }

        return;
    }

    if (bt_phonebook) {
        memset(bt_phonebook, 0, sizeof(bt_phone_book_t) * g_pb_count);
    }
}

static void on_phone_book_proc(char (*data)[TEXT_PARAM_LEN], int param_count) {
    if (param_count > 1 && g_bt_data.btConnected) {
        if (strcmp(data[0], "E") == 0) {
            hcn_log_info("\r\n bt phone book num:%d\r\n", g_bt_data.btBookCount);
            return;
        }

        ///< 保存电话本
        if (g_bt_data.btBookCount < g_pb_count) {
            if (bt_phonebook) {
                strncpy(bt_phonebook[g_bt_data.btBookCount].name, data[1], 
                        strlen(data[1]) < TEXT_PARAM_LEN ? \
                        strlen(data[1]) : TEXT_PARAM_LEN - 1);
                strncpy(bt_phonebook[g_bt_data.btBookCount].number, data[2], 
                        strlen(data[2]) < TEXT_PARAM_LEN ? \
                        strlen(data[2]) : TEXT_PARAM_LEN - 1);
            }
            g_bt_data.btBookCount++;
        }
    }
}

static void on_bt_hfp_state_proc(char (*state)[TEXT_PARAM_LEN], 
                                uint16_t param_count) {
    ///< +HFPSTAT= 4,10086
    hfp_state_e hfp_state =  UNSUPPORTED;
    if (param_count > 0) {
        hfp_state = (hfp_state_e)atoi(state[0]);
        hcn_log_info("\r\nhfp state is = %d\r\n", hfp_state);
    } 

    uint8_t bt_connect = hfp_state >= CONNECTED ? 1 : 0;
    if (bt_connect != g_bt_data.btConnected) {
        g_bt_data.btConnected = bt_connect;

        if (bt_connect) {
            ///< 蓝牙连接成功以后，开始下载电话本
            hcn_bt_download_book();
        } else {
            if (g_bt_data.btBookCount > 0) {
                clean_bt_phone_book();
            }
        }
    }
    
    if (g_bt_call.btHfpState != hfp_state) {
        if ((hfp_state <= CONNECTED) && (g_bt_call.btHfpState > CONNECTED)) {
            g_bt_call.btHfpState = hfp_state;
            memset(g_bt_call.btCallNumber1, 0, sizeof(g_bt_call.btCallNumber1));
            memset(g_bt_call.btCallPerson1, 0, sizeof(g_bt_call.btCallPerson1));
        }

        ///< 过滤掉微信电话状态，防止电话信息显示在主界面
        if (!strstr(state[1], "000000")) {
            g_bt_call.btHfpState = hfp_state;
            if (param_count > 2) {
                ///< 三方通话，解析电话号码
                find_name_contact(state[1], state[2]);
            } else if (param_count > 1) {
                ///< 通话，解析号码
                find_name_contact(state[1], NULL);
            }
        }
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

    //printf("\r\npos:%d, cmd_str:%s, prama_data:%s\r\n", pos, cmd_str, prama_data);

    if (strstr(cmd_str, "+IND_PAGE")) {
        g_bt_data.btSwitchState = (uint8_t)atoi(prama_data);
        static bool is_first = true;
        hcn_log_info("\r\nbt switch state:%d\r\n", g_bt_data.btSwitchState);
        ///< 默认开启蓝牙，进行测试
        if (is_first) {
            is_first = false;
            hcn_log_info("Switch bt open!\r\n");
            bt_send_cmd("AT+PAGE=1");
        } 
    } else if (strstr(cmd_str, "+DEVSTAT")) {
        on_bt_dev_state_change(prama_data);
    } else if (strstr(cmd_str, "+PBCNT")) {
        on_phone_book_num(atoi(prama_data));
    } else if (strstr(cmd_str, "+PWRSTAT")) {
        g_bt_data.btPowerState = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+PIN")) {
        memcpy(g_bt_data.btDevPin, prama_data, strlen(prama_data)\
         < TEXT_PARAM_LEN ? strlen(prama_data) : TEXT_PARAM_LEN);
    } else if (strstr(cmd_str, "+HFPSIG")) {
        int signal =  atoi(prama_data);
        if (signal >= 0 && signal <= 5 && g_bt_data.btSignal != signal) {
            g_bt_data.btSignal = signal;
        }
    } else if (strstr(cmd_str, "+HFPBATT")) {
        int battery = atoi(prama_data);
        if (battery >= 0 && battery <= 5 && 
            g_bt_data.btBatteryLevel != battery) {
            g_bt_data.btBatteryLevel = battery;    
        }
    } else if (strstr(cmd_str, "+HFPIBR")) {
        g_bt_data.btHfpIBR = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+HFPAUDIO")) {
        uint8_t audio_state = atoi(prama_data);
    } else if (strstr(cmd_str, "+A2DPSTAT")) {
        g_bt_data.btA2dpState = (uint8_t)atoi(prama_data);
    } else if (strstr(cmd_str, "+PBSTAT")) {
        g_bt_data.btPbState = (uint8_t)atoi(prama_data);
    } else {
        char param_array[UART_BT_PARAM_NUMBER][TEXT_PARAM_LEN] = {0};
        char delimit[2] = {0xFF, '\0'};

        uint16_t param_count = string_split(prama_data, delimit, param_array,
                                sizeof(param_array) / sizeof(param_array[0]));
        if (strstr(cmd_str, "+HFPSTAT")) {
            on_bt_hfp_state_proc(param_array, param_count);
        } else if (strstr(cmd_str, "+VER")) {
            bt_send_cmd("AT+ADDR");
        } else if (strstr(cmd_str, "+NAME")) {
            static bool is_firsend = true;
            hcn_log_info("\r\n bt name;%s\r\n ", param_array[0]);
            if (strstr(param_array[0], HCN_CUSTOMER_NAME)) {
                memcpy(g_bt_data.btDevName, param_array[0], \
                strlen(param_array[0]) < TEXT_PARAM_LEN ? \
                strlen(param_array[0]) : TEXT_PARAM_LEN);
            }

            if (is_firsend) {
                is_firsend = false;
                bt_send_cmd("AT+LEADDR");
            }
        } else if (strstr(cmd_str, "+LENAME")) {

        } else if (strstr(cmd_str, "+PBDATA")) {
            on_phone_book_proc(param_array, param_count);
        } else if (strstr(cmd_str, "+HFPDEV")) {
            memcpy(g_bt_data.btHfpAddr, param_array[1], \
                strlen(param_array[0]) < TEXT_PARAM_LEN ? \
                strlen(param_array[0]) : TEXT_PARAM_LEN);
            memcpy(g_bt_data.btConnectDevName, param_array[1], \
                strlen(param_array[1]) < BT_CONNECT_DEV_NAME_LEN ? \
                strlen(param_array[1]) : BT_CONNECT_DEV_NAME_LEN);
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

    if (xTaskCreate(bt_msg_parse_task, "bt_msg_parse_task", 
                    configMINIMAL_STACK_SIZE * 4, NULL, 
                    configMAX_PRIORITIES / 2, NULL) != pdPASS) {
        vQueueDelete(bt_task_queue);
        bt_task_queue = NULL;

        hcn_log_error("Failed to create BT message task\n");
        return -1;
    }
    
    hcn_log_info("\r\nbt module init ok!\r\n");

    return 0;
}
