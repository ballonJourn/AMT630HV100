#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"


#define PLL_DIV_MASK			0xFF
#define PLL_DIV_OFFSET			0
#define PLL_NO_MASK				0x3
#define PLL_NO_OFFSET			12
#define PLL_ENA					(1 << 14)

typedef struct {
	uint32_t clkid;
	uint32_t clktype; 
	uint32_t source;
	int clksource[MAX_CLK_SOURCE_NUM];
	int source_index;
	int div;
	uint32_t enable_reg[MAX_CLK_ENABLE_BITS];
	int enable_offset[MAX_CLK_ENABLE_BITS];
	int enable_bits;
	union  {
		uint32_t fixed_freq;
		struct {
			int div;
			int mult;
		} fixed_fator_property;
		struct {
			uint32_t cfgreg;
			uint32_t refclkreg;
			uint32_t offset;
			uint32_t mask;
		} pll_property;
		struct {
			uint32_t cfgreg;
			uint32_t analogreg;
		} dds_property;
		struct {
			uint32_t cfgreg;
			int index_offset;
			int index_value;
			uint32_t index_mask;
			int div_offset;
			int div_value;
			uint32_t div_mask;
			int div_mode;
			uint32_t inv_reg;
			int inv_offset;
			uint32_t inv_mask;
			int inv_value;
		} sys_property;
	} u;
} xClockProperty;

xClockProperty xClocks[] = {
	{.clkid = CLK_XTAL32K, .clktype = FIXED_CLOCK, .u.fixed_freq = 32768},
	{.clkid = CLK_XTAL24M, .clktype = FIXED_CLOCK, .u.fixed_freq = 24000000},
	{.clkid = CLK_240MHZ, .clktype = FIXED_FACTOR_CLOCK, .clksource = {CLK_XTAL24M},
	 .u.fixed_fator_property.div = 1, .u.fixed_fator_property.mult = 10},
	{.clkid = CLK_12MHZ, .clktype = FIXED_FACTOR_CLOCK, .clksource = {CLK_XTAL24M},
	 .u.fixed_fator_property.div = 2, .u.fixed_fator_property.mult = 1},
	{.clkid = CLK_6MHZ, .clktype = FIXED_FACTOR_CLOCK, .clksource = {CLK_XTAL24M},
	 .u.fixed_fator_property.div = 4, .u.fixed_fator_property.mult = 1},
	{.clkid = CLK_CPUPLL, .clktype = PLL_CLOCK, .clksource = {CLK_6MHZ, CLK_12MHZ},
	 .u.pll_property.cfgreg = 0x60000088, .u.pll_property.refclkreg = 0x60000140,
	 .u.pll_property.offset = 0, .u.pll_property.mask = 1},
	{.clkid = CLK_SYSPLL, .clktype = PLL_CLOCK, .clksource = {CLK_6MHZ, CLK_12MHZ},
	 .u.pll_property.cfgreg = 0x6000008c, .u.pll_property.refclkreg = 0x60000140,
	 .u.pll_property.offset = 1, .u.pll_property.mask = 1},
	{.clkid = CLK_DDRPLL, .clktype = PLL_CLOCK, .clksource = {CLK_6MHZ, CLK_12MHZ},
	 .u.pll_property.cfgreg = 0x60000090, .u.pll_property.refclkreg = 0x60000140,
	 .u.pll_property.offset = 2, .u.pll_property.mask = 1},
	{.clkid = CLK_VPUPLL, .clktype = PLL_CLOCK, .clksource = {CLK_6MHZ, CLK_12MHZ},
	 .u.pll_property.cfgreg = 0x60000094, .u.pll_property.refclkreg = 0x60000140,
	 .u.pll_property.offset = 3, .u.pll_property.mask = 1},
	{.clkid = CLK_DDR, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M,CLK_DDRPLL},
	 .u.sys_property.cfgreg = 0x60000040, .u.sys_property.index_offset = 24,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = -1,
	 .u.sys_property.div_offset = 26, .u.sys_property.div_mask = 0x7,
	 .u.sys_property.div_value = -1, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
	{.clkid = CLK_CPU, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M,CLK_CPUPLL},
	 .u.sys_property.cfgreg = 0x60000040, .u.sys_property.index_offset = 0,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = -1,
	 .u.sys_property.div_offset = 2, .u.sys_property.div_mask = 0x7,
	 .u.sys_property.div_value = -1, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
	{.clkid = CLK_AHB, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M,CLK_SYSPLL},
	 .u.sys_property.cfgreg = 0x60000040, .u.sys_property.index_offset = 8,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = -1,
	 .u.sys_property.div_offset = 10, .u.sys_property.div_mask = 0x7,
	 .u.sys_property.div_value = -1, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
	{.clkid = CLK_APB, .clktype = SYS_CLOCK, .clksource = {CLK_AHB},
	 .u.sys_property.cfgreg = 0x60000040,
	 .u.sys_property.div_offset = 16, .u.sys_property.div_mask = 0x3,
	 .u.sys_property.div_value = -1, .u.sys_property.div_mode = DIVMODE_DOUBLE,},
	{.clkid = CLK_SPI0, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M, CLK_CPUPLL},
	 .enable_reg = {0x60000064, 0x60000064}, .enable_offset = {30, 31}, .enable_bits = 2,
	.u.sys_property.cfgreg = 0x60000064, .u.sys_property.index_offset = 4,
	.u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 1,
	.u.sys_property.div_offset = 0, .u.sys_property.div_mask = 0xf,
	.u.sys_property.div_value = 4, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
	{.clkid = CLK_SPI1, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M, CLK_CPUPLL},
	 .u.sys_property.cfgreg = 0x60000064, .u.sys_property.index_offset = 20,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 1,
	 .u.sys_property.div_offset = 16, .u.sys_property.div_mask = 0xf,
	 .u.sys_property.div_value = 10, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	{.clkid = CLK_SDMMC0, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M, CLK_SYSPLL},
	 .enable_reg = {0x60000048, 0x60000050, 0x60000058}, .enable_offset = {0, 12, 15}, .enable_bits = 3,
	 .u.sys_property.cfgreg = 0x60000048, .u.sys_property.index_offset = 7,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 0,},
#else
	{.clkid = CLK_SDMMC0, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M, CLK_SYSPLL},
	 .enable_reg = {0x60000048, 0x60000050, 0x60000058}, .enable_offset = {6, 12, 15}, .enable_bits = 3,
	 .u.sys_property.cfgreg = 0x60000048, .u.sys_property.index_offset = 7,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 1,	 
	 .u.sys_property.div_offset = 0, .u.sys_property.div_mask = 0x1f,
	 .u.sys_property.div_value = 10, .u.sys_property.div_mode = DIVMODE_PONEDOUBLE,},
#endif
	{.clkid = CLK_LCD, .clktype = SYS_CLOCK, .clksource = {CLK_SYSPLL, CLK_VPUPLL, CLK_XTAL24M, CLK_XTAL24M},
	 .enable_reg = {0x60000050, 0x60000058, 0x60000058, 0x60000058}, .enable_offset = {3, 18, 17, 16}, .enable_bits = 4,
	 .u.sys_property.cfgreg = 0x6000004c, .u.sys_property.index_offset = 0,
	 .u.sys_property.index_mask = 0x3, .u.sys_property.index_value = 0,
	 .u.sys_property.div_offset = 3, .u.sys_property.div_mask = 0x1f,
	 .u.sys_property.div_value = 11, .u.sys_property.div_mode = DIVMODE_PLUSONE,
#ifdef LCD_CLK_INVERSE
	 .u.sys_property.inv_reg = 0x6000004c, .u.sys_property.inv_offset = 8,
	 .u.sys_property.inv_mask = 0x1, .u.sys_property.inv_value = 1,
#endif
	},
	{.clkid = CLK_TIMER, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M, CLK_CPUPLL},
	 .u.sys_property.cfgreg = 0x60000068, .u.sys_property.index_offset = 4,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 0,
	 .u.sys_property.div_offset = 0, .u.sys_property.div_mask = 0xf,
	 .u.sys_property.div_value = 1, .u.sys_property.div_mode = DIVMODE_PLUSONE,}, 
	{.clkid = CLK_MFC, .clktype = SYS_CLOCK, .clksource = {CLK_SYSPLL, CLK_VPUPLL, CLK_XTAL24M},
	 .enable_reg = {0x60000050, 0x60000058}, .enable_offset = {6, 23}, .enable_bits = 2,
	 .u.sys_property.cfgreg = 0x60000068, .u.sys_property.index_offset = 8,
	 .u.sys_property.index_mask = 0x3, .u.sys_property.index_value = 1,
	 .u.sys_property.div_offset = 11, .u.sys_property.div_mask = 0x1f,
	 .u.sys_property.div_value = -1, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
	{.clkid = CLK_PWM, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M, CLK_240MHZ},
	 .enable_reg = {0x60000054, 0x60000058}, .enable_offset = {10, 10}, .enable_bits = 2,
	 .u.sys_property.cfgreg = 0x60000044, .u.sys_property.index_offset = 8,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 0,
	 .u.sys_property.div_offset = 4, .u.sys_property.div_mask = 0xf,
	 .u.sys_property.div_value = 1, .u.sys_property.div_mode = DIVMODE_PLUSONE,},
	{.clkid = CLK_CAN0, .clktype = SYS_CLOCK, .clksource = {CLK_APB},
	 .enable_reg = {0x60000054}, .enable_offset = {19}, .enable_bits = 1,
	 .u.sys_property.div_value = 1,},
	{.clkid = CLK_CAN1, .clktype = SYS_CLOCK, .clksource = {CLK_APB},
	 .enable_reg = {0x60000054}, .enable_offset = {20}, .enable_bits = 1,
	 .u.sys_property.div_value = 1,},
	{.clkid = CLK_ADC, .clktype = SYS_CLOCK, .clksource = {CLK_XTAL24M},
	 .enable_reg = {0x60000054, 0x60000058}, .enable_offset = {14, 13}, .enable_bits = 2,
	 .u.sys_property.cfgreg = 0x60000068, .u.sys_property.div_offset = 16,
	 .u.sys_property.div_mask = 0x7fff, .u.sys_property.div_value = 642,
	 .u.sys_property.div_mode = DIVMODE_PONEDOUBLE,},
	{.clkid = CLK_I2S, .clktype = SYS_CLOCK, .clksource = {CLK_240MHZ, CLK_SYSPLL},
	 .enable_reg = {0x60000054, 0x60000058, 0x60000058}, .enable_offset = {12, 12, 11}, .enable_bits = 3,
	 .u.sys_property.cfgreg = 0x60000044, .u.sys_property.index_offset = 16,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 0,
	 .u.sys_property.div_value = 1,},
	{.clkid = CLK_I2S1, .clktype = SYS_CLOCK, .clksource = {CLK_240MHZ, CLK_SYSPLL},
	 .enable_reg = {0x60000054, 0x60000144, 0x60000144}, .enable_offset = {22, 1, 0}, .enable_bits = 3,
	 .u.sys_property.cfgreg = 0x60000140, .u.sys_property.index_offset = 4,
	 .u.sys_property.index_mask = 0x1, .u.sys_property.index_value = 0,
	 .u.sys_property.div_value = 1,},
};

#define CLOCK_NUM	(sizeof(xClocks) / sizeof(xClocks[0]))

static xClockProperty *clk_get(uint32_t clkid)
{
	int i;
	
	for (i = 0; i < CLOCK_NUM; i++) {
		if (xClocks[i].clkid == clkid) {
			return &xClocks[i];
		}
	}

	return NULL;
}

static uint32_t clk_fixed_get_rate(xClockProperty *clk)
{
	return clk->u.fixed_freq;
}

static uint32_t clk_fixed_factor_get_rate(xClockProperty *clk)
{
	xClockProperty *parentclk = clk_get(clk->clksource[0]);

	configASSERT(parentclk);
	
	return clk_fixed_get_rate(parentclk) * clk->u.fixed_fator_property.mult
				/ clk->u.fixed_fator_property.div;
}

static uint32_t clk_pll_get_rate(xClockProperty *clk)
{
	uint32_t parent_rate;
	uint32_t div, no, reg;
	
	configASSERT(clk);

	parent_rate = ulClkGetRate(clk->clksource[clk->source_index]);

	reg = readl(clk->u.pll_property.cfgreg);
	no = (reg >> PLL_NO_OFFSET) & PLL_NO_MASK;
	div = (reg >> PLL_DIV_OFFSET) & PLL_DIV_MASK;

	return (parent_rate * div) / (1 << no);
}

static uint32_t clk_sys_get_rate(xClockProperty *clk)
{
	uint32_t parent_rate;
	
	configASSERT(clk);
	configASSERT(clk->div);
	
	parent_rate = ulClkGetRate(clk->clksource[clk->source_index]);

	return parent_rate / clk->div;  
}

static void clk_pll_init(xClockProperty *clk)
{
	configASSERT(clk);
	
	clk->source_index = (readl(clk->u.pll_property.refclkreg) >> clk->u.pll_property.offset)
						& clk->u.pll_property.mask;
}

static int clk_get_div(int div, int divmode)
{
	switch(divmode) {
	case DIVMODE_NOZERO:
		div = div ? div : 1;
		break;
	case DIVMODE_PLUSONE:
		div = div + 1;
		break;
	case DIVMODE_DOUBLE:
		div *= 2;
		break;
	case DIVMODE_EXPONENT:
		div = 1 << div;
		break;
	case DIVMODE_PONEDOUBLE:
		div = (div + 1) * 2;
		break;
	}
	return div;
}

static __INLINE int clk_set_div(int div, int divmode)
{
	switch(divmode) {
	case DIVMODE_PLUSONE:
		div = div - 1;
		break;
	case DIVMODE_DOUBLE:
		div /= 2;
		break;
	case DIVMODE_EXPONENT:
		div = fls(div) - 1;
		break;
	case DIVMODE_PONEDOUBLE:
		div = div / 2 - 1;
		break;
	}
	return div;
}

static void clk_sys_set_rate(xClockProperty *clk, uint32_t freq)
{
	int div;
	uint32_t reg;
	uint32_t parent_rate;

	configASSERT(clk);

	parent_rate = ulClkGetRate(clk->clksource[clk->source_index]);
	div = DIV_ROUND_UP(parent_rate, freq);
	clk->div = div;
	div = clk_set_div(div, clk->u.sys_property.div_mode);
	reg = readl(clk->u.sys_property.cfgreg);
	reg &= ~(clk->u.sys_property.div_mask << clk->u.sys_property.div_offset);
	reg |= (div & clk->u.sys_property.div_mask) << clk->u.sys_property.div_offset;
	writel(reg, clk->u.sys_property.cfgreg);
}

static void clk_sys_init(xClockProperty *clk)
{
	uint32_t reg;
	uint32_t val;
	
	configASSERT(clk);

	if (clk->u.sys_property.index_value >= 0 && clk->u.sys_property.index_mask) {
		clk->source_index = clk->u.sys_property.index_value;
		val = clk->u.sys_property.index_value == 3 ? 4 : clk->u.sys_property.index_value;
		reg = readl(clk->u.sys_property.cfgreg);
		reg &= ~(clk->u.sys_property.index_mask << clk->u.sys_property.index_offset);
		reg |= (val & clk->u.sys_property.index_mask) << clk->u.sys_property.index_offset;
		writel(reg, clk->u.sys_property.cfgreg);
	} else if (clk->u.sys_property.index_mask) {
		reg = readl(clk->u.sys_property.cfgreg);
		val = (reg >> clk->u.sys_property.index_offset) & clk->u.sys_property.index_mask;
		clk->source_index = val == 4 ? 3 : val;
	}

	if (clk->u.sys_property.div_value >= 0 && clk->u.sys_property.div_mask) {
		val = clk_set_div(clk->u.sys_property.div_value, clk->u.sys_property.div_mode);
		clk->div = clk_get_div(val, clk->u.sys_property.div_mode);
		reg = readl(clk->u.sys_property.cfgreg);
		reg &= ~(clk->u.sys_property.div_mask << clk->u.sys_property.div_offset);
		reg |= (val & clk->u.sys_property.div_mask) << clk->u.sys_property.div_offset;
		writel(reg, clk->u.sys_property.cfgreg);		
	} else if (clk->u.sys_property.div_mask) {
		reg = readl(clk->u.sys_property.cfgreg);
		val = (reg >> clk->u.sys_property.div_offset) & clk->u.sys_property.div_mask;
		clk->div = clk_get_div(val, clk->u.sys_property.div_mode);
	} else if (clk->u.sys_property.div_value > 0) {
		clk->div = clk->u.sys_property.div_value;
	} else {
		clk->div = 1;
	}

	if (clk->u.sys_property.inv_reg) {
		reg = readl(clk->u.sys_property.inv_reg);
		reg &= ~(clk->u.sys_property.inv_mask << clk->u.sys_property.inv_offset);
		reg |= (clk->u.sys_property.inv_value & clk->u.sys_property.inv_mask)
				<< clk->u.sys_property.inv_offset;
		writel(reg, clk->u.sys_property.inv_reg);
	}
}

static void clk_sys_enable(xClockProperty *clk)
{
	int i;

	configASSERT(clk);

	for (i = 0; i < clk->enable_bits; i++)
		writel(readl(clk->enable_reg[i]) | (1 << clk->enable_offset[i]),
			clk->enable_reg[i]);
}

static void clk_sys_disable(xClockProperty *clk)
{
	int i;

	configASSERT(clk);

	for (i = 0; i < clk->enable_bits; i++)
		writel(readl(clk->enable_reg[i]) & ~(1 << clk->enable_offset[i]),
			clk->enable_reg[i]);
}

void vClkInit(void)
{
	int i;
	xClockProperty *clk;

	for (i = 0; i < CLOCK_NUM; i++) {
		clk = &xClocks[i];
		if (clk->clktype == PLL_CLOCK) {
			clk_pll_init(clk);
		} else if (clk->clktype == SYS_CLOCK) {
			clk_sys_init(clk);
		}
	}
}

uint32_t ulClkGetRate(uint32_t clkid)
{
	xClockProperty *clk = clk_get(clkid);

	if (clk == NULL)
		return 0;

	switch (clk->clktype) {
	case FIXED_CLOCK:
		return clk_fixed_get_rate(clk);
	case FIXED_FACTOR_CLOCK:
		return clk_fixed_factor_get_rate(clk);
	case PLL_CLOCK:
		return clk_pll_get_rate(clk);
	case SYS_CLOCK:
		return clk_sys_get_rate(clk);
	}

	return 0;
}

void vClkSetRate(uint32_t clkid, uint32_t freq)
{
	xClockProperty *clk = clk_get(clkid);

	if (clk == NULL)
		return;

	if (clk->clktype == SYS_CLOCK)
		clk_sys_set_rate(clk, freq);
}

void vClkEnable(uint32_t clkid)
{
	xClockProperty *clk = clk_get(clkid);

	if (clk == NULL)
		return;

	if (clk->clktype == SYS_CLOCK)
		clk_sys_enable(clk);
}

void vClkDisable(uint32_t clkid)
{
	xClockProperty *clk = clk_get(clkid);

	if (clk == NULL)
		return;

	if (clk->clktype == SYS_CLOCK)
		clk_sys_disable(clk);
}

