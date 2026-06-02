 /**
 * \file
 *
 * Implementation of Ark Interrupt Controller (AIC) controller.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "amt630h.h"
#include "aic.h"
#include "timer.h"

#include <stdint.h>
#include <string.h>

#define ICSET		0x00
#define ICPEND		0x04
#define ICMODE		0x08
#define ICMASK		0x0C
#define ICLEVEL		0x10
#define IRQISPR		0x3C
#define IRQISPC		0x40
#define IVEC_ADDR	0x78

typedef struct {
	ISRFunction_t handler;
	void *handler_param;
}IrqDesc_t;
static IrqDesc_t irq_descs[MAX_IRQ_NUM];

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/
void AIC_Initialize(void)
{
	memset(irq_descs, 0, sizeof(irq_descs));

	writel(0x8, REGS_AIC_BASE + ICSET);
	udelay(10);
	writel(0x5, REGS_AIC_BASE + ICSET);
	writel(0x0, REGS_AIC_BASE + ICMODE);
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

	irq_descs[irq_source].handler = func;
	irq_descs[irq_source].handler_param = param;
	AIC_EnableIT(irq_source);

	return 0;
}

int32_t free_irq(uint32_t irq_source)
{
	if (irq_source > MAX_IRQ_NUM - 1) {
		return -1;
	}

	irq_descs[irq_source].handler = NULL;
	irq_descs[irq_source].handler_param = NULL;
	AIC_DisableIT(irq_source);

	return 0;
}


void AIC_IrqHandler(void)
{
	int32_t source = readl(REGS_AIC_BASE + IVEC_ADDR) >> 2;
	if (irq_descs[source].handler)
		irq_descs[source].handler(irq_descs[source].handler_param);
	writel(1 << source, REGS_AIC_BASE + IRQISPC);
}
