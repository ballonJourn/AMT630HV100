/**
*
* @file hcn_mw_init.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 14:45
* @author och
*
*/

#include <FreeRTOS.h>
#include "mw_init/hcn_mw_init.h"
#include "config/hcn_config.h"
#include "log/hcn_log.h"
#include "amt630hv100.h"

#define SYS_PADCTL1_ADDR    (0x000000C4U)

#if 0
static void read_reg(void) {
    uint32_t reg_val = *((volatile uint32_t *)(REGS_SYSCTL_BASE + SYS_PADCTL1_ADDR));
    printf("[ouchunhua]pad ctrl value = 0x%08X\n", reg_val);
}

static void reg_read_thread(void *param) {
    for (;;) {
        read_reg();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif

void hcn_mw_init(void) {
#ifdef CAN_MODULE_ENABLE
    can_module_init();
#endif

    dev_state_init();
    
#ifdef HCN_ADC_KEY_ENABLE
    hcn_keypad_msg_init();
#endif

#ifdef HCN_UART_COMM_ENABLE
    uart_mcu_init();
#endif

    ///< 开启亮度
    set_backlight_level(5);
    hal_audio_init();
#if 0
    if (xTaskCreate(reg_read_thread, "reg_read", configMINIMAL_STACK_SIZE,
                    NULL, configMAX_PRIORITIES / 4, NULL) != pdPASS) {
        hcn_log_error("create keypad task fail.\n");
        return;
    }
#endif
}