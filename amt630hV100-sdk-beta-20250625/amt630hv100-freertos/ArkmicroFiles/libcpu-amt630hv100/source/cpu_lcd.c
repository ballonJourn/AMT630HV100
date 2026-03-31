
#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"

#if LCD_INTERFACE_TYPE == LCD_INTERFACE_CPU
   
/*
Cpu 屏接口模式。
*/
#define LCD_BASE		REGS_LCD_BASE
#define CPU_SCR_SOFT    (56 * 4)
#define CPU_SCR_CTRL    (57 * 4)  
#define LCD_PARAM1		4 
#define SYS_BASE		REGS_SYSCTL_BASE	


#define WR_Com(address)		WriteCpuCmd(CPU_PANEL_DATA,address);
#define WR_D(data)       	WriteCpuData(CPU_PANEL_DATA,data);

/*
Cpu 控制信号数据位。
*/
#define  RS_1   	0X200000
#define  CS_1   	0X100000
#define  RD_1   	0X080000
#define  WR_1  		0X040000

#define  RS_0   	(~0X200000)
#define  CS_0   	(~0X100000)
#define  RD_0   	(~0X080000)
#define  WR_0  		(~0X040000)

void ConfigCpuInit(void)
{
	u32 val = 0;
	val=readl(LCD_BASE + LCD_PARAM1);
	val&=~(BIT(19)|BIT(18)|BIT(17)|BIT(3)|BIT(2)|BIT(1));
	val|=(0x06<<1);//CPU MODE    RS RD CS NORMAL
	writel(val, LCD_BASE + LCD_PARAM1);
}

void ConfigCpuPadmux(u8 Interface)
{
    u32 val = 0;

   	//cpu_screen_rst cpu_scr_cs cpu_scr_wr cpu_scr_rs cpu_scr_rd

	val=readl(SYS_BASE + SYS_PAD_CTRL05);
	val &= ~0x00FFC000;
	      // rd     rs     wr     cs     reset
	val |= 1<<22|1<<20|1<<18|1<<16|1<<14;

    writel(val,SYS_BASE + SYS_PAD_CTRL05);
	
	switch(Interface) {
	case CPU_PANEL_18BIT_MODE:
		val = readl(SYS_BASE + SYS_PAD_CTRL04);
		val &= ~0xFFFFFFFF;
		//d15   d14    d13   d12   d11   d10     d19     d8     d7    d6     d5    d4    d3   d2   d1   d0
		val |= 1<<30|1<<28|1<<26|1<<24|1<<22|1<<20|1<<18|1<<16|1<<14|1<<12|1<<10|1<<8|1<<6|1<<4|1<<2|1<<0;//_018BIT_MODE 接D0-D17
		writel(val,SYS_BASE + SYS_PAD_CTRL04);
		val = readl(SYS_BASE + SYS_PAD_CTRL05);
		val &= ~0x0000000f;
		//     D17     D16
		val |= 1<<2|1<<0;
		writel(val,SYS_BASE + SYS_PAD_CTRL05);
		break;

	case CPU_PANEL_16BIT_MODE:
		val =readl(SYS_BASE + SYS_PAD_CTRL04);
		val &= ~0xFFFFFFF0;
		//d15   d14    d13   d12   d11   d10     d19   d8      d7    d6    d5    d4   d3    d2
		val |= 1<<30|1<<28|1<<26|1<<24|1<<22|1<<20|1<<18|1<<16|1<<14|1<<12|1<<10|1<<8|1<<6|1<<4;//_016BIT_MODE 接D2-D17
		writel(val,SYS_BASE + SYS_PAD_CTRL04);
		val = readl(SYS_BASE + SYS_PAD_CTRL05);
		val &= ~0x0000000f;
		//     D17     D16
		val |= 1<<2|1<<0;

		writel(val,SYS_BASE + SYS_PAD_CTRL05);
		break;

	case CPU_PANEL_9BIT_MODE:
		val = readl(SYS_BASE + SYS_PAD_CTRL04);
		val &= ~0xFFFC0000;
		//d15   d14    d13   d12   d11   d10     d19
		val |= 1<<30|1<<28|1<<26|1<<24|1<<22|1<<20|1<<18;//_09BIT_MODE 接D9-D17
		writel(val,SYS_BASE + SYS_PAD_CTRL04);
		val = readl(SYS_BASE + SYS_PAD_CTRL05);
		val &= ~0x0000000f;
		//     D17     D16
		val |= 1<<2|1<<0;
		writel(val,SYS_BASE + SYS_PAD_CTRL05);
		break;

	case CPU_PANEL_8BIT_MODE:
		val = readl(SYS_BASE + SYS_PAD_CTRL04);
		val &= ~0xFFF00000;
		//d15     d14    d13    d12   d11   d10                                                             
		val |= 1<<30|1<<28|1<<26|1<<24|1<<22|1<<20;//_08BIT_MODE 接D10-D17
		writel(val,SYS_BASE + SYS_PAD_CTRL04);
		val=readl(SYS_BASE + SYS_PAD_CTRL05);
		val &= ~0x0000000f;
		//     D17     D16
		val |= 1<<2|1<<0;
		writel(val,SYS_BASE + SYS_PAD_CTRL05);
		break;

	default:
		break;
	}
}

void CPU_Pannel_Reset(void)
{ 	
	u32 val=0;
	val=readl(LCD_BASE + CPU_SCR_CTRL);

	val|= BIT(9);  
	writel(val,LCD_BASE + CPU_SCR_CTRL);//cpu reset 1
	mdelay(20);

	val &= ~(BIT(9)); 
	writel(val,LCD_BASE + CPU_SCR_CTRL);//cpu reset 0
	mdelay(20);

	val|= BIT(9);  
	writel(val,LCD_BASE + CPU_SCR_CTRL);//cpu reset 1
	mdelay(20); 
}

void SetCpuSoftwareMode()
{		
	u32 val=0;

	val=readl(LCD_BASE + CPU_SCR_CTRL);
	val |=BIT(0);
	writel(val,LCD_BASE + CPU_SCR_CTRL);
}

void SetCpuHardwareMode()
{
	u32 val=0;
	val=readl(LCD_BASE + CPU_SCR_CTRL);
	val &=~(BIT(0));
	writel(val,LCD_BASE + CPU_SCR_CTRL);
}

u32 transfor18BitDate(u8 Interface,u16 dat)
{

    u32 tempDat = 0x00;

    switch(Interface)
	{
	   case CPU_PANEL_18BIT_MODE:
			tempDat = (dat>>8)&0xff;
			tempDat |= dat & 0xff;
	   	    break;

	   case CPU_PANEL_16BIT_MODE:
			tempDat = (dat>>8)&0xff;
			tempDat <<= 9;
			tempDat |= dat & 0xff;
			tempDat <<= 1;
	   	    break;

	   case CPU_PANEL_9BIT_MODE:
			tempDat = dat&0xff;
			tempDat <<= 10;
	   	    break;

	   case CPU_PANEL_8BIT_MODE:
			tempDat = dat&0xff;
			tempDat <<= 10;
	   	    break;

	   default:
	   	   break;
	}
	return tempDat;
}

void WriteCpuCmd(u8 Interface,u16 Val)
{
	u32 tmpDat;
	tmpDat=transfor18BitDate(Interface,Val);
	tmpDat |=( RD_1);
	tmpDat &=( CS_0);
	tmpDat &=( RS_0);
	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
	tmpDat&=( WR_0);
	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
	tmpDat |=( WR_1);
	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
	tmpDat |=( CS_1); 
	tmpDat &=( RS_0);
	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
    
}

void WriteCpuData(u8 Interface,u16 Val)
{
	u32 tmpDat;
	tmpDat=transfor18BitDate(Interface,Val);
	tmpDat |=( RD_1);
	tmpDat &=( CS_0);
	tmpDat |=( RS_1);

	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);

	tmpDat&=( WR_0);
	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
	tmpDat |=( WR_1);

	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
	tmpDat |=( CS_1);
	tmpDat |=( RS_1); 

	writel(tmpDat,LCD_BASE + CPU_SCR_SOFT);
}

void InitalCPU_TCXD035ABFON_8bit(void)
{
	uint16_t i,j,k=0;

	WR_Com(0x5E);  // SET password		
	WR_D(0xA5);
	mdelay(100);
	WR_Com(0x49);  // SET password		
	WR_D(0x0E);
	WR_D(0x00);
	WR_D(0x00);
	WR_D(0xA5);

	WR_Com(0x61);      // Set Power Control
	WR_D(0x8F);      // n=2, all power on
	WR_D(0x44);      
	WR_D(0x02);
	WR_D(0xA5); 

	WR_Com(0x5A);      // Set Power Control
	WR_D(0x70);      // n=2, all power on
	WR_D(0x21);      
	WR_D(0xA5);
	WR_D(0xA5); 

	WR_Com(0x71);     //Set Electronic Volume1
	WR_D(0x1E);     //VCOM=-0.675V
	WR_D( 0x0B);     //VGH=15V
	WR_D(0x0B);     //VGL=-10V
	WR_D(0xA5);

	WR_Com( 0x72);     //Set Electronic Volume2
	WR_D(0x17);     //GVDD=5.4V
	WR_D(0x17);     //GVCL=-5.4V
	WR_D(0xA5);
	WR_D(0xA5);

	WR_Com( 0x81);     //Set Gamma Positive1
	WR_D(0x00);     
	WR_D(0x16);
	WR_D(0x1B);
	WR_D(0x1C);

	WR_Com(0x82);     //Set Gamma Positive2
	WR_D(0x1E);
	WR_D(0x1F);
	WR_D(0x20);
	WR_D(0x21);

	WR_Com(0x83);    //Set Gamma Positive3
	WR_D(0x23);
	WR_D(0x24);
	WR_D(0x26);
	WR_D(0x28);

	WR_Com(0x84);  //Set Gamma Positive4
	WR_D(0x2B);
	WR_D(0x2F);
	WR_D(0x34);
	WR_D(0x3F);

	WR_Com(0x89);  //Set Gamma Negative1
	WR_D(0x00);     
	WR_D(0x16);     
	WR_D(0x1B);     
	WR_D(0x1C);     

	WR_Com(0x8A);   //Set Gamma Negative2
	WR_D(0x1E);    
	WR_D(0x1F);    
	WR_D(0x20);    
	WR_D(0x21);    

	WR_Com(0x8B);  //Set Gamma Negative3
	WR_D(0x23);  
	WR_D(0x24);  
	WR_D(0x26);  
	WR_D(0x28);  

	WR_Com(0x8C); //Set Gamma Negative4
	WR_D(0x2B);   
	WR_D(0x2F);   
	WR_D(0x34);   
	WR_D(0x3F);   

	WR_Com(0x21);   //Set Memory Address Control
	WR_D(0x01);     
	WR_D(0xA5);     
	WR_D(0xA5);     
	WR_D(0xA5); 

	WR_Com(0x13);       // SLEEP OUT
	WR_D(0xA5);       // VOFREG

	mdelay(100);

	WR_Com(0x25);       //Set Page Address
	WR_D(0x02);       //from page0
	WR_D(0xA5);       //to page 159
	WR_D(0xA5);      
	WR_D(0xA5);      

	WR_Com(0x22);       //Set Page Address
	WR_D(0x00);       //from page0
	WR_D(0x9F);       //to page 159
	WR_D(0x00);      
	WR_D(0xA5);      
	WR_Com(0x24);
	WR_D(0x00);
	WR_D(0xA5);
	WR_D(0xA5);
	WR_D(0xA5);
	WR_Com(0x23);      //Set Column Address
	WR_D(0x00);      //from col0
	WR_D(0x00);     
	WR_D(0x00);      //to col239
	WR_D(0xEF);

	WR_Com(0x12); 
	WR_D(0xA5);
	WR_Com(0x3a); 
	WR_D(0xA5);


	for(i=0;i<38400;i++)
		WR_D(0xff); 

	// 16灰阶
	for(k=0;k<16;k++) {
		for(i=0;i<10;i++) {
			for(j=0;j<240;j++)
			   WR_D(k<<4|k);
		}   
	}
};

void Cpulcd_Init(void)
{
    ConfigCpuInit();
	ConfigCpuPadmux(CPU_PANEL_DATA);
	CPU_Pannel_Reset();
	SetCpuSoftwareMode();
	InitalCPU_TCXD035ABFON_8bit();
	SetCpuHardwareMode();
}

#endif


