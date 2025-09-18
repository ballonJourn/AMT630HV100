/*
**********************************************************************
Copyright (c)2009 Arkmicro Technologies Inc.  All Rights Reserved 
Filename: boot.c
Version : 1.00
Date    : 2010.06.29
Author  : Donier
Abstract: Ark2116 SoC boot rom code file.
Note    : The size the code(*.bin) loaded should never exceed 10K Bytes.
History : From the ark2116 SoC boot rom code file.
***********************************************************************
*/
#include "hcn_config.h"
#include "typedef.h"
#include "amt630h.h"
#include "BootModeSel.h"
#include "UartPrint.h"
#include "timer.h"
#include "mmu.h"
#include "sysinfo.h"
#include "spi.h"

#define PROJECT_FOR_DDR_INIT		0
#define PROJECT_FOR_SPIFLASH_LOADER	1
#define PROJECT_FOR_SD_UPDATE		2
#define PROJECT_FOR_JTAG_UPDATE		3
#define PROJECT_FOR_EMMC_LOADER		4
#define PROJECT_FOR_LAUNCH_EMMC		5

#define PROJECT_PURPOSE		PROJECT_FOR_SD_UPDATE

#if PROJECT_PURPOSE == PROJECT_FOR_EMMC_LOADER || PROJECT_PURPOSE == PROJECT_FOR_LAUNCH_EMMC
#if DEVICE_TYPE_SELECT != EMMC_FLASH
#error "you should select emmc flash device type!"
#endif
#endif
						//ok  //err//ok //ok //err//ok //ok
#define SYSPLL_CLK  400	//550//600//550//400//500//300//400
#define CPUPLL_CLK  550	//550//550//550//550//500//550//550
#define DDRPLL_CLK  600	//700//700//700//600//700//700//700	（RGB屏超过600M低温卡死，lvds屏正常）
#define VPUPLL_CLK  240	//240//240//240//240//240//240//240

#define CLK_APB_FREQ	(SYSPLL_CLK * 1000000 / 2)
#define OSC_FREQ		24000000

#define tref    1560 //(unsigned int)((3900*DDR_CLK)/1000)
#define cas     6   
#define twl     5
#define tmrd    4
#define tras    19 //13 //(unsigned int)((45*DDR_CLK)/1000)
#define trc     28 //16 //(unsigned int)((60*DDR_CLK)/1000)
#define trcd    6  //(unsigned int)((15*DDR_CLK)/1000)
#define trfc    45 //37 //(unsigned int)((105*DDR_CLK)/1000)
#define trp     6  //(unsigned int)((15*DDR_CLK)/1000)
#define trrd    5  //(unsigned int)((10*DDR_CLK)/1000)
#define twr     6 //(unsigned int)((15*DDR_CLK)/1000)
#define twtr    4 //(unsigned int)((8*DDR_CLK)/1000)
#define txp     4
#define txsr    200
#define tesr    6
#define tfaw    18 //(unsigned int)((45*DDR_CLK)/1000) 

extern void SetSysPLL(unsigned int freq);
extern void SetCpuPLL(unsigned int freq);
extern void SetDDRPLL(unsigned int freq);
extern void SetVPUPLL(unsigned int freq);
extern void SetXclkAHBclkAPBclk(void);
extern void SetSpiclk(void);
extern void SetGpuclk(void);
extern void SetMfcclk(void);
extern void SwitchTo24MHz(void);
extern void updateFromSD(int chipid);
extern void FlashBurn(void *buf, unsigned int offset, unsigned int size);
extern int wdt_init(unsigned int clk_freq);
extern int EmmcInit(int chipid);
extern void launchEMMC(int chipid);
extern int EmmcBurn(void *buf, unsigned int offset, unsigned int size, int show_progress);

static void delay(volatile UINT32  count )
{
	while(count--);
}

void ddr_rd_clk_config()
{
  unsigned int  i;
  rSYS_DDRCTL1_CFG = 0x01;  //reset pll
  delay(1000);
  rSYS_DDRCTL1_CFG = 0x00;  //enable pll
  delay(100000);
  i = 0x01 << 1 | 200 << 8;
  rSYS_DDRCTL1_CFG = i;
  delay(100000);
}	

void ddr_training_one(void)
{
	int value = 0;

	rSYS_DDRCTL_CFG = (1<<15);
//	value = (0x1c<<0) |(0x1c<<8)|(0x30<<16);
//	value = (0x1c<<0) |(0x1c<<8)|(0x18<<16);  fail 25
//	value = (0x1c<<0) |(0x1c<<8)|(0x20<<16);   25 ok
//	value = (0x1c<<0) |(0x1c<<8)|(0x28<<16);   25 ok
//	value = (0x1c<<0) |(0x1c<<8)|(0x30<<16);
//	value = (0x1c<<0) |(0x1c<<8)|(0x26<<16);    //4 ok
//	value = (0x1c<<0) |(0x1c<<8)|(0x20<<16);    //1 ok
//	value = (0x1c<<0) |(0x1c<<8)|(0x30<<16);    //4 ok  0514
	value = (0x1c<<0) |(0x1c<<8)|(0x33<<16);    //4  ok
//	value = (0x1c<<0) |(0x1c<<8)|(0x3b<<16);    //4  ok

	rSYS_DDRCTL1_CFG =value;
	rSYS_DDRCTL2_CFG =value;
}

#if 0
#define DDR_TOTAL 0x10000
int DDR3_RW_Test(void)
{

	unsigned int i=0;
	unsigned int  data_char;

	for (i=0; i<DDR_TOTAL; i++)
	{
		(*(volatile unsigned int *)(0x20000000 + (i*4))) =i+0x04;
		//PrintVariableValueHex("write is %x",i+4);
	}

	for (i=0; i<DDR_TOTAL;i++)
	{
		data_char =(*(volatile unsigned int *)(0x20000000 + (i*4 )));
		// PrintVariableValueHex("read is %x",data_char);
		if(data_char!=(i+0x04))
		//  if(data_char!=0x55aabbcc)
		{
			// SendUartString("\r\nddr error\r\n");
			//  PrintVariableValueHex("read is ",data_char);
			//  PrintVariableValueHex("addr is ",(0x20000000 + (i*4 )));
			break;
		}
		// PrintVariableValueHex("datachar11",data_char);
	}
	SendUartString("ddr1  test1 end \n");

	if(i==DDR_TOTAL)
	{
		SendUartString("ddr1 test1 ok \n");
	}

	for (i=0; i<DDR_TOTAL; i++)
	{
		(*(volatile unsigned int *)(0x20000000 + (i*4))) = 0x20000000 + (i*4);
		//PrintVariableValueHex("write is ",i+4);
		data_char =(*(volatile unsigned int *)(0x20000000 + (i*4 )));
		if(data_char!=(0x20000000 + (i*4 )))
		{
			//  SendUartString("\ddr error\n");
			// PrintVariableValueHex("read is",data_char);
			// PrintVariableValueHex("addr  is ",(0x20000000 + (i*4 )));
			break;
		}
	}
	SendUartString("\nddr1 test2 end \n");

	if(i==DDR_TOTAL)
	{
		SendUartString("ddr1 test2 ok \n");
		return 1;
	}

	return 0;
}

void ddr_training(void)
{
	int i=0;
	int j=0;
	int value =0 ;
	unsigned  int result;

	rSYS_DDRCTL_CFG = (1<<15); 

	//printk("rd_dly:(0x08----0x30);qs_dly:(0x00------0x40)\r\n");
	//printk("rd_dly: ");
	//for(j=0x08;j<0x30;j=j+1)
	//printk(" %2d",j);
	//printk("\r\n");

	for(j=0x18; j<0x30; j++)
//  for(i=0x00;i<0x40;i++)
	{
		//printk("    0x%2x ",i);
		for(i=0x00;i<0x40;i++)
		//for(j=0x18;j<0x30;j++)
		{
			udelay(3);
			//j = 0x18;
			udelay(3);
			value = i |(i<<8)|(j<<16);  //|i<<16 ;
			*(volatile unsigned int *)0x60000074 =value;
			*(volatile unsigned int *)0x600000b0 =value;
			udelay(3000);
			result = DDR3_RW_Test();
			udelay(3000);
			if(result)
				SendUartString("O");
			else
				SendUartString("X");
			udelay(3);
		}
		SendUartString("\r\n");
	}
}
#endif

void ddr3_sdramc_init(void)
{
     unsigned long i;
	 //unsigned int rdata=0;

	//i = rSYS_DDRCTL1_CFG;
	//i &= ~((0xFF << 24) |(0xFF << 16) | (0x7F << 8) | 0x7F);
	//i |= (0x40 << 24) |(0x40 << 8) | 0x40;
	//rSYS_DDRCTL1_CFG = i;

   ddr_training_one();

   rSYS_DDRCTL3_CFG = (0x0 <<11 | 0x01 <<10);

	//ddr initialization
    // i= 0x2;
    // READ_DELAY_REG = i;
     //active_chip       qos      burst       stop     auto_power_down  power_down_cycle     row        col
     i = 0x0 << 21  | 0x0 <<18 | 0x2 << 15 | 0x0 <<14 |  0x0 <<13     |   0x0 << 7       |   0x2 <<3 |  0x2<< 0;
     MEM_CFG_REG = i;
     //  mem_width  bank_bit   cke_init   dqm_init   clk_cfg
     i = 0x0 <<6 | 0x0 << 4 | 0x0 <<3 |  0x0 <<2  |  0x0 << 0;
     MEM_CFG2_REG = i;
     // ref timeout
     i = 0x1 << 0;
     MEM_CFG3_REG = i;
     //addr_fmt     addr_match   addr_mask
     i = 0x1 << 16  | 0x20 <<8 | 0xfc << 0;
     CHIP_CFG_REG = i;
     //wr_block     | early_resp
     i= 0x0<<2     | 0x0 << 0;
     FEA_CTL_REG = i;
     //tref
     i= tref;
     //i= 0x300;
     REF_PRD_REG = i;
     //cas
     i= cas<<1;
     TCAS_REG = i;
     //twl
     //i=  0x1<< 0;
     i=  twl<< 0;
     TWL_REG = i;
     //tmrd
     //i=  0x10<< 0;
     i=  tmrd<< 0;
     TMRD_REG = i;
     //tras
     i=  (tras)<< 0;
     TRAS_REG = i;
     //trc
     i=  trc << 0;
     TRC_REG = i;
     //trcd        schelue rcd
     //i=  0x2<< 0 | 0x2<<3;
     i=  trcd<< 0 | (trcd-3)<<8;
     TRCD_REG = i;
     //schelue trfc    trfc
     //i=  15<<5|18<< 0;
     i=  (trfc-3)<<8|trfc<< 0;
     TRFC_REG = i;
     //trp
     //i=  0x3<< 0 | 0x2 << 3;
     i=  trp<< 0 | (trp-3) << 8;
     TRP_REG = i;
     //trrd
     //i=  0x6<< 0;
     i=  trrd<< 0;
     TRRD_REG = i;
     //twr
     i=  twr<< 0;
     TWR_REG = i;
     //twtr
     //i=  0x7<< 0;
     i=  twtr<< 0;
     TWTR_REG = i;
     //txp
     //i=  0x40<< 0;
     i=  txp<< 0;
     TXP_REG = i;
     //txsr
     //i=  199<< 0;
     i=  txsr<< 0;
     TXSR_REG = i;
     //tesr
     i=  tesr<< 0;
     TESR_REG = i;
     //tfaw
     i=  tfaw<< 0|(tfaw-3)<<8;
     TFAW_REG = i;

    //
    // //precharge
     DIR_CMD_REG = 0x000c0000 ;  //direct_cmd_reg nop
     DIR_CMD_REG = 0x00000000 ;  //direct_cmd_reg precharge
     DIR_CMD_REG = 0x000a0000 ;  //direct_cmd_reg xmodereg2 set
     DIR_CMD_REG = 0x000b0000 ;  //direct_cmd_reg xmodereg3 set
     DIR_CMD_REG = 0x00090000 |(0<<6)|(1<<2);  //direct_cmd_reg xmodereg set

   //  DIR_CMD_REG = 0x00080952 ;  //direct_cmd_reg modereg set
     DIR_CMD_REG = 0x00080962 ;    //direct_cmd_reg modereg set
     DIR_CMD_REG = 0x00000000 ;    //direct_cmd_reg precharge
     DIR_CMD_REG = 0x00040000 ;    //direct_cmd_reg autorefresh
     DIR_CMD_REG = 0x00040000 ;    //direct_cmd_reg autorefresh
    // DIR_CMD_REG = 0x00080852 ;  //direct_cmd_reg modereg set
     DIR_CMD_REG = 0x00080862 ;    //direct_cmd_reg modereg set
    // DIR_CMD_REG = 0x00090380 ;  //direct_cmd_reg xmodereg1 set
    // DIR_CMD_REG = 0x00090000 ;  //direct_cmd_reg xmodereg1 set

     MEM_CMD_REG = 0x00000000 ;  //mem_cmd_reg go

     while ((MEM_STA_REG &0x01)!=0x01);

     //ddr_training();	//Only for test.
     // *(volatile unsigned int *)0x60000074 = 0x9;  //0x6;//0xD;
}

void updateFromJtag(void)
{
	unsigned int loader_addr = 0x20000000;
#if DEVICE_TYPE_SELECT == EMMC_FLASH	    
	unsigned int launch_addr = 0x20008000;
#endif    
	unsigned int stepldr_addr = 0x20010000;
	unsigned int app_addr = 0x20100000;
	UpFileHeader *header = (UpFileHeader *)app_addr;

	SysInfo *sysinfo = GetSysInfo();
	sysinfo->app_checksum = header->checksum;
	sysinfo->stepldr_offset = STEPLDRA_OFFSET;
	sysinfo->stepldr_size = STEPLDR_MAX_SIZE;
	sysinfo->update_status = UPDATE_STATUS_END;
	sysinfo->image_offset = IMAGE_OFFSET;
	sysinfo->loader_offset = LOADER_OFFSET;
	sysinfo->loader_size = LOADER_MAX_SIZE;

#if DEVICE_TYPE_SELECT != EMMC_FLASH
	SendUartString("burn loader start... \r\n");
	FlashBurn((void*)loader_addr, LOADER_OFFSET, LOADER_MAX_SIZE);
	SendUartString("burn loader end. \r\n");

	SendUartString("burn stepldr start... \r\n");
	FlashBurn((void*)stepldr_addr, STEPLDRA_OFFSET, STEPLDR_MAX_SIZE);
	SendUartString("burn stepldr end. \r\n");	
	
	SendUartString("burn app start... \r\n");
	FlashBurn((void*)app_addr, IMAGE_OFFSET, header->size);
	SendUartString("burn app end. \r\n");
#else
	EmmcInit(0);

	SendUartString("burn launch start... \r\n");
	FlashBurn((void*)launch_addr, LOADER_OFFSET, LOADER_MAX_SIZE);
	SendUartString("burn launch end. \r\n");

	SendUartString("burn loader start... \r\n");
	EmmcBurn((void*)loader_addr, LOADER_OFFSET, LOADER_MAX_SIZE, 0);
	SendUartString("burn loader end. \r\n");

	SendUartString("burn stepldr start... \r\n");
	EmmcBurn((void*)stepldr_addr, STEPLDRA_OFFSET, STEPLDR_MAX_SIZE, 0);
	SendUartString("burn stepldr end. \r\n");

	SendUartString("burn app start... \r\n");
	EmmcBurn((void*)app_addr, IMAGE_OFFSET, header->size, 0);
	SendUartString("burn app end. \r\n");
#endif

	SaveSysInfo(sysinfo);

	SendUartString("Update is finished. Please reset. \r\n");
	while(1);
}

void main(void)
{
	unsigned int val;

	//set all io drive to 4ma(default 8ma).
	rSYS_IO_DRIVER00=0x55555555;
	rSYS_IO_DRIVER01=0x55555555;
	rSYS_IO_DRIVER02=0x55555555;
	rSYS_IO_DRIVER03=0x55555555;
	rSYS_IO_DRIVER04=0x55555555;
	rSYS_IO_DRIVER05=0x55555555;
	rSYS_IO_DRIVER06=0x55555555;

#if PROJECT_PURPOSE == PROJECT_FOR_LAUNCH_EMMC
	timer_init();
	InitUart(115200);
	SendUartString("\nARK AMT630Hv100 launch emmc from norflash\r\n");
	wdt_init(OSC_FREQ);
	launchEMMC(0);
#endif

	SwitchTo24MHz();
	timer_init();		
	InitUart(115200);
	SendUartString("\nARK AMT630Hv100 AMTLDR 0906_600M\n");
	SendUartString("\nARK AMT630Hv100 sys all drive io 2ma\n");

/*
	val = rSYS_ANA1_CFG;
	val |= (1 << 4)|(5 << 1);
	rSYS_ANA1_CFG = val;
	udelay(300);
	val = rSYS_ANA1_CFG;
	val &= ~(0x1 <<5);
	rSYS_ANA1_CFG = val;
*/
#if 1	//低电压2.8V复位
    rSYS_ANA1_CFG |= (1 << 4)|(5 << 1);
    udelay(300);
    rSYS_ANA1_CFG |= (1 << 5);
    udelay(100);
#else	//低电压2.5V复位
    rSYS_ANA1_CFG |= (1 << 4)|(0 << 1);
    udelay(3000);
    rSYS_ANA1_CFG |= (1 << 5);
    udelay(1000);
#endif
	val = rSYS_ANA2_CFG;
	val = (0x3F <<6)|(1 << 2)|(1 << 0);
	rSYS_ANA2_CFG = val;

    SetSysPLL(SYSPLL_CLK);
    SetCpuPLL(CPUPLL_CLK);
    SetDDRPLL(DDRPLL_CLK);
    SetVPUPLL(VPUPLL_CLK);
    udelay(500);

    SetXclkAHBclkAPBclk();
    SetSpiclk();
    SetGpuclk();
    SetMfcclk();
    mdelay(50);	//50

#if PROJECT_PURPOSE == PROJECT_FOR_SPIFLASH_LOADER || PROJECT_PURPOSE == PROJECT_FOR_EMMC_LOADER
	wdt_init(CLK_APB_FREQ);
#endif

	ddr3_sdramc_init();
	mdelay(50);	//80//100
	SendUartString("\nDDR init over2!!_m50\n");
	SpiInit();
#ifdef MMU_ENABLE
	MMU_Init();
#endif

#if PROJECT_PURPOSE == PROJECT_FOR_DDR_INIT
	while(1);
#elif PROJECT_PURPOSE == PROJECT_FOR_SPIFLASH_LOADER
	bootFromSPI();
#elif PROJECT_PURPOSE == PROJECT_FOR_SD_UPDATE
	SetDefaultSysInfo();
	SaveSysInfo(0);
	updateFromSD(0);
	bootFromSPI();
#elif PROJECT_PURPOSE == PROJECT_FOR_JTAG_UPDATE
	updateFromJtag();
#elif PROJECT_PURPOSE == PROJECT_FOR_EMMC_LOADER
	bootFromEMMC(0);
#endif
}
