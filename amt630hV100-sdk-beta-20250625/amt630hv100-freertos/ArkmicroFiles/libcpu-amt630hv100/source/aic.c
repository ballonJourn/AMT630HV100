 /**
 * \file
 *
 * Implementation of Ark Interrupt Controller (AIC) controller.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "chip.h"

#include <stdint.h>
#include <assert.h>
#include <string.h>

#define ICSET		0x00
#define ICPEND		0x04
#define ICMODE		0x08
#define ICMASK		0x0C
#define ICLEVEL		0x10
#define IRQPRIO		0x24
#define IRQISPR		0x3C
#define IRQISPC		0x40
#define IVEC_ADDR	0x78

typedef struct {
	ISRFunction_t handler;
	void *handler_param;
}IrqDesc_t;
static IrqDesc_t irq_descs[MAX_IRQ_NUM];

static volatile uint8_t interrupt_nest = 0;

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/
void AIC_Initialize(void)
{
	memset(irq_descs, 0, sizeof(irq_descs));

	writel(0x8, REGS_AIC_BASE + ICSET);
	vTimerUdelay(10);
	writel(0x5, REGS_AIC_BASE + ICSET);
	writel(0x0, REGS_AIC_BASE + ICMODE);
	/* set irq 15-8 highest priority */
	writel((1 << 12) | (0xf << 8) | (3 << 6) | (2 << 4) | (0 << 2) | 1, REGS_AIC_BASE + IRQPRIO);
    /* set round-robin priority policy */
    //writel(0, REGS_AIC_BASE + IRQPRIO);
	writel(0xffffffff, REGS_AIC_BASE + ICMASK);
	writel(0xffffffff, REGS_AIC_BASE + ICLEVEL);
	writel(0xffffffff, REGS_AIC_BASE + IRQISPC);
}

/**
 * \brief Enables interrupts coming from the given (unique) source (ID_xxx).
 *
 * \param source  Interrupt source to enable.
 */
void AIC_EnableIT(uint32_t source)
{
	writel(readl(REGS_AIC_BASE + ICMASK) & ~(1 << source), REGS_AIC_BASE + ICMASK);
}

/**
 * \brief Disables interrupts coming from the given (unique) source (ID_xxx).
 *
 * \param source  Interrupt source to disable.
 */
void AIC_DisableIT(uint32_t source)
{
	writel(readl(REGS_AIC_BASE + ICMASK) | (1 << source), REGS_AIC_BASE + ICMASK);
}

int32_t request_irq(uint32_t irq_source, int32_t priority, ISRFunction_t func, void *param)
{
	if (irq_source > MAX_IRQ_NUM - 1) {
		return -1;
	}
	portENTER_CRITICAL();
	irq_descs[irq_source].handler = func;
	irq_descs[irq_source].handler_param = param;
	AIC_EnableIT(irq_source);
	portEXIT_CRITICAL();

	return 0;
}

int32_t free_irq(uint32_t irq_source)
{
	if (irq_source > MAX_IRQ_NUM - 1) {
		return -1;
	}
	portENTER_CRITICAL();
	irq_descs[irq_source].handler = NULL;
	irq_descs[irq_source].handler_param = NULL;
	AIC_DisableIT(irq_source);
	portEXIT_CRITICAL();

	return 0;
}


void AIC_IrqHandler(void)
{
	int32_t source;

	interrupt_nest++;	
	source = readl(REGS_AIC_BASE + IVEC_ADDR) >> 2;
	if (irq_descs[source].handler)
		irq_descs[source].handler(irq_descs[source].handler_param);
	writel(1 << source, REGS_AIC_BASE + IRQISPC);
	interrupt_nest--;
}

uint8_t interrupt_get_nest(void)
{
    uint8_t ret;
    UBaseType_t uxSavedInterruptStatus;

    uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
    ret = interrupt_nest;
    portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);
    return ret;
}

