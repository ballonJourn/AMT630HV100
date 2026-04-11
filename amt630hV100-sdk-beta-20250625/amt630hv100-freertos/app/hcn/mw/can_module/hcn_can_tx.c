/**
*
* @file hcn_can_tx.c
*
* @brief High efficiency CAN transmission module with precise timing
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 09:40
* @author och
*
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"
#include "chip.h"
#include "can_module/hcn_can_tx.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"

#define HCN_CAN_TX_ENABLE

#ifdef HCN_CAN_TX_ENABLE

typedef void (*tx_fun_handle)(CanPort_t *cap, void *param);

typedef struct {
    uint32_t id;
    uint64_t start;
    uint32_t cycle;
    tx_fun_handle fun;
    void *param;
} can_tx_cycle_t;

static void tx_450_msg_fun(CanPort_t *cap, void *param);

static can_tx_cycle_t can_tx_cycle_msg[] = {
    // id,     start, cycle,     function,          param
    {0x450, 5, 100, tx_450_msg_fun, NULL},
};

static const uint8_t can_tx_cycle_count = 
    sizeof(can_tx_cycle_msg) / sizeof(can_tx_cycle_msg[0]);

void tx_450_msg_fun(CanPort_t *cap, void *param) {
    static uint8_t tcs_on_of_status = 0;

    CanMsg txmsg = {0};
    txmsg.IDE = CAN_Id_Standard;
    txmsg.DLC = 8;
    txmsg.StdId = 0x450;

    tcs_on_of_status = !tcs_on_of_status;
    txmsg.Data[0] = tcs_on_of_status;

    iCanWrite(cap, &txmsg, 1, 0);
}

static void can_tx_cycle_msg_proc(CanPort_t *cap) {
    static TickType_t last_execution_time = 0;
    TickType_t current_time = xTaskGetTickCount();
    
    if (current_time == last_execution_time) {
        return;
    }
    
    last_execution_time = current_time;
    for (uint8_t i = 0; i < can_tx_cycle_count; i++) {
        can_tx_cycle_t *p = &can_tx_cycle_msg[i];
        if (p) {
            if (current_time >= p->start) {
                p->start = current_time + p->cycle; 
                p->fun(cap, p->param);
            }
        }
    }
}

static void can_txdemo_thread(void *param) {
    CanPort_t *cap = (CanPort_t *)param;

    for (;;) {
        can_tx_cycle_msg_proc(cap);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

#endif

int can_msg_tx_msg_init(CanPort_t *cap) {
    if (!cap) {
        hcn_log_error("can port is null!\n");
        return -1;
    }

#ifdef HCN_CAN_TX_ENABLE
    int ret = -1;
    ret = xTaskCreate(can_txdemo_thread, 
                     "can_tx_cycle", 
                     configMINIMAL_STACK_SIZE,
                     cap, 
                    configMAX_PRIORITIES / 3 + 1,
                     NULL);
    
    if (ret != pdPASS) {
        hcn_log_error("Failed to create CAN cyclic task\n");
        return -1;
    }
#endif

    return 0;
}
