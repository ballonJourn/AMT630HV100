/**
*
* @file hcn_adc_key.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/25 09:20
* @author och
*
*/
#ifndef __HCN_ADC_KEY_H__
#define __HCN_ADC_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "config/hcn_config.h"

#ifdef HCN_ADC_KEY_ENABLE

typedef enum {
    ADC_KEY_UP        = 17,  
    ADC_KEY_DOWN      = 18,  
    ADC_KEY_RIGHT     = 19, 
    ADC_KEY_LEFT      = 20,  
    ADC_KEY_ESC       = 27,  
    ADC_KEY_ENTER     = 10,  
    ADC_KEY_HOME      = 2,   
    ADC_KEY_BACK      = 4,
    ADC_KEY_SET       = 5,
    ADC_KEY_MODE      = 6,
} adc_key_type_e;

typedef enum {
    KEY_RELEASE = 0,
    KEY_PRESS   = 1,
} key_state_e;

typedef struct  {
    int state;    ///< 松开，或者按下
    int key_type; ///<按键类型 adc_key_type_e
} keypad_event_t;

void send_keypad_event_isr(int key, int state);

int hcn_keypad_msg_init(void);

#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_ADC_KEY_H__
