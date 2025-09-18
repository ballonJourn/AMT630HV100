#include "amt630h.h"

void vSysctlConfigure(uint32_t regoffset, uint32_t bitoffset, uint32_t mask, uint32_t val)
{
	uint32_t tmp = readl(REGS_SYSCTL_BASE + regoffset);

	tmp &= ~(mask << bitoffset);
	tmp |= val << bitoffset;
	writel(tmp, REGS_SYSCTL_BASE + regoffset);
}
