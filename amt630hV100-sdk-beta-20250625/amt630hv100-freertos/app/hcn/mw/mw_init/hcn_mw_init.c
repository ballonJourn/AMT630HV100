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
#include "backlight/hcn_backlight.h"
#include "log/hcn_log.h"
#include "amt630hv100.h"
#include "board.h"
#include "task.h"

static void hcn_power_io_init(void) {
    hal_gpio_set_output(AW_88028_PWR_EN_GPIO, 1);
    hal_gpio_set_output(CAN_PWR_EN_GPIO, 1);
}

void hcn_mw_init(void) {
    hcn_power_io_init();

    init_backlight_pwm();

#ifdef HCN_NOR_FLASH_PARAM_ENABLE    
    usr_param_init();
#endif

#ifdef HCN_UART_COMM_ENABLE
    uart_mcu_init();
#endif

    bt_module_init();
    
    mw_common_init();

#ifdef CAN_MODULE_ENABLE
    can_module_init();
#endif

    dev_state_init();
    
#ifdef HCN_ADC_KEY_ENABLE
    hcn_keypad_msg_init();
#else
    gpio_key_init();
#endif

#ifdef HCN_ADC_LIGHT_SENSOR_ENABLE
    display_mode_init();
#endif

    hal_audio_init();

    carlink_cb_init();
}