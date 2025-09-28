/**
*
* @file hcn_can_rx.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 09:40
* @author och
*
*/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "chip.h"
#include "log/hcn_log.h"
#include "hal_gpio/hal_gpio.h"
#include "can_module/hcn_can_rx.h"
#include "can_module/hcn_can_tx.h"
#include "utils/hcn_utils.h"
#include "vehicle_param/vehicle_param.h"

//#define CAN_RX_DEBUG
#define CAN_RX_TIMEOUT_INTERVAL    (1000)

static bool is_can_com = false;
static can_rx_timeout rx_timeout;

static void timeout_clear_can_data(void) {
    TickType_t current_time = xTaskGetTickCount();

    if (current_time - rx_timeout.ecu_110_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_SPEED_CURRENT) > 0) {
            vehicle_set_data(VEH_SPEED_CURRENT, 0);
        }
    }

    if (current_time - rx_timeout.ecu_402_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_SPEED_ENGINE) > 0) {
            vehicle_set_data(VEH_SPEED_ENGINE, 0);
        }
    }

    if (current_time - rx_timeout.ecu_12b_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_TRAM_POWR) > 0) {
            vehicle_set_data(VEH_TRAM_POWR, 0);
        }
    }
}

static int parse_can_msg_110_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_110 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_110_rx_timeout = xTaskGetTickCount();

    uint16_t data = ((buf[1] << 8) + (buf[0]));
    if (data == 0xffff) {
        data = 0;
    }

    float speed_tmp = data * 0.01;
    if (speed_tmp > 199) {
        speed_tmp = 199;
    }

    vehicle_set_data(VEH_SPEED_CURRENT, (int)speed_tmp);

    return 0;
}

static int parse_can_msg_402_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_402 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_402_rx_timeout = xTaskGetTickCount();

    int data = ((buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0]);
    if (data == 0xffffffff) {
        data = 0;
    }

    float rpm_tmp = data * 0.01;
    if (rpm_tmp > 12000) {
        rpm_tmp = 12000;
    }

    vehicle_set_data(VEH_SPEED_ENGINE, (int)rpm_tmp);

    return 0;
}

static int parse_can_msg_12b_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_12b dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_12b_rx_timeout = xTaskGetTickCount();

    uint16_t data = ((buf[1] << 8) + (buf[0]));
    if (data == 0xffff) {
        data = 0;
    }

    float power_tmp = data * 0.01;
    if (power_tmp > 1000) {
        power_tmp = 1000;
    }

    vehicle_set_data(VEH_TRAM_POWR, (int)power_tmp);

    return 0;
}

static void can_recv_msg_process(CanMsg *pMsg) {
    if (!pMsg) {
        return;
    }

#ifdef  CAN_RX_DEBUG
    hcn_hex_config_data_print(__FUNCTION__, ":recv(0x)", pMsg->Data, pMsg->DLC);
#endif

    switch (pMsg->StdId) {
        case 0x110:
            parse_can_msg_110_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x402:
            parse_can_msg_402_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x12b:
            parse_can_msg_12b_msg(pMsg->Data, pMsg->DLC);
            break;

        default:
            break;
    }
}

static void can_rxdemo_thread(void *param) {
    CanPort_t *cap = param;
    CanMsg rxmsg[8] = {0};
    int recv_len = 0;
    int i = 0;
    uint32_t comm_timer = 0;

    for (;;) {
        if ((recv_len = iCanRead(cap, rxmsg, 8, pdMS_TO_TICKS(10))) > 0) 
		{
            comm_timer = xTaskGetTickCount();
            for (i = 0; i < recv_len; i++) {
                can_recv_msg_process(&rxmsg[i]);
            }
        }
        
        timeout_clear_can_data();

        if (xTaskGetTickCount() - comm_timer > 3000) {
            is_can_com = false;
        } else {  
            is_can_com = true;
        }
    }
}

int can_module_init(void) {
    CanPort_t *cap = xCanOpen(CAN_ID0);
    if (!cap) {
        hcn_log_error("open can %d failed!\n", CAN_ID1);
        return -1;
    }

    #if 0
    hal_gpio_set_output(58);
    #endif

    vCanInit(cap, CAN500kBaud, CAN_MODE_NORMAL);

#if 0
	CAN_FilterInitTypeDef canfilter = {0};
	/* 只接收ID的第0位为1的帧 */
	canfilter.MODE = 1; /* 单滤波器模式 */
	canfilter.ID = 0x1;
	canfilter.IDMASK = 0x7fe;
	vCanSetFilter(cap, &canfilter);
#endif

    if (xTaskCreate(can_rxdemo_thread, "canrx", configMINIMAL_STACK_SIZE, cap,
                    configMAX_PRIORITIES / 3, NULL) != pdPASS) {
        hcn_log_error("create can rxdemo task fail.\n");
        return -1;
    }

    can_msg_tx_msg_init(cap);
    printf("can moudle init ok!\r\n");

    return 0;
}