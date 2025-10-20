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
#include "task.h"
#include "config/hcn_config.h"
#include "mw_init/hcn_mw_common.h"
#include "mw_init/hcn_mw_init.h"    
#include "log/hcn_log.h"

#define VEHICLE_THREAD_PERIOD  pdMS_TO_TICKS(100)

#define SYS_PADCTL1_ADDR    (0x000000C4U)
#define SYS_PADCTL0_ADDR    (0x000000C0U)

#if 0
static void read_reg(void) {
    uint32_t reg_val = *((volatile uint32_t *)(REGS_SYSCTL_BASE + SYS_PADCTL1_ADDR));
    hcn_log_info("pad ctrl value = 0x%08X\n", reg_val);
}
#endif

static void common_io_thread(void *param) {
    light_gpio_init(VEHICLE_THREAD_PERIOD);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
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
