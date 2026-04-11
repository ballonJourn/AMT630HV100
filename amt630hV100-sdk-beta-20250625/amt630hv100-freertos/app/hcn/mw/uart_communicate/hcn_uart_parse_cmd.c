/**
*
* @file hcn_uart_parse_cmd.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/29 12:23
* @author och
*
*/

#include <stdbool.h>
#include <string.h>
#include <FreeRTOS.h>
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "uart_communicate/hcn_uart_parse_cmd.h"
#include "uart_communicate/hcn_uart_common.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#include "vehicle_param/vehicle_param.h"
#include "storage_param1/hcn_usr_param.h"
#include "storage_param2/hcn_mile_param.h"
#include "maintence/hcn_mileage_maintence.h"
#include "shutdown_manage/hcn_shutdown.h"
#include "shutdown_manage/hcn_shutdown_anim.h"
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"

#ifdef HCN_UART_COMM_ENABLE

#define START_SRC_TIMEOUT_PERIOD pdMS_TO_TICKS(1500)
#define HANDSHAKE_TIMEOUT_PERIOD pdMS_TO_TICKS(100)

static QueueHandle_t recv_msg_queue = NULL;
static TimerHandle_t start_src_timer = NULL;
static TimerHandle_t handshake_timer = NULL;

static bool is_hande_shake = false;

static char mcu_ver_full[30] = {0};
static uint8_t g_mcu_status = 0;
static int8_t g_handshake_max_num = 5;
static bool is_acc_off = false;
static bool is_acc_on = false;

static void start_source_timeout(TimerHandle_t xTimer) {
    hcn_log_info("Mcu rx start source timeout \n");
    check_start_source(0x01);
}

static void handshake_timeout(TimerHandle_t xTimer) {
    if (g_handshake_max_num > 0) {
        g_handshake_max_num--;
        send_mcu_heartbeat();
        hcn_log_info("Times %d:Try handshake with Mcu!\n", g_handshake_max_num);
        xTimerReset(handshake_timer, 0);
    } else {
        hcn_log_info("Error, Can not communicate with Mcu!\n");
        xTimerStop(handshake_timer, 0);
    }
}

void start_handshake_timer(void) {
    if (handshake_timer) {
        xTimerReset(handshake_timer, 0);
        hcn_log_info("Start handshake timer success!\n");
    }
}

bool get_handshake_state(void) {
    return is_hande_shake;
}

const char *get_mcu_version(void) {
    return mcu_ver_full;
}
 
bool get_mcu_ign_state(void) {
    return is_acc_on;
}

static void set_mcu_status(uint8_t status) { g_mcu_status = status; }

uint8_t get_mcu_status(void) { return g_mcu_status; }

static void light_info_parse(uint8_t *data) {
    uint8_t data_start = 8;
    uint8_t data_tmp = 0;

    data_tmp = (data[data_start] & 0x01);
    hcn_log_info("location value :%d\n", data_tmp);
    vehicle_set_data(VEH_LIGHT_LOCATION, (int)data_tmp);

    data_tmp = (data[data_start] >> 1 & 0x01);
    vehicle_set_data(VEH_LIGHT_LOW_BEAM, (int)data_tmp);

    data_tmp = (data[data_start] >> 2 & 0x01);
    vehicle_set_data(VEH_SIDE_STAND, (int)data_tmp);
    hcn_log_info("side stand:%d\n", data_tmp);

    data_tmp = (data[data_start] >> 3 & 0x01);
    vehicle_set_data(VEH_TILT_SWITCH, (int)data_tmp);

    data_tmp = (data[data_start] >> 4 & 0x01);
    vehicle_set_data(VEH_LIGHT_OIL_PRESSURE, (int)data_tmp);
    hcn_log_info("oil pressure:%d\n", data_tmp);

    data_tmp = (data[data_start] >> 5 & 0x01);
    vehicle_set_data(VEH_LIGHT_ENGINE_FAULT, (int)data_tmp);
    hcn_log_info("obd:%d\n", data_tmp);

    data_tmp = (data[data_start + 1] & 0x01);
    vehicle_set_data(VEH_LAUNCH_CONTROL_SWITCH, (int)data_tmp);
    hcn_log_info("lanuch switch:%d\n", data_tmp);

    data_tmp = (data[data_start + 1] >> 1 & 0x01);
    vehicle_set_data(VEH_LAUNCH_CONTROL_STATUS, (int)data_tmp);
    hcn_log_info("lanuch ctrl status:%d\n", data_tmp);

    data_tmp = (data[data_start + 2] >> 3 & 0x01);
    vehicle_set_data(VEH_CRUISE_MAIN_SWITCH, (int)data_tmp);

    data_tmp = (data[data_start + 2] >> 4 & 0x01);
    vehicle_set_data(VEH_CRUISE_STATUS, (int)data_tmp);
    vehicle_set_data(VEH_CRUISE_CONTROL_SPEED, (int)data[data_start + 9]);
    hcn_log_info("Cruise speed:%d\n", data[data_start + 8]);

    data_tmp = (data[data_start + 3] >> 2 & 0x01);
    hcn_log_info("high beam:%d\n", data_tmp);
    vehicle_set_data(VEH_LIGHT_HIGH_BEAM, (int)data_tmp);

    int oil_data = ((data[data_start + 6] << 8) + data[data_start + 7]);
    vehicle_set_data(VEH_FUEL_CONSUMPTION_INSTANT, oil_data);
}

static void uart_mcu_parse_msg_process(uint8_t *data) {
    if (!data) {
        return;
    }

    static bool is_first_set_odo = true; 
    static bool is_first_set_trip = true;

    uint16_t cmd = ((data[4] << 8) + data[5]);
    uint8_t data_start = 8;
    switch (cmd) {
        case UART_MCU_CMD_ACK_SHAKE_HANDS:
        case UART_MCU_CMD_ACC_STATE:
        //case UART_MCU_CMD_VEHICLE_INFO:
        case UART_MCU_CMD_VER_INFO:
        case UART_MCU_CMD_ODO_INFO:
        case UART_MCU_CMD_TRIP_INFO:
        case UART_MCU_CMD_RPM_AND_SPEED:
        case UART_MCU_CMD_VEHICLE_INFO1:
        case UART_MCU_CMD_VEH_LIGHT_INFO:
        case UART_MCU_CMD_ABS_INFO:
        case UART_MCU_CMD_TORQUE_INFO:
        case UART_MCU_CMD_GEAR_INFO:
        case UART_MCU_CMD_START_SRC: {
#if 1
            uint16_t data_len = ((data[6] << 8) + data[7]);
            hcn_hex_config_data_print("Soc analysis", ":Recv(0x)", data,
                                     data_len + UART_MCU_MSG_MIN_LEN);
#endif
        } break;

        default:
            break;
    }

    switch (cmd) {
        case UART_MCU_CMD_ACK_SHAKE_HANDS:
            hcn_log_info("Recv mcu shake hand.\n");
            if (!is_hande_shake) {
                xTimerStop(handshake_timer, 0);
                is_hande_shake = true;
                send_req_mcu_version();
                send_checkself_state(0);
                send_mcu_request_odo();
                send_mcu_request_trip(0);
            }
            break;

        case UART_MCU_CMD_ACC_STATE: {
            uint8_t state = data[data_start];
            hcn_log_info("acc state:%d\r\n", state);
            if (state) {
                is_acc_on = true;
                if (is_acc_off) {
#ifdef HCN_SHUTDOWN_ANIM_ENABLE
                    stop_power_off_timer();
#endif
                    hcn_ign_on();
                }
            } else {
                is_acc_off = true;
                is_acc_on = false;
#ifdef HCN_SHUTDOWN_ANIM_ENABLE
                start_power_off_timer();
#endif
                hcn_ign_off();
            }
        } break;

        case UART_MCU_CMD_VEHICLE_INFO: {
            uint8_t data_tmp = 0;

            int battery = (data[data_start] << 8) + data[data_start + 1];
                        vehicle_set_data(VEH_VOLTAGE_BATTERY, battery);
            battery = (int)(battery / 10);
            data_tmp = data[data_start + 2];
#if 0
            hcn_log_info("turn left light:%d\n", data_tmp);
            vehicle_set_data(VEH_INDICATOR_TURN_RIGHT, (int)data_tmp);
#endif

            data_tmp = data[data_start + 3];
            //hcn_log_info("vehcile data :%d\n", data_tmp);

#if 0
            hcn_log_info("turn right light:%d\n", data_tmp);
            vehicle_set_data(VEH_INDICATOR_TURN_LEFT, (int)data_tmp);
#endif
        } break;

        case UART_MCU_CMD_VER_INFO: {
            uint16_t ver_len = ((data[6] << 8) + data[7]);
            memcpy(mcu_ver_full, data + 8,
                   (ver_len > sizeof(mcu_ver_full)) ? sizeof(mcu_ver_full)
                                                    : ver_len);
            hcn_log_info("Mcu version:%s\n", mcu_ver_full);
        } break;

        case UART_MCU_CMD_ODO_INFO: {
            uint32_t odo = ((data[data_start] << 24) + (data[data_start + 1] << 16) +
                       (data[data_start + 2] << 8) + data[data_start + 3]);
            vehicle_set_data(VEH_MILEAGE_TOTAL, (int)odo);
            hcn_log_info("ODO data = %dm\r\n", odo);
            if (is_first_set_odo) {
                is_first_set_odo = false;
                set_hcn_mile_param(HCN_MILE_PARAM_ODO, &odo);
            }
            #ifdef HCN_MILEAGE_MAINTENCE_ENABLE
            update_maintence_mileage(odo);
            #endif
        } break;

        case UART_MCU_CMD_TRIP_INFO: {
            int data_tmp =
                ((data[data_start] << 24) + (data[data_start + 1] << 16) +
                 (data[data_start + 2] << 8) + data[data_start + 3]);
            if (data_tmp == 0xFFFFFFFF) {
                data_tmp = 0;
            }
            vehicle_set_data(VEH_MILEAGE_SUB_A, data_tmp);
            hcn_log_info("trip a = %dm\r\b", data_tmp);

            uint32_t trip_data = (uint32_t)data_tmp;
            if (is_first_set_trip) {
                is_first_set_trip = false;
                set_hcn_mile_param(HCN_MILE_PARAM_TRIP_A, &trip_data);
            }

            data_tmp =
                ((data[data_start + 4] << 24) + (data[data_start + 5] << 16) +
                 (data[data_start + 6] << 8) + data[data_start + 7]);
            vehicle_set_data(VEH_MILEAGE_SUB_B, data_tmp);
            hcn_log_info("trip b = %dm\r\b", data_tmp);
        } break;

        case UART_MCU_CMD_RPM_AND_SPEED: {
            float tmp = ((data[data_start] << 8) + data[data_start + 1]) * 0.01;
            int data_tmp = 0;

            tmp = tmp * 1.05;
            //if (tmp > VEH_MAX_SPEED) {
            //    tmp = VEH_MAX_SPEED;
          //}

            data_tmp = (int)tmp;
            vehicle_set_data(VEH_SPEED_CURRENT, data_tmp);

            data_tmp = ((data[data_start + 2] << 8) + data[data_start + 3]);
            //if (data_tmp > VEH_MAX_RPM) {
            //    data_tmp = VEH_MAX_RPM;
            //}

            vehicle_set_data(VEH_SPEED_ENGINE, data_tmp);
        } break;

        case UART_MCU_CMD_VEHICLE_INFO1: {
            uint8_t data_tmp = data[data_start];
            if (data_tmp == 0xF0 || data_tmp == 0xFF) {
                vehicle_set_data(VEH_OIL_LEVEL, 0);
            } else {
                vehicle_set_data(VEH_OIL_LEVEL, (int)data_tmp);
            }

            if (data[data_start + 1] == 0xFF) {
                //check_water_temp_level(0xFF);
                vehicle_set_data(VEH_TEMP_WATER, 0);
            } else {
                int water = data[data_start + 1] - 48;
                //check_water_temp_level(water);
                vehicle_set_data(VEH_TEMP_WATER, water * 10);
            }
      
            int mileage_param =
                ((data[data_start + 4] << 8) + data[data_start + 5]);
            vehicle_set_data(VEG_AVG_FUEL_CONSUMPTION_A, mileage_param);
            mileage_param =
                ((data[data_start + 6] << 8) + data[data_start + 7]);
            vehicle_set_data(VEG_AVG_FUEL_CONSUMPTION_B, mileage_param);
            mileage_param =
                ((data[data_start + 8] << 8) + data[data_start + 9]);
            vehicle_set_data(VEH_MILEAGE_ENDURANCE_A, mileage_param);
            mileage_param =
                ((data[data_start + 10] << 8) + data[data_start + 11]);
           vehicle_set_data(VEH_MILEAGE_ENDURANCE_B, mileage_param);
        } break;

        case UART_MCU_CMD_GEAR_INFO: {
            uint8_t data_tmp = 0;
            data_tmp = data[data_start];
            if (data_tmp == 0) {
                data_tmp = 2;
            } else if (data_tmp == 1) {
                data_tmp = 0;
            } else if (data_tmp == 2) {
                data_tmp = 1;
            }
            vehicle_set_data(VEH_DRIVE_MODE, (int)data_tmp);
            vehicle_set_data(VEH_ABS_MODE, (int)data[data_start + 1]);

            if (data[data_start + 2] == 0xFF) {
                vehicle_set_data(VEH_LIGHT_ABS, 0);
            } else {
                vehicle_set_data(VEH_LIGHT_ABS, (int)data[data_start + 2]);
            }

            uint8_t gear_info = data[data_start + 3];
            if ((gear_info == 0xF0) || (gear_info == 0xFF) || (gear_info == 0x0F)) {
                vehicle_set_data(VEH_GEAR_POSITION, 7);
            } else {
                vehicle_set_data(VEH_GEAR_POSITION, (int)gear_info);
            }

            data_tmp = data[data_start + 4];
            if (data_tmp == 0xFF) {
                vehicle_set_data(VEH_TCS_MODE, 0);
            } else {
                vehicle_set_data(VEH_TCS_MODE, (int)data_tmp);
            }

            if (data_tmp == 0) {
                vehicle_set_data(VEH_TCS_WARNING, 2);
            } else if (data_tmp == 0xFF) {
                vehicle_set_data(VEH_TCS_WARNING, 0);
            } else {
                if (data[data_start + 5] == 0) {
                    vehicle_set_data(VEH_TCS_WARNING, 1);
                } else {
                    vehicle_set_data(VEH_TCS_WARNING, 0);
                }
            }
        } break;

        case UART_MCU_CMD_START_SRC: {
            uint8_t start_src = data[8];
            uint8_t mcu = data[9];

            if (start_src_timer) {
                xTimerStop(start_src_timer, 0);
            }

            set_mcu_status(mcu);
            check_start_source(start_src);
        } break;

        case UART_MCU_CMD_VEH_LIGHT_INFO:
            light_info_parse(data);
            break;

        case UART_MCU_CMD_ABS_INFO:
            break;

        case UART_MCU_CMD_TORQUE_INFO:
            break;

        default:
            break;
    }
}

static void uart_mcu_parse_msg_thread(void *param) {
    uint8_t *data;

    for (;;) {
        if (xQueueReceive(recv_msg_queue, &data, portMAX_DELAY) != pdPASS) {
            hcn_log_error("xQueueRecv parse msg error.\n");
            continue;
        }

        uart_mcu_parse_msg_process(data);
        vPortFree(data);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void uart_mcu_parse_add_task(uint8_t *data) {
    if (recv_msg_queue == NULL) {
        return;
    }

    uint8_t *msg = pvPortMalloc(UART_MCU_MSG_MAX_LEN);
    if (msg == NULL) {
        hcn_log_error("Malloc uart msg pointer is NULL.\n");
        return;
    }

    uint16_t data_len = ((data[6] << 8) + data[7]);
    memcpy(msg, data,
           data_len + UART_MCU_MSG_MIN_LEN > UART_MCU_MSG_MAX_LEN
               ? UART_MCU_MSG_MAX_LEN
               : data_len + UART_MCU_MSG_MIN_LEN);
    if (xQueueSend(recv_msg_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        vPortFree(msg);
        hcn_log_error("xQueueSend uart mcu msg error.\n");
    }
}

int uart_mcu_parse_task_init(void) {
    if (recv_msg_queue) {
        hcn_log_info("Uart mcu recv msg queue already inited.\n");
        return 0;
    }

    recv_msg_queue = xQueueCreate(10, sizeof(uint32_t));
    if (recv_msg_queue == NULL) {
        hcn_log_error("create recv_msg_queue error.\n");
        return -1;
    }

    start_src_timer =
        xTimerCreate("start_src_timeout", START_SRC_TIMEOUT_PERIOD, pdFALSE,
                     NULL, start_source_timeout);
    if (start_src_timer == NULL) {
        hcn_log_error("Create start src timer failed!\n");
        vQueueDelete(recv_msg_queue);
        recv_msg_queue = NULL;
        return -1;
    }

    handshake_timer = xTimerCreate("hande_shake", HANDSHAKE_TIMEOUT_PERIOD,
                                   pdFALSE, NULL, handshake_timeout);
    if (handshake_timer == NULL) {
        hcn_log_error("Create start handshake timer failed!\n");
        vQueueDelete(recv_msg_queue);
        xTimerDelete(start_src_timer, 0);

        start_src_timer = NULL;
        recv_msg_queue = NULL;

        return -1;
    }

    xTimerStart(start_src_timer, 0);

    if (xTaskCreate(uart_mcu_parse_msg_thread, "uart mcu parse thread",
                    configMINIMAL_STACK_SIZE *2, NULL, configMAX_PRIORITIES / 3,
                    NULL) != pdPASS) {
        hcn_log_error("Create uart mcu parse task fail.\n");
        return -1;
    }

#ifdef HCN_SHUTDOWN_ANIM_ENABLE
    hcn_shutdown_init();
#endif

    return 0;
}

#endif
