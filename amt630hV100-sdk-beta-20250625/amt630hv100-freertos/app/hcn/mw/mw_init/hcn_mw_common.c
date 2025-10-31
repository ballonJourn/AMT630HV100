/**
*
* @file hcn_mw_common.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/18 12:20
* @author och
*
*/

#include <FreeRTOS.h>
#include <string.h>
#include "task.h"
#include "config/hcn_config.h"
#include "mw_init/hcn_mw_common.h"
#include "mw_init/hcn_mw_init.h"    
#include "log/hcn_log.h"
#include "uart_communicate/hcn_uart_send_cmd.h"

#define VEHICLE_THREAD_PERIOD  pdMS_TO_TICKS(100)

#define SYS_PADCTL1_ADDR    (0x000000C4U)
#define SYS_PADCTL0_ADDR    (0x000000C0U)

#if 0
static void read_reg(void) {
    uint32_t reg_val = *((volatile uint32_t *)(REGS_SYSCTL_BASE + SYS_PADCTL1_ADDR));
    hcn_log_info("pad ctrl value = 0x%08X\n", reg_val);
}
#endif

void set_os_date_time(SystemTime_t date_time) {
#ifdef HCN_UART_COMM_ENABLE
    send_mcu_set_time(date_time);
#else
    vSetLocalTime(&date_time);
#endif
}

SystemTime_t get_os_date_time(void) {
#ifdef HCN_UART_COMM_ENABLE
    return get_mcu_time();
#else
    SystemTime_t sys_time;
    memset(sys_time, 0, sizeof(SystemTime_t));

    iGetLocalTime(&sys_time);

    return sys_time;
#endif
}

static void common_io_thread(void *param) {
    light_gpio_init(VEHICLE_THREAD_PERIOD);

    for (;;) {
        vTaskDelay(VEHICLE_THREAD_PERIOD);
        scan_frame_light();
    }
}

void mw_common_init(void) {
    if (xTaskCreate(common_io_thread, "common_io_thread", configMINIMAL_STACK_SIZE,
                    NULL, configMAX_PRIORITIES / 4, NULL) != pdPASS) {
        hcn_log_error("create common io task fail.\n");
        return;
    }
}
