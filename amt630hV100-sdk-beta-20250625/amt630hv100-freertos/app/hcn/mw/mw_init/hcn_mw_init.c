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
#include <stdbool.h>
#include "mw_init/hcn_mw_init.h"
#include "config/hcn_config.h"
#include "log/hcn_log.h"
#include "amt630hv100.h"
#include "board.h"
#include "task.h"

#define SYS_PADCTL1_ADDR    (0x000000C4U)
#define SYS_PADCTL0_ADDR    (0x000000C0U)

#if 0
static void read_reg(void) {
    uint32_t reg_val = *((volatile uint32_t *)(REGS_SYSCTL_BASE + SYS_PADCTL1_ADDR));
    hcn_log_info("pad ctrl value = 0x%08X\n", reg_val);
}

static void backlight_test(void) {
    static uint8_t level = 5;
    static bool inc = true;

    if ((level > 1) && inc) {
        level--;
        if (level == 1) {
            inc = false;
        }
    }

    if (level < 5 && !inc) {
        level++;
        if (level == 5) {
            inc = true;
        }
    }
    
    hcn_log_info("current level:%d\n", level);
    set_backlight_level(level);
}

static void reg_read_thread(void *param) {
    light_gpio_init(500);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        scan_frame_light();
        //start_record();
        send_mcu_heartbeat();
        //light_sensor_process();
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
#else
    gpio_key_init();
#endif

#ifdef HCN_UART_COMM_ENABLE
    uart_mcu_init();
#endif

    ///< 开启亮度
    set_backlight_level(5);
#if 0
    if (xTaskCreate(reg_read_thread, "reg_read", configMINIMAL_STACK_SIZE,
                    NULL, configMAX_PRIORITIES / 4, NULL) != pdPASS) {
        hcn_log_error("create keypad task fail.\n");
        return;
    }
#endif

    hal_audio_init();
}