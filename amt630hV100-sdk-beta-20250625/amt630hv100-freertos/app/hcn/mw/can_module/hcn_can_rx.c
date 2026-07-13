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
#include "dashboard_state/hcn_dev_state.h"
#include "storage_param2/hcn_mile_param.h"
#include "uart_communicate/hcn_uart_send_cmd.h"

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

        if (vehicle_get_data(VEH_GEAR_POSITION) > 0) {
            vehicle_set_data(VEH_GEAR_POSITION, 0);
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

#define CRC8_MAGIC_NUM (0x1D)

static uint8_t can_j1850_crc8(const uint8_t *data, uint8_t length) {
    uint8_t crc = 0xFF; 
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i]; 
        for (uint8_t j = 0; j < 8; j++) 
	    {
	        if (crc & 0x80) { 
	            crc = (crc << 1) ^ CRC8_MAGIC_NUM; 
	        } else {
	            crc <<= 1; 
	        }
	    }
        crc &= 0xFF; 
    }
	crc ^= 0xFF;
    return crc; 
}

static int parse_can_msg_110_msg(uint8_t *buf, uint8_t size) {
    if (get_check_self_state() < CHECK_SELF_STATE_START) {
        return 0;
    }

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
    if (get_check_self_state() < CHECK_SELF_STATE_START) {
        return 0;
    }

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
    if (get_check_self_state() < CHECK_SELF_STATE_START) {
        return 0;
    }

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
    if (get_check_self_state() < CHECK_SELF_STATE_START) {
        return 0;
    }

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
    if (get_check_self_state() < CHECK_SELF_STATE_START) {
        return 0;
    }

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
    if (get_check_self_state() < CHECK_SELF_STATE_START) {
        return 0;
    }
    
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_12b dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    rx_timeout.ecu_12b_rx_timeout = xTaskGetTickCount();

    int data = GET_DATA_BITS(buf[5], 0, 0x01);
    vehicle_set_data(VEH_LIGHT_ABS, data);

    return 0;
}

static int parse_can_msg_776_msg(uint8_t *buf, uint8_t size) {
    if (get_check_self_state() < CHECK_SELF_STATE_SUCCESS) {
        return 0;
    }
    
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_776 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    uint8_t crc_chceck;
    static uint8_t rx_cnt = 0;
	static uint8_t rx_buf[8] = {0};
#if 0
     hcn_hex_config_data_print("can", "recv(0x):", buf, 8);   
#endif

    crc_chceck = can_j1850_crc8(buf, 7);
    if (crc_chceck != buf[7]) {
        hcn_log_error("can_ecu_776 Error checksum = 0x%x, buf[7] = 0x%x !\r\n", \
            crc_chceck, buf[7]);
        return -1;
    }

    if (rx_cnt == 0) {
		memcpy(rx_buf, buf, 8);
		rx_cnt++;
	} else if (rx_cnt == 1) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else{
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	} else if(rx_cnt == 2) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else {
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	}

	if (rx_cnt == 3) {
		rx_cnt = 0;
		uint32_t trip_a;
        uint32_t tmp = 0;

        if (!get_hcn_mile_param(HCN_MILE_PARAM_TRIP_A, &tmp)) {
            hcn_log_error("Get HCN_MILE_PARAM_TRIP_A error!\r\n");
            return -1;
        }

        trip_a = (((uint32_t) buf[0]) << 24 ) |  (((uint32_t) buf[1]) << 16 ) | \
                (((uint32_t) buf[2]) << 8 ) |(((uint32_t) buf[3]) << 0);
		if (trip_a != tmp) {
            set_hcn_mile_param(HCN_MILE_PARAM_TRIP_A, &trip_a);
            vehicle_set_data(VEH_MILEAGE_CHANGE_MSG, 1);
		}
	}

	return 0;
}

static int parse_can_msg_777_msg(uint8_t *buf, uint8_t size) {
    if (get_check_self_state() < CHECK_SELF_STATE_SUCCESS) {
        return 0;
    }
    
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_777 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    uint8_t crc_chceck;
    static uint8_t rx_cnt = 0;
	static uint8_t rx_buf[8] = {0};
#if 0
     hcn_hex_config_data_print("can", "recv(0x):", buf, 8);   
#endif

    crc_chceck = can_j1850_crc8(buf, 7);
    if (crc_chceck != buf[7]) {
        hcn_log_error("can_ecu_777 Error checksum = 0x%x, buf[7] = 0x%x !\r\n", \
                    crc_chceck, buf[7]);
        return -1;
    }

    if (rx_cnt == 0) {
		memcpy(rx_buf, buf, 8);
		rx_cnt++;
	} else if (rx_cnt == 1) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else{
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	} else if(rx_cnt == 2) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else {
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	}

	if (rx_cnt == 3) {
		rx_cnt = 0;
		uint32_t trip_b;
        uint32_t tmp = 0;

        if (!get_hcn_mile_param(HCN_MILE_PARAM_TRIP_B, &tmp)) {
            hcn_log_error("Get HCN_MILE_PARAM_TRIP_B error!\r\n");
            return -1;
        }

        trip_b = (((uint32_t) buf[0]) << 24 ) |  (((uint32_t) buf[1]) << 16 ) | \
                (((uint32_t) buf[2]) << 8 ) |(((uint32_t) buf[3]) << 0);
		if (trip_b != tmp) {
            set_hcn_mile_param(HCN_MILE_PARAM_TRIP_B, &trip_b);
            vehicle_set_data(VEH_MILEAGE_CHANGE_MSG, 2);
		}
	}

	return 0;
}

static int parse_can_msg_778_msg(uint8_t *buf, uint8_t size) {
   if (get_check_self_state() < CHECK_SELF_STATE_SUCCESS) {
        return 0;
    }
    
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_778 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    uint8_t crc_chceck;
    static uint8_t rx_cnt = 0;
	static uint8_t rx_buf[8] = {0};
#if 0
     hcn_hex_config_data_print("can", "recv(0x):", buf, 8);   
#endif

    crc_chceck = can_j1850_crc8(buf, 7);
    if (crc_chceck != buf[7]) {
        hcn_log_error("can_ecu_778 Error checksum = 0x%x, buf[7] = 0x%x !\r\n", \
                    crc_chceck, buf[7]);
        return -1;
    }

    if (rx_cnt == 0) {
		memcpy(rx_buf, buf, 8);
		rx_cnt++;
	} else if (rx_cnt == 1) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else{
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	} else if(rx_cnt == 2) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else {
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	}

	if (rx_cnt == 3) {
		rx_cnt = 0;
		uint32_t odo;
        uint32_t tmp = 0;

        if (!get_hcn_mile_param(HCN_MILE_PARAM_ODO, &tmp)) {
            hcn_log_error("Get HCN_MILE_PARAM_ODO error!\r\n");
            return -1;
        }

        odo = (((uint32_t) buf[0]) << 24 ) |  (((uint32_t) buf[1]) << 16 ) | \
                (((uint32_t) buf[2]) << 8 ) |(((uint32_t) buf[3]) << 0);
		if (odo != tmp) {
            set_hcn_mile_param(HCN_MILE_PARAM_ODO, &odo);
            vehicle_set_data(VEH_MILEAGE_CHANGE_MSG, 3);
		}
	}

	return 0;
}

static int parse_can_msg_779_msg(uint8_t *buf, uint8_t size) {
    if (get_check_self_state() < CHECK_SELF_STATE_SUCCESS) {
        return 0;
    }
    
    if ((buf == NULL) || (size < 8)) {
        hcn_log_error("can_ecu_779 dlc[%d] or msg buff err!\r\n",size);
        return -1;
    }

    uint8_t crc_chceck;
    static uint8_t rx_cnt = 0;
	static uint8_t rx_buf[8] = {0};
#if 0
     hcn_hex_config_data_print("can", "recv(0x):", buf, 8);   
#endif

    crc_chceck = can_j1850_crc8(buf, 7);
    if (crc_chceck != buf[7]) {
        hcn_log_error("can_ecu_779 Error checksum = 0x%x, buf[7] = 0x%x !\r\n", \
                    crc_chceck, buf[7]);
        return -1;
    }

    if (rx_cnt == 0) {
		memcpy(rx_buf, buf, 8);
		rx_cnt++;
	} else if (rx_cnt == 1) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else{
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	} else if(rx_cnt == 2) {
		if (memcmp(rx_buf, buf, 8) == 0) {
			rx_cnt++;
		} else {
			memcpy(rx_buf, buf, 8);
			rx_cnt = 1;
		}
	}

	if (rx_cnt == 3) {
		if ((buf[0] == 0xa1) && (buf[1] == 0xb1) 
            && (buf[2] == 0xc1) && (buf[3] == 0xd1) 
            && (buf[4] == 0xe1) && (buf[5] == 0xf1) 
            && (buf[6] == 0xee)) {
            extern void clean_eeprom_operate(void);
            clean_eeprom_operate();
            vTaskDelay(pdMS_TO_TICKS(1000));
            send_mcu_clear_eeprom();
            hcn_log_info("clear eerpom, os will reboot...\r\n"); 
        }
	}

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
            break;

        case 0x12b:
            parse_can_msg_12b_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x776:
            parse_can_msg_776_msg(pMsg->Data, pMsg->DLC);
            break;
            
        case 0x777:
            parse_can_msg_777_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x778:
            parse_can_msg_778_msg(pMsg->Data, pMsg->DLC);
            break;

        case 0x779:
            parse_can_msg_779_msg(pMsg->Data, pMsg->DLC);
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

    if (xTaskCreate(can_rxdemo_thread, "canrx", configMINIMAL_STACK_SIZE * 4, cap,
                    configMAX_PRIORITIES / 3, NULL) != pdPASS) {
        hcn_log_error("create can rxdemo task fail.\n");
        return -1;
    }

    can_msg_tx_msg_init(cap);

    hcn_log_info("can moudle init ok!\r\n");

    return 0;
}
