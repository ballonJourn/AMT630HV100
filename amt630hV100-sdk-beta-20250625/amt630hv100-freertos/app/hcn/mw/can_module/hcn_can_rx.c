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
#define GET_DATA_BITS(data,offset,mask) ((data >> offset) & mask)  

static bool is_can_com = false;
static can_rx_timeout rx_timeout;

static void timeout_clear_can_data(void) {
    TickType_t current_time = xTaskGetTickCount();

    if (current_time - rx_timeout.ecu_110_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_SPEED_ENGINE) > 0) {
            vehicle_set_data(VEH_SPEED_ENGINE, 0);
        }
    }

    if (current_time - rx_timeout.ecu_111_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_DRIVE_MODE) > 0) {
            vehicle_set_data(VEH_DRIVE_MODE, 0);
        }

        if (vehicle_get_data(VEH_LIGHT_ENGINE_FAULT) > 0) {
            vehicle_set_data(VEH_LIGHT_ENGINE_FAULT, 0);
        }
    }

    if (current_time - rx_timeout.ecu_112_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_LIGHT_BRAKE) > 0) {
            vehicle_set_data(VEH_LIGHT_BRAKE, 0);
        }

        if (vehicle_get_data(VEH_LIGHT_READY) > 0) {
            vehicle_set_data(VEH_LIGHT_READY, 0);
        }

        if (vehicle_get_data(VEH_LIGHT_GPS) > 0) {
            vehicle_set_data(VEH_LIGHT_GPS, 0);
        }

        if (vehicle_get_data(VEH_TRAM_POWR) > 0) {
            vehicle_set_data(VEH_TRAM_POWR, 0);
        }

        if (vehicle_get_data(VEH_TRAM_REMAIN_BATTARY) > 0) {
            vehicle_set_data(VEH_TRAM_REMAIN_BATTARY, 0);
        }
    }

    if (current_time - rx_timeout.ecu_120_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_TCS_WARNING) > 0) {
            vehicle_set_data(VEH_TCS_WARNING, 0);
        }
    }

    if (current_time - rx_timeout.ecu_122_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_SPEED_CURRENT) > 0) {
            vehicle_set_data(VEH_SPEED_CURRENT, 0);
        }
    }

    if (current_time - rx_timeout.ecu_12b_rx_timeout > CAN_RX_TIMEOUT_INTERVAL) {
        if (vehicle_get_data(VEH_LIGHT_ABS) == 0) {
            vehicle_set_data(VEH_LIGHT_ABS, 1);
        }
    }
}

static int parse_can_msg_110_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_110 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_110_rx_timeout = xTaskGetTickCount();

    int data = ((buf[2] << 8) + (buf[3]));
    if (data == 0xffff) {
        data = 0;
    }

    float engine_speed_tmp = data * 0.25;
    data = (int)engine_speed_tmp;
    if (data > 0 && data <= 1700) {
        data = 1700;
    }

    vehicle_set_data(VEH_SPEED_ENGINE, data);

    return 0;
}

static int parse_can_msg_111_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_111 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_111_rx_timeout = xTaskGetTickCount();

    int data = GET_DATA_BITS(buf[0], 1, 0x01);
    vehicle_set_data(VEH_LIGHT_ENGINE_FAULT, data);

    data = GET_DATA_BITS(buf[0], 2, 0x07);
    vehicle_set_data(VEH_DRIVE_MODE, data);

    return 0;
}

static int parse_can_msg_112_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_112 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_112_rx_timeout = xTaskGetTickCount();

    int data = GET_DATA_BITS(buf[0], 0, 0x01);
    vehicle_set_data(VEH_LIGHT_BRAKE, data);

    data = GET_DATA_BITS(buf[0], 1, 0x1);
    vehicle_set_data(VEH_LIGHT_READY, data);

    data = GET_DATA_BITS(buf[0], 2, 0x1);
    vehicle_set_data(VEH_LIGHT_GPS, data);

    data = GET_DATA_BITS(buf[2], 0, 0x07);
    vehicle_set_data(VEH_GEAR_POSITION, data);

    float power = 0;
    data = ((buf[5] << 8) + buf[6]);
    if (data == 0xFFFF) {
        data = 0;
    }

    power = data * 0.01;
    data = (int)power;
    if (data > 100) {
        data = 100;
    }
    vehicle_set_data(VEH_TRAM_POWR, data);

    data = buf[7];
    if (data == 0xFF) {
        data = 0;
    }
    vehicle_set_data(VEH_TRAM_REMAIN_BATTARY, data);

    return 0;
}

static int parse_can_msg_120_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_120 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_120_rx_timeout = xTaskGetTickCount();

    int data = GET_DATA_BITS(buf[3], 2, 0x01);
    vehicle_set_data(VEH_TCS_WARNING, data);

    return 0;
}

static int parse_can_msg_122_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_122 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_122_rx_timeout = xTaskGetTickCount();
    int data = 0;
    float speed_tmp = 0;

    data = ((buf[2] << 8) + buf[3]);
    if (data == 0xFFFF) {
        data = 0;
    }

    speed_tmp = data * 0.01;
    data = (int)speed_tmp;
    if (data > 199) {
        data = 199;
    }

    vehicle_set_data(VEH_SPEED_CURRENT, data);

    return 0;
}

static int parse_can_msg_12b_msg(uint8_t *buf, uint8_t size) {
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_12b dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_12b_rx_timeout = xTaskGetTickCount();

    int data = GET_DATA_BITS(buf[5], 0, 0x01);
    vehicle_set_data(VEH_LIGHT_ABS, data);

    return 0;
}

bool can_get_communication_status(void) {
    return is_can_com;
}

static void can_recv_msg_process(CanMsg *pMsg) {
    if (!pMsg) {
        return;
    }

#ifdef CAN_RX_DEBUG
    hcn_hex_config_data_print(__FUNCTION__, ":recv(0x)", pMsg->Data, pMsg->DLC);
#endif

    switch (pMsg->StdId) {
        case 0x110:
            parse_can_msg_110_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x111:
            parse_can_msg_111_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x112:
            parse_can_msg_112_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x120:
            parse_can_msg_120_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x122:
            parse_can_msg_122_msg(pMsg->Data, pMsg->DLC);

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
    CanPort_t *cap = xCanOpen(CAN_ID1);
    if (!cap) {
        hcn_log_error("open can %d failed!\n", CAN_ID1);
        return -1;
    }

    vTaskDelay(pdMS_TO_TICKS(2));
    hal_gpio_set_output(CAN_STB_GPIO, 0);
    vCanInit(cap, CAN500kBaud, CAN_MODE_NORMAL);

    vehicle_set_data(VEH_LIGHT_ABS, 1);
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

    hcn_log_info("can moudle init ok!\r\n");

    return 0;
}
