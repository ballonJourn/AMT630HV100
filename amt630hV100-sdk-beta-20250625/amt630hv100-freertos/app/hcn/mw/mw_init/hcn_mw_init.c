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

#include "mw_init/hcn_mw_init.h"
#include "config/hcn_config.h"

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
}