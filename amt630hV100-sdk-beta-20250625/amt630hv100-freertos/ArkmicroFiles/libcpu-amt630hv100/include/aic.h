/**
 *  \file
 *
 *  \section Purpose
 *
 *  Methods and definitions for configuring interrupts.
 *
 *  \section Usage
  *  -# Enable or disable interrupt generation of a particular source with
 *    IRQ_EnableIT and IRQ_DisableIT.
 *  -# Start or stop the timer clock using TC_Start() and TC_Stop().
 */
 
#ifndef AIC_H
#define AIC_H

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include "chip.h"

#include <stdint.h>

/*------------------------------------------------------------------------------
 *         Global functions
 *------------------------------------------------------------------------------*/
 
 #ifdef __cplusplus
 extern "C" {
#endif

typedef void (*ISRFunction_t)( void *param );

enum {
	IRQ_TYPE_EDGE_BOTH,
	IRQ_TYPE_EDGE_RISING,
	IRQ_TYPE_EDGE_FALLING,
	IRQ_TYPE_LEVEL_HIGH,
	IRQ_TYPE_LEVEL_LOW,	
};

extern void AIC_Initialize(void);
extern void AIC_EnableIT( uint32_t source);
extern void AIC_DisableIT(uint32_t source);
extern int32_t request_irq(uint32_t irq_source, int32_t priority, ISRFunction_t func, void *param);
extern int32_t free_irq(uint32_t irq_source);
extern void AIC_IrqHandler(void);
extern uint8_t interrupt_get_nest(void);

#ifdef __cplusplus
}
#endif

#endif //#ifndef AIC_H

