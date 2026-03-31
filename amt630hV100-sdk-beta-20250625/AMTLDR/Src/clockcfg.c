#include "amt630h.h"

static void delay(volatile int  count )
{
	while(count--);
}

void SetCpuPLL(unsigned int freq_mhz)
{	
	rSYS_CPUPLL_CFG &= ~(1<<14);
	delay(10);

	rSYS_BUS_CLK1_SEL |= (1<< 0);
	rSYS_CPUPLL_CFG = (freq_mhz/6)|(0x1C<<8)|(1<<14)|(1<<15);
}

void SetSysPLL(unsigned int freq_mhz)
{
	rSYS_SYSPLL_CFG &= ~(1<<14);
	delay(10);

	rSYS_BUS_CLK1_SEL |= (1<< 1);
	rSYS_SYSPLL_CFG = (freq_mhz/6)|(0x1C<<8)|(1<<14)|(1<<15);
}

void SetDDRPLL(unsigned int freq_mhz)
{
	rSYS_DDRPLL_CFG &= ~(1<<14);
	delay(10);

	rSYS_BUS_CLK1_SEL |= (1<< 2);
//	rSYS_DDRPLL_CFG = (freq_mhz/6)|(0x1C<<8)|(1<<14)|(1<<15);
	rSYS_DDRPLL_CFG = (freq_mhz/6)|(0x1C<<8)|(1<<15);
   
//	rSYS_DDRPLL_CFG = (freq_mhz/12)|(0x0C<<8)|(1<<14)|(1<<15);

	delay(100);
        rSYS_DDRPLL_CFG |= (1<< 14);
}

void SetVPUPLL(unsigned int freq_mhz)
{
	rSYS_VPUPLL_CFG &= ~(1<<14);
	delay(10);

	rSYS_BUS_CLK1_SEL |= (1<< 3);
	rSYS_VPUPLL_CFG = (freq_mhz/6)|(0x1C<<8)|(1<<14)|(1<<15);
}

void  SetXclkAHBclkAPBclk(void)
{
	unsigned int val;
	unsigned int hclk2x_div;
	unsigned int apbclk_div;
	unsigned int main_hclk_sel;
	unsigned int main_hclk2x_sel;
	unsigned int cpuclk_div;
	unsigned int main_cpuclk_sel;
	unsigned int ddrclk2x_div;
	unsigned int main_ddrclk2x_sel;
	
	main_cpuclk_sel = 1;
	cpuclk_div = 0;
	main_hclk2x_sel = 1;
	hclk2x_div = 0;
	main_hclk_sel = 1;
	apbclk_div = 1;
	ddrclk2x_div = 0;
	main_ddrclk2x_sel = 1;
#if 0	
	val = rSYS_BUS_CLK_SEL;	
	val &=~(0xFFFFFFFF);	
	val |= (main_cpuclk_sel<< 0) |(cpuclk_div << 2) | (main_hclk2x_sel << 8) | (hclk2x_div << 7) | (main_hclk_sel << 11) | (apbclk_div << 16);
	
	rSYS_BUS_CLK_SEL = val;
#endif
	
	val = rSYS_BUS_CLK_SEL;	
	val &=~(0xFFFFFFFF);	
	val |= (main_cpuclk_sel<< 0) |(cpuclk_div << 2);	
	rSYS_BUS_CLK_SEL = val;
	delay(2000);
	
	val = rSYS_BUS_CLK_SEL;	
	val |= (main_hclk2x_sel << 8) | (hclk2x_div << 10) | (main_hclk_sel << 15) | (apbclk_div << 16) | (main_ddrclk2x_sel << 24) | (ddrclk2x_div << 26);	
	rSYS_BUS_CLK_SEL = val;
	delay(2000);

}
void  SetSpiclk(void)
{
	unsigned int val;
    val = rSYS_SSP_CLK_CFG;	
	val &=~(0x1F);
	val |= (1<< 4) |(4 << 0);	
	rSYS_SSP_CLK_CFG = val;
}	

void  SetGpuclk(void)
{
	unsigned int val;
    val = rSYS_TIMER1_CLK_CFG;	
	val &=~(0xfF);
	val |= (0<< 3) |(1<< 0);    //00:syspll_clk, 01:cpupll_clk
	rSYS_TIMER1_CLK_CFG = val;
}

void  SetMfcclk(void)
{
  unsigned int val;
  val = rSYS_TIMER_CLK_CFG;	
  val &=~(0xfF<<8);
  val |= (0x00<< 11) |(0x1<< 8);
  rSYS_TIMER_CLK_CFG = val;
}



void SwitchTo24MHz(void)
{
  	rSYS_BUS_CLK_SEL = 0;

	delay(10);
}
