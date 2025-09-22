/*
**********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved
Filename: sdmmc.c
Version : 1.0
Date    : 2015.12.01
Abstract: ark1680 soc sd driver
History :
***********************************************************************
*/
#include <string.h>
#include "typedef.h"
#include "amt630h.h"
#include "sdmmc.h"
#include "uart.h"
#include "timer.h"
#include "fs/ff.h"
#include "fs/diskio.h"
#include "os_adapt.h"
#include "sysinfo.h"
#include "update.h"
#include "crc32.h"
#include "cp15.h"

#define SDMMC_BUS_CLK	40000000//CLK_24MHZ

#define INIT_CLOCK 		400000
#define SD_TRUE        	1
#define SD_FAULSE      	0
#define TIMEOUT_SD      10000


#if SD_DEBUG
#define DEBUG_MSG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_MSG(fmt, ...)
#endif

static UINT32 lg_ulChip;
static UINT32 SDHC_Controls[1] = {SDHC0_BASE};
static UINT32 mmc_voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
#define READ_INT32(r)       *((volatile UINT32 *)(r))
#define WRITE_INT32(r,d)    (*((volatile UINT32 *)(r)) = d)

#define SDHC_REG_READ32(c,r)\
    (READ_INT32(SDHC_Controls[(c)] + (r)))
#define SDHC_REG_WRITE32(c,r,v)\
    (WRITE_INT32(SDHC_Controls[(c)] + (r),(v)))

#define SDMMC_CTRL						0x00
#define SDMMC_PWREN						0x04
#define SDMMC_CLKDIV					0x08
#define SDMMC_CLKSRC					0x0C
#define SDMMC_CLKENA					0x10
#define SDMMC_TMOUT						0x14
#define SDMMC_CTYPE						0x18
#define SDMMC_BLKSIZ					0x1C
#define SDMMC_BYTCNT					0x20
#define SDMMC_INTMASK					0x24
#define SDMMC_CMDARG					0x28
#define SDMMC_CMD						0x2C
#define SDMMC_RESP0						0x30
#define SDMMC_RESP1						0x34
#define SDMMC_RESP2						0x38
#define SDMMC_RESP3						0x3C
#define SDMMC_MINTSTS					0x40
#define SDMMC_RINTSTS					0x44
#define SDMMC_STATUS					0x48
#define SDMMC_FIFOTH					0x4C
#define SDMMC_CDETECT					0x50
#define SDMMC_WRTPRT					0x54
#define SDMMC_GPIO						0x58
#define SDMMC_TCBCNT					0x5C
#define SDMMC_TBBCNT					0x60
#define SDMMC_DEBNCE					0x64
#define SDMMC_USRID						0x68
#define SDMMC_VERID						0x6C
#define SDMMC_HCON						0x70
#define SDMMC_UHS_REG					0x74
#define SDMMC_BMOD						0x80
#define SDMMC_PLDMND					0x84
#define SDMMC_DBADDR					0x88
#define SDMMC_IDSTS						0x8C
#define SDMMC_IDINTEN					0x90
#define SDMMC_DATA						0x200
#define SDMMC_FIFO						(SDHC_Controls[lg_ulChip] + 0x200)

/* Interrupt Mask register */
#define SDMMC_INTMSK_ALL	0xffffffff
#define SDMMC_INTMSK_RE		(1 << 1)
#define SDMMC_INTMSK_CDONE	(1 << 2)
#define SDMMC_INTMSK_DTO	(1 << 3)
#define SDMMC_INTMSK_TXDR	(1 << 4)
#define SDMMC_INTMSK_RXDR	(1 << 5)
#define SDMMC_INTMSK_DCRC	(1 << 7)
#define SDMMC_INTMSK_RTO	(1 << 8)
#define SDMMC_INTMSK_DRTO	(1 << 9)
#define SDMMC_INTMSK_HTO	(1 << 10)
#define SDMMC_INTMSK_FRUN	(1 << 11)
#define SDMMC_INTMSK_HLE	(1 << 12)
#define SDMMC_INTMSK_SBE	(1 << 13)
#define SDMMC_INTMSK_ACD	(1 << 14)
#define SDMMC_INTMSK_EBE	(1 << 15)

/* Raw interrupt Regsiter */
#define SDMMC_DATA_ERR	(SDMMC_INTMSK_EBE | SDMMC_INTMSK_SBE | SDMMC_INTMSK_HLE |\
			SDMMC_INTMSK_FRUN | SDMMC_INTMSK_EBE | SDMMC_INTMSK_DCRC)
#define SDMMC_DATA_TOUT	(SDMMC_INTMSK_HTO | SDMMC_INTMSK_DRTO)

/* CTRL register */
#define SDMMC_CTRL_RESET	(1 << 0)
#define SDMMC_CTRL_FIFO_RESET	(1 << 1)
#define SDMMC_CTRL_DMA_RESET	(1 << 2)
#define SDMMC_DMA_EN		(1 << 5)
#define SDMMC_CTRL_SEND_AS_CCSD	(1 << 10)
#define SDMMC_IDMAC_EN		(1 << 25)
#define SDMMC_RESET_ALL		(SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET |\
				SDMMC_CTRL_DMA_RESET)

/* CMD register */
#define SDMMC_CMD_RESP_EXP	(1 << 6)
#define SDMMC_CMD_RESP_LENGTH	(1 << 7)
#define SDMMC_CMD_CHECK_CRC	(1 << 8)
#define SDMMC_CMD_DATA_EXP	(1 << 9)
#define SDMMC_CMD_RW		(1 << 10)
#define SDMMC_CMD_SEND_STOP	(1 << 12)
#define SDMMC_CMD_ABORT_STOP	(1 << 14)
#define SDMMC_CMD_PRV_DAT_WAIT	(1 << 13)
#define SDMMC_CMD_UPD_CLK	(1 << 21)
#define SDMMC_CMD_USE_HOLD_REG	(1 << 29)
#define SDMMC_CMD_START		(1UL << 31)

/* CLKENA register */
#define SDMMC_CLKEN_ENABLE	(1 << 0)
#define SDMMC_CLKEN_LOW_PWR	(1 << 16)

/* Card-type registe */
#define SDMMC_CTYPE_1BIT	0
#define SDMMC_CTYPE_4BIT	(1 << 0)
#define SDMMC_CTYPE_8BIT	(1 << 16)

/* Status Register */
#define SDMMC_BUSY		(1 << 9)
#define SDMMC_FIFO_MASK		0x1fff
#define SDMMC_FIFO_SHIFT	17

/* FIFOTH Register */
#define MSIZE(x)		((x) << 28)
#define RX_WMARK(x)		((x) << 16)
#define TX_WMARK(x)		(x)
#define RX_WMARK_SHIFT		16
#define RX_WMARK_MASK		(0xfff << RX_WMARK_SHIFT)

#define fifo_tran_over() do{}while (!(SDHC_REG_READ32(lg_ulChip,SDMMC_STATUS) & 0x00000004))
#define confi_fifo_full() do{}while (!(SDHC_REG_READ32(lg_ulChip,SDMMC_STATUS) & 0x00000008))
//#define confi_fifo_unempty() do{}while ((rSTATUS & 0x00000004))

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const UINT8 multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};

//extern volatile FS_CARD SDCard;

static UINT32 fifoth_val;
static UINT32 sd_ocr;
static UINT32 sd_cid[4];
static UINT32 sd_csd[4];
static UINT32 sd_rca;
static UINT32 read_bl_len;
static UINT32 write_bl_len;
static UINT32 erase_grp_size;
static UINT64 capacity_user;

typedef enum {MMC, SD}CARDTYPE;
static CARDTYPE CardType = SD; // 1 sd card, 0 means MMC
static enum {Ver0, Ver1, Ver2} Spec = Ver1;
static enum {Standard, High} Capacity = Standard;
//static INT8 *sd_spec[3]={"1.0 & 1.01", "1.1", "2.0"};
//static INT8 *mmc_spec[5]={"1.0-1.2", "1.4", "2.0-2.2","3.1-3.2-3.31","4.x"};
//static UINT8 SD_SPEC;
//static UINT8 MMC_SPEC;

static UINT32 g_MatchSdVcc = 1;

#define SDMMC_SECTOR_SIZE	512

static void select_sd_pad(UINT32 ulChip)
{
	UINT32 val;
    UINT32 regval;
	val = rSYS_BOOT_SAMPLE;
	val = (val>>2)&0x1;

	if(val == 1)
	{		/*sd0 128pin*/
		regval = rSYS_PAD_CTRL01;
		regval &= ~((0x3<<12)|(0x3<<10)|(0x3<<8)|(0x3<<6)|(0x3<<4)|(0x3<<2)|(0x3<<0));
		regval |=  ((0x1<<12)|(0x1<<10)|(0x1<<8)|(0x1<<6)|(0x1<<4)|(0x1<<2)|(0x1<<0));
		rSYS_PAD_CTRL01 = regval;
		regval = rSYS_PAD_CTRL07;
		regval &= ~((0x1<<26)|(0x1<<25)|(0x1<<24)|(0x1<<2)|(0x1<<1)|(0x1<<0));
		rSYS_PAD_CTRL07 = regval;
	}
	else 
	{
	   /*sd0 96pin*/
	    regval = rSYS_PAD_CTRL01;
		regval &= ~((0x3<<20)|(0x3<<18)|(0x3<<16)|(0x3<<14));
		regval |=  ((0x3<<20)|(0x3<<18)|(0x3<<16)|(0x3<<14));
		rSYS_PAD_CTRL01 = regval;	 
		
	    regval = rSYS_PAD_CTRL02;
		regval &= ~((0x3<<28)|(0x3<<26)|(0x3<<24));
		regval |=  ((0x2<<28)|(0x2<<26)|(0x2<<24));
		rSYS_PAD_CTRL02 = regval;	
		
	    regval = rSYS_PAD_CTRL07;
		regval &= ~((0x1<<31)|(0x1<<26)|(0x1<<25)|(0x1<<24));
		regval |=  (0x1<<31)|(0x1<<26)|(0x1<<25)|(0x1<<24);
		rSYS_PAD_CTRL07 = regval;		
		
	}
//	rSYS_SDMMC_CLK_CFG |= (1<<6);
}

static int sdmmc_wait_reset(UINT32 value)
{
	unsigned long timeout = 1000;
	UINT32 ctrl;

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CTRL, value);

	while (timeout--) {
		ctrl = SDHC_REG_READ32(lg_ulChip, SDMMC_CTRL);
		if (!(ctrl & SDMMC_RESET_ALL))
			return -1;
	}
	return 0;
}

static int sdmmc_data_transfer(struct mmc_data *data)
{
	int ret = 0;
	UINT32 timeout = 240000;
	UINT32 mask, size, i, len = 0;
	UINT32 *buf = NULL;
	ULONG start = get_timer(0);
	UINT32 fifo_depth = (((fifoth_val & RX_WMARK_MASK) >>
			    RX_WMARK_SHIFT) + 1) * 2;

	size = data->blocksize * data->blocks / 4;
	if (data->flags == MMC_DATA_READ)
		buf = (unsigned int *)data->dest;
	else
		buf = (unsigned int *)data->src;

	for (;;) {
		mask = SDHC_REG_READ32(lg_ulChip, SDMMC_RINTSTS);
		/* Error during data transfer. */
		if (mask & (SDMMC_DATA_ERR | SDMMC_DATA_TOUT)) {
			DEBUG_MSG("%s: DATA ERROR!\n", __func__);
			ret = -1;
			break;
		}

		if (size) {
			len = 0;
			if (data->flags == MMC_DATA_READ &&
			    (mask & SDMMC_INTMSK_RXDR)) {
				while (size) {
					len = SDHC_REG_READ32(lg_ulChip, SDMMC_STATUS);
					len = (len >> SDMMC_FIFO_SHIFT) &
						    SDMMC_FIFO_MASK;
					len = min(size, len);
					for (i = 0; i < len; i++)
						*buf++ = SDHC_REG_READ32(lg_ulChip, SDMMC_DATA);
					size = size > len ? (size - len) : 0;
				}
				SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS,
					     SDMMC_INTMSK_RXDR);
			} else if (data->flags == MMC_DATA_WRITE &&
				   (mask & SDMMC_INTMSK_TXDR)) {
				while (size) {
					len = SDHC_REG_READ32(lg_ulChip, SDMMC_STATUS);
					len = fifo_depth - ((len >>
						   SDMMC_FIFO_SHIFT) &
						   SDMMC_FIFO_MASK);
					len = min(size, len);
					for (i = 0; i < len; i++)
						SDHC_REG_WRITE32(lg_ulChip, SDMMC_DATA,
							     *buf++);
					size = size > len ? (size - len) : 0;
				}
				SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS,
					     SDMMC_INTMSK_TXDR);
			}
		}

		/* Data arrived correctly. */
		if (mask & SDMMC_INTMSK_DTO) {
			ret = 0;
			break;
		}

		/* Check for timeout. */
		if (get_timer(start) > timeout) {
			DEBUG_MSG("%s: Timeout waiting for data!\n",
			      __func__);
			ret = -1;
			break;
		}
	}

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS, mask);

	return ret;
}

static int sdmmc_set_transfer_mode(struct mmc_data *data)
{
	unsigned long mode;

	mode = SDMMC_CMD_DATA_EXP;
	if (data->flags & MMC_DATA_WRITE)
		mode |= SDMMC_CMD_RW;

	return mode;
}

static int mmc_send_cmd(struct mmc_cmd *cmd, struct mmc_data *data)
{
	int ret = 0, flags = 0, i;
	unsigned int timeout = 500;
	UINT32 retry = 100000;
	UINT32 mask;
	ULONG start = get_timer(0);

	while (SDHC_REG_READ32(lg_ulChip, SDMMC_STATUS) & SDMMC_BUSY) {
		if (get_timer(start) > timeout) {
			DEBUG_MSG("%s: Timeout on data busy\n", __func__);
			return SDMMC_RW_CMDTIMEROUT;
		}
	}

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS, SDMMC_INTMSK_ALL);

	if (data) {
		SDHC_REG_WRITE32(lg_ulChip, SDMMC_BLKSIZ, data->blocksize);
		SDHC_REG_WRITE32(lg_ulChip, SDMMC_BYTCNT,
			     data->blocksize * data->blocks);
		sdmmc_wait_reset(SDMMC_CTRL_FIFO_RESET);
	}

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CMDARG, cmd->cmdarg);

	if (data)
		flags = sdmmc_set_transfer_mode(data);

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
		return -1;

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		flags |= SDMMC_CMD_ABORT_STOP;
	else
		flags |= SDMMC_CMD_PRV_DAT_WAIT;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		flags |= SDMMC_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			flags |= SDMMC_CMD_RESP_LENGTH;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= SDMMC_CMD_CHECK_CRC;

	flags |= (cmd->cmdidx | SDMMC_CMD_START | SDMMC_CMD_USE_HOLD_REG);

	DEBUG_MSG("Sending CMD%d\n",cmd->cmdidx);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CMD, flags);

	for (i = 0; i < retry; i++) {
		mask = SDHC_REG_READ32(lg_ulChip, SDMMC_RINTSTS);
		if (mask & SDMMC_INTMSK_CDONE) {
			if (!data)
				SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS, mask);
			break;
		}
	}

	if (i == retry) {
		DEBUG_MSG("%s: Timeout.\n", __func__);
		return SDMMC_RW_CMDTIMEROUT;
	}

	if (mask & SDMMC_INTMSK_RTO) {
		/*
		 * Timeout here is not necessarily fatal. (e)MMC cards
		 * will splat here when they receive CMD55 as they do
		 * not support this command and that is exactly the way
		 * to tell them apart from SD cards. Thus, this output
		 * below shall be debug(). eMMC cards also do not favor
		 * CMD8, please keep that in mind.
		 */
		DEBUG_MSG("%s: Response Timeout.\n", __func__);
		return SDMMC_RW_CMDTIMEROUT;
	} else if (mask & SDMMC_INTMSK_RE) {
		DEBUG_MSG("%s: Response Error.\n", __func__);
		return -1;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = SDHC_REG_READ32(lg_ulChip, SDMMC_RESP3);
			cmd->response[1] = SDHC_REG_READ32(lg_ulChip, SDMMC_RESP2);
			cmd->response[2] = SDHC_REG_READ32(lg_ulChip, SDMMC_RESP1);
			cmd->response[3] = SDHC_REG_READ32(lg_ulChip, SDMMC_RESP0);
		} else {
			cmd->response[0] = SDHC_REG_READ32(lg_ulChip, SDMMC_RESP0);
		}
	}

	if (data) {
		ret = sdmmc_data_transfer(data);
	}

	udelay(100);

	return ret;
}

/* static void GetCardInfo(INT32 *CID, CARDTYPE ct)
{
	UINT16 Year;
	UINT8 Month;
	UINT8 CardName[7];
	UINT8 RevisionN;
	UINT8 RevisionM;
	
	if(ct== MMC)
	{
		Month = (UINT8)((CID[0]&0x0000f000)>>12);
		Year = (unsigned long)((CID[0]&0x00000f00)>>8)+1997;
		RevisionN = (UINT8)((CID[1]&0x00f00000)>>20);
		RevisionM = (UINT8)((CID[1]&0x000f0000)>>16);
		CardName[0] = (UINT8)(CID[3]&0x000000ff);
		CardName[1] = (UINT8)((CID[2]&0xff000000)>>24);
		CardName[2] = (UINT8)((CID[2]&0x00ff0000)>>16);
		CardName[3] = (UINT8)((CID[2]&0x0000ff00)>>8);
		CardName[4] = (UINT8)(CID[2]&0x000000ff);
		CardName[5] = (UINT8)((CID[1]&0xff000000)>>24);
		CardName[6] = '\0';
		DEBUG_MSG("Product Name : %s\n", CardName);
		DEBUG_MSG("Product revision : %d.%d\n", RevisionN, RevisionM);
		DEBUG_MSG("Manufacturing date %d-%d\n", Month, Year);
	}
	else
	{
		Month = (UINT8)((CID[0]&0x00000f00)>>8);
		Year = (unsigned long)((CID[0]&0x000ff000)>>12)+2000;
		RevisionN = (UINT8)((CID[1]&0xf0000000)>>28);
		RevisionM = (UINT8)((CID[1]&0x0f00000)>>24);
		CardName[0] = (UINT8)(CID[3]&0x000000ff);
		CardName[1] = (UINT8)((CID[2]&0xff000000)>>24);
		CardName[2] = (UINT8)((CID[2]&0x00ff0000)>>16);
		CardName[3] = (UINT8)((CID[2]&0x0000ff00)>>8);
		CardName[4] = (UINT8)(CID[2]&0x000000ff);
		CardName[5] = '\0';
		DEBUG_MSG("Product Name : %s\n", CardName);
		DEBUG_MSG("Product revision : %d.%d\n", RevisionN, RevisionM);
		DEBUG_MSG("Manufacturing date %d-%d\n", Month, Year);
	}
} */

/* static UINT8 GetSD_SPEC(UINT32 *scr)
{
	UINT8 ss;
	ss=(UINT8)(scr[0]&0x0000000f);
	
	return ss;
}

static UINT8 GetMMC_SPEC(void)
{
	UINT8 ss;
 	ss = (sd_csd[3]>>26)&0x0000000f;
	
	return ss;
} */

INT32 data_tran_over(void)
{
	INT32 count_sd = 0;
  	INT32 tt;

	do
	{
		count_sd++;
		if(count_sd > TIMEOUT_SD)		
			return -1;

		//tt = rSDMMC_RINTSTS;               //Command done
		tt = SDHC_REG_READ32(lg_ulChip, SDMMC_RINTSTS);
	}while (!(tt & 0x00000008));

	if((tt&0xBFC2) != 0)
	{
    		DEBUG_MSG("This rSDMMC_RINTSTS = 0x%x\n", tt);


    		//rSDMMC_RINTSTS = 0x0000ffff;	
    		SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS, 0x0000ffff);
  			return -1;
	}
	//rSDMMC_RINTSTS = 0x0000ffff;
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS, 0x0000ffff);
	 
	return 0;   // 0 means successful
}

static INT32 mmc_go_idle(void)
{
	struct mmc_cmd cmd;
	int err;

	udelay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;

	err = mmc_send_cmd(&cmd, NULL);

	if (err)
		return err;

	udelay(2000);

	return 0;
}

static int mmc_set_blocklen(int len)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;

	err = mmc_send_cmd(&cmd, NULL);

	return err;
}


static int sd_select_bus_width(int w)
{
	int err;
	struct mmc_cmd cmd;

	if ((w != 4) && (w != 1))
		return -1;

	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = sd_rca << 16;

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
	cmd.resp_type = MMC_RSP_R1;
	if (w == 4)
		cmd.cmdarg = 2;
	else if (w == 1)
		cmd.cmdarg = 0;
	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		return err;

	return 0;
}

/* static UINT8 get_width(UINT32 *scr)
{
	if((scr[0]&0x00000f00) == 0x00000500)
		return 4;
	else
		return 1;
} */

static int sdmmc_setup_bus(UINT32 freq)
{
	UINT32 div, status;
	int timeout = 10000;
	unsigned long sclk = SDMMC_BUS_CLK;

	div = sclk / freq;
	if (sclk % freq && sclk > freq)
	  div += 1;
	
	div = (sclk != freq) ? DIV_ROUND_UP(div, 2) : 0;

#if 0	
	if (sclk == freq)
		div = 0;	/* bypass mode */
	else
		div = DIV_ROUND_UP(sclk, 2 * freq);
#endif	

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CLKENA, 0);
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CLKSRC, 0);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CLKDIV, div);
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CMD, SDMMC_CMD_PRV_DAT_WAIT |
			SDMMC_CMD_UPD_CLK | SDMMC_CMD_START);

	do {
		status = SDHC_REG_READ32(lg_ulChip, SDMMC_CMD);
		if (timeout-- < 0) {
			DEBUG_MSG("%s: Timeout!\n", __func__);
			return -1;
		}
	} while (status & SDMMC_CMD_START);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CLKENA, SDMMC_CLKEN_ENABLE |
			SDMMC_CLKEN_LOW_PWR);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CMD, SDMMC_CMD_PRV_DAT_WAIT |
			SDMMC_CMD_UPD_CLK | SDMMC_CMD_START);

	timeout = 10000;
	do {
		status = SDHC_REG_READ32(lg_ulChip, SDMMC_CMD);
		if (timeout-- < 0) {
			DEBUG_MSG("%s: Timeout!\n", __func__);
			return -1;
		}
	} while (status & SDMMC_CMD_START);

	return 0;
}

static int sdmmc_hw_init()
{
	UINT32 fifo_size;
	
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_PWREN, 1);

	if (!sdmmc_wait_reset(SDMMC_RESET_ALL)) {
		DEBUG_MSG("%s[%d] Fail-reset!!\n", __func__, __LINE__);
		return -1;
	}

	/* Enumerate at 400KHz */
	sdmmc_setup_bus(INIT_CLOCK);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_RINTSTS, 0xFFFFFFFF);
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_INTMASK, 0);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_TMOUT, 0xFFFFFFFF);

	SDHC_REG_WRITE32(lg_ulChip, SDMMC_IDINTEN, 0);
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_BMOD, 1);

	fifo_size = SDHC_REG_READ32(lg_ulChip, SDMMC_FIFOTH);
	fifo_size = ((fifo_size & RX_WMARK_MASK) >> RX_WMARK_SHIFT) + 1;
	fifoth_val = MSIZE(0x2) | RX_WMARK(fifo_size / 2 - 1) |
			TX_WMARK(fifo_size / 2);
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_FIFOTH, fifoth_val);

	return 0;
}

static int mmc_send_if_cond()
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = (1 << 8) | 0xaa;
	cmd.resp_type = MMC_RSP_R7;

	err = mmc_send_cmd(&cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return -1;
	else
		Spec = Ver2;

	return 0;
}

static int sd_send_op_cond()
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	while (1) {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(&cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc_voltages & 0xff8000;

		if (Spec == Ver2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(&cmd, NULL);

		if (err)
			return err;

		if (cmd.response[0] & OCR_BUSY)
			break;

		if (timeout-- <= 0)
			return -1;

		udelay(1000);
	}

	sd_ocr = cmd.response[0];
	if ((sd_ocr & OCR_HCS) == OCR_HCS)
		Capacity = High;	
	sd_rca = 0;

	return 0;
}

static int mmc_send_op_cond_iter(int use_arg)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R3;
	cmd.cmdarg = 0;
	if (use_arg)
		cmd.cmdarg = OCR_HCS |
			(mmc_voltages &
			(sd_ocr & OCR_VOLTAGE_MASK)) |
			(sd_ocr & OCR_ACCESS_MODE);

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		return err;
	sd_ocr = cmd.response[0];
	return 0;
}

static int mmc_send_op_cond()
{
	int err, i;

	/* Some cards seem to need this */
	mmc_go_idle();

 	/* Asking to the card its capabilities */
	for (i = 0; i < 2; i++) {
		err = mmc_send_op_cond_iter(i != 0);
		if (err)
			return err;

		/* exit if not busy (flag seems to be inverted) */
		if (sd_ocr & OCR_BUSY)
			break;
	}

	return 0;
}

static int mmc_complete_op_cond()
{
	int timeout = 1000;
	ULONG start;
	int err;

	if (!(sd_ocr & OCR_BUSY)) {
		/* Some cards seem to need this */
		mmc_go_idle();

		start = get_timer(0);
		while (1) {
			err = mmc_send_op_cond_iter(1);
			if (err)
				return err;
			if (sd_ocr & OCR_BUSY)
				break;
			if (get_timer(start) > timeout)
				return -1;
			udelay(100);
		}
	}

	if ((sd_ocr & OCR_HCS) == OCR_HCS);
		Capacity = High;
	sd_rca = 1;

	return 0;
}

int mmc_send_status(int timeout)
{
	struct mmc_cmd cmd;
	int err, retries = 5;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = sd_rca << 16;

	while (1) {
		err = mmc_send_cmd(&cmd, NULL);
		if (!err) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;

			if (cmd.response[0] & MMC_STATUS_MASK) {
				return -1;
			}
		} else if (--retries < 0)
			return err;

		if (timeout-- <= 0)
			break;

		udelay(1000);
	}

	if (timeout <= 0) {
		return -1;
	}

	return 0;
}

int mmc_switch(UINT8 set, UINT8 index, UINT8 value)
{
	struct mmc_cmd cmd;
	int timeout = 1000;
	int retries = 3;
	int ret;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
				 (index << 16) |
				 (value << 8);

	while (retries > 0) {
		ret = mmc_send_cmd(&cmd, NULL);

		/* Waiting for the ready status */
		if (!ret) {
			ret = mmc_send_status(timeout);
			return ret;
		}

		retries--;
	}

	return ret;
}

static int mmc_startup()
{
	int err;
	UINT mult, freq;
	UINT64 cmult, csize;
	struct mmc_cmd cmd;

	/* Put the Card in Identify Mode */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		return err;
	
	memcpy(sd_cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
	cmd.cmdarg = sd_rca << 16;
	cmd.resp_type = MMC_RSP_R6;

	err = mmc_send_cmd(&cmd, NULL);

	if (err)
		return err;

	if (CardType == SD)
		sd_rca = (cmd.response[0] >> 16) & 0xffff;

	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = sd_rca << 16;

	err = mmc_send_cmd(&cmd, NULL);

	if (err)
		return err;

	sd_csd[0] = cmd.response[0];
	sd_csd[1] = cmd.response[1];
	sd_csd[2] = cmd.response[2];
	sd_csd[3] = cmd.response[3];

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];
	read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (CardType == SD)//tf ka
		write_bl_len = read_bl_len;
	else//emmc
		write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);
	
	if (Capacity == High) {
		csize = (sd_csd[1] & 0x3f) << 16
			| (sd_csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (sd_csd[1] & 0x3ff) << 2
			| (sd_csd[2] & 0xc0000000) >> 30;
		cmult = (sd_csd[2] & 0x00038000) >> 15;
	}
	capacity_user = (csize + 1) << (cmult + 2);
	capacity_user *= read_bl_len;
	if (read_bl_len > MMC_MAX_BLOCK_LEN)
		read_bl_len = MMC_MAX_BLOCK_LEN;

	if (write_bl_len > MMC_MAX_BLOCK_LEN)
		write_bl_len = MMC_MAX_BLOCK_LEN;
	
	erase_grp_size = 0x400;
	
	/* Select the card, and put it into Transfer Mode */
	cmd.cmdidx = MMC_CMD_SELECT_CARD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = sd_rca << 16;
	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		return err;

	mmc_set_blocklen(read_bl_len);
	sdmmc_setup_bus(freq * mult);
	if (CardType == SD)
	{ 
		err = sd_select_bus_width(4);
		SDHC_REG_WRITE32(0, SDMMC_CTYPE, 0x00000001);
	}
	else
	{
		err = mmc_switch(EXT_CSD_CMD_SET_NORMAL,
				    EXT_CSD_BUS_WIDTH,
				    EXT_CSD_BUS_WIDTH_4);
		SDHC_REG_WRITE32(0, SDMMC_CTYPE, 0x00000001);
	}
	if (err)
		return err;

	return 0;
}

INT32 sd_mmc_card_identify(void)
{
	INT32 ret;
	INT32 op_cond_pending = 0;
	
  	CardType = SD;
	Capacity = Standard; //need to re_initialize in case of MMC card

	sdmmc_hw_init();
	
	// set the controller 1bit mode
	SDHC_REG_WRITE32(lg_ulChip, SDMMC_CTYPE, 0x00000000);

	if (mmc_go_idle())
		return -1;

	/* Test for SD version 2 */
	mmc_send_if_cond();

	if (g_MatchSdVcc)
		ret = sd_send_op_cond();
	else
		ret = SDMMC_RW_CMDTIMEROUT;

	if (ret == SDMMC_RW_CMDTIMEROUT)
	{
		/* If the command timed out, we check for an MMC card */
		ret = mmc_send_op_cond();
		if (ret) {
			DEBUG_MSG("Card did not respond to voltage select!\n");
			return -1;
		}
		CardType = MMC;
		op_cond_pending = 1;
	}

	if (op_cond_pending)
		mmc_complete_op_cond();

	return 0;
}

static int mmc_read_blocks(void *dst, ULONG start, ULONG blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (Capacity == High)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * read_bl_len;

	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = read_bl_len;
	data.flags = MMC_DATA_READ;

	if (mmc_send_cmd(&cmd, &data))
		return 0;

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(&cmd, NULL)) {
			return 0;
		}
	}

	return blkcnt;
}

static INT32 SD_Identify(UINT32 Unit)
{
	INT32 ret;
	
	ret = sd_mmc_card_identify();
	
//	if(ret)
//		DEBUG_MSG("SD_Identify fails\n");

	return ret;
}

INT32 SD_Init(UINT32 Unit)
{
	INT32 ret;

	ret = mmc_startup();
	
	return ret;
}

INT32 SD_ReadSector(UINT32 Unit,ULONG Sector,UINT8 *pBuffer, ULONG Count)
{
	INT32 ret;

	ret = mmc_read_blocks((UINT32 *)pBuffer, Sector, Count);
	
	return ret;
}

INT32 InitSDMMCCard()
{
	INT32 result = -1;
		
	if (!SD_Identify(lg_ulChip))
	{
		SendUartString("SDMMC card successfully\n");
		if (!SD_Init(lg_ulChip))
		{
			result = 0;
		}
	}
	else
	{
		SendUartString("SDMMC card identify failed\n");
	}

	return result;
}

int  IsCardExist(void)
{
	return !(SDHC_REG_READ32(lg_ulChip, SDMMC_CDETECT) & 0x1);
}  

#ifdef CHECK_FISRT_FILE
static BYTE *g_rBuffer = g_fs.win;

static INT32 check_fatfs(UINT8 *buf)
{
	if (!memcmp(&buf[BS_FilSysType], "FAT", 3))			/* Check FAT signature */
		return 0;
	if (!memcmp(&buf[BS_FilSysType32], "FAT32", 5) && !(buf[BPB_ExtFlags] & 0x80))
		return 0;

	return 1;
}

static INT32 check_loaderfile(void)
{
	BYTE fmt, *tbl;
	DWORD bootsect = 0, fatsize, fatbase, totalsect, csize, n_rootdir, maxclust, dirbase;
	INT i;
	
	if (mmc_read_blocks((UINT32 *)g_rBuffer, 0, 1) != 1)
		return 1;

	fmt = check_fatfs(g_rBuffer);
#if 0	
	if (fmt == 1) {						/* Not an FAT boot record, it may be patitioned */
		/* Check a partition listed in top of the partition table */
		tbl = &g_rBuffer[MBR_Table];	/* Partition table */
		if (tbl[4]) {									/* Is the partition existing? */
			bootsect = LD_DWORD(&tbl[8]);				/* Partition offset in LBA */
			if (mmc_read_blocks((UINT32 *)g_rBuffer, bootsect, 1) != 1)
				return 1;			
			fmt = check_fatfs(g_rBuffer);				/* Check the partition */
		}
	}
#else
	/* Check a first fat partition listed in partition table */	
	for(i = 0; i < 4; i++) {
		tbl = &g_rBuffer[MBR_Table + i * 16];			/* Partition table */		
		if (tbl[4] == 0x01 || tbl[4] == 0x04 || tbl[4] == 0x06 || tbl[4] == 0x0B ||
			tbl[4] == 0x0C || tbl[4] == 0x0E) {			/* Is the FAT partition? */
			bootsect = LD_DWORD(&tbl[8]);				/* Partition offset in LBA */
			if (mmc_read_blocks((UINT32 *)g_rBuffer, bootsect, 1) != 1)
				return 1;			
			fmt = check_fatfs(g_rBuffer);				/* Check the partition */
			break;
		}				
	}
#endif
	if (fmt || LD_WORD(&g_rBuffer[BPB_BytsPerSec]) != 512)	/* No valid FAT patition is found */
	{
		SendUartString("Not found fatfs.\r\n");
		return 2;
	}

	/* Initialize the file system object */
	fatsize = LD_WORD(&g_rBuffer[BPB_FATSz16]);			/* Number of sectors per FAT */
	if (!fatsize) fatsize = LD_DWORD(&g_rBuffer[BPB_FATSz32]);
	fatsize *= g_rBuffer[BPB_NumFATs];								/* (Number of sectors in FAT area) */
	fatbase = bootsect + LD_WORD(&g_rBuffer[BPB_RsvdSecCnt]); /* FAT start sector (lba) */
	csize = g_rBuffer[BPB_SecPerClus];				/* Number of sectors per cluster */
	n_rootdir = LD_WORD(&g_rBuffer[BPB_RootEntCnt]);	/* Nmuber of root directory entries */
	totalsect = LD_WORD(&g_rBuffer[BPB_TotSec16]);		/* Number of sectors on the file system */
	if (!totalsect) totalsect = LD_DWORD(&g_rBuffer[BPB_TotSec32]);
	maxclust = (totalsect				/* max_clust = Last cluster# + 1 */
		- LD_WORD(&g_rBuffer[BPB_RsvdSecCnt]) - fatsize - n_rootdir / (512/32)
		) / csize + 2;

	fmt = FS_FAT12;										/* Determine the FAT sub type */
	if (maxclust >= 0xFF7) fmt = FS_FAT16;
	if (maxclust >= 0xFFF7) fmt = FS_FAT32;

	dirbase = fatbase + fatsize;			/* Root directory start sector (lba) */

	if (fmt == FS_FAT32)
	{
		DWORD clust = LD_DWORD(&g_rBuffer[BPB_RootClus]);
		if(clust <= maxclust)
			dirbase += csize * (clust - 2);	/* Root directory start cluster */
	}

	if (mmc_read_blocks((UINT32 *)g_rBuffer, dirbase, 1) != 1)
		return 1;	

	//有卷标时第一个拷贝的文件位于第二个目录项
	if (!memcmp(&g_rBuffer[0], "ARKSDLDRBIN", 11) || 
		!memcmp(&g_rBuffer[32], "ARKSDLDRBIN", 11)
		)
	{
		return 0;	
	}
	//Win10下格式化会自动生成一个System Volume Information目录
	else if (!memcmp(&g_rBuffer[64], "SYSTEM~1   ", 11))
	{
		if (!memcmp(&g_rBuffer[96], "ARKSDLDRBIN", 11) || 
			!memcmp(&g_rBuffer[128], "ARKSDLDRBIN", 11))
			return 0;
	}
	//有卷标
	else if (!memcmp(&g_rBuffer[96], "SYSTEM~1   ", 11))
	{
		if (!memcmp(&g_rBuffer[128], "ARKSDLDRBIN", 11) || 
			!memcmp(&g_rBuffer[160], "ARKSDLDRBIN", 11))
			return 0;
	}
	else
	{
		SendUartString("Not found ARKSDLDR.bin.\r\n");
		return 3;
	}

	return -1;
}
#endif

extern void UpdateFromMedia(int drv);
void updateFromSD(int chipid)
{
	lg_ulChip = chipid;
	g_MatchSdVcc = 1;	
	select_sd_pad(lg_ulChip);

	if(!IsCardExist())
	{
		SendUartString("Card is not in the slot.\r\n");
		return;
	}
	else	  
	{
		if(InitSDMMCCard())
		{
			SendUartString("InitSDMMCCard fail.\r\n");
			return;
		}		
	}

	UpdateFromMedia(SDMMC);
}

// for fatfs's sd interface
static unsigned long sdmmc_disk_size;
#define SECOTR_SIZE 512

int MMC_disk_initialize()
{
	UINT64 secotrs;

	/* if(InitSDMMCCard() < 0)
	{
		SendUartString("disk mount fail!\r\n");
		return -1;
	}
	else
		SendUartString("disk mount success!\r\n"); */
	
	secotrs = capacity_user / SECOTR_SIZE;
	if(secotrs == -1) return -1;
	//最大支持4G卡。
	if(secotrs * SECOTR_SIZE > (unsigned int)-1 ) 
		secotrs = ((unsigned int)-1)/SECOTR_SIZE;
	sdmmc_disk_size = secotrs * SECOTR_SIZE; 

	return 0;
}

int MMC_disk_read(void *buff, DWORD sector, BYTE count)
{
	return SD_ReadSector(0, sector, buff, count);
}

int MMC_disk_ioctl(BYTE ctrl, void *buff)
{
	switch(ctrl)
	{
	case CTRL_SYNC:
		break;
	case GET_SECTOR_COUNT:
		*(DWORD*)buff = sdmmc_disk_size / SECOTR_SIZE;
		break;
	case GET_BLOCK_SIZE:
		*(DWORD*)buff = 1;
		break;
	default:
		return -1;	
	}
	return 0;	
}
static ulong mmc_erase_t(ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	ulong end;
	int err, start_cmd, end_cmd;

	if (Capacity == High) {
		end = start + blkcnt - 1;
	} else {
		end = (start + blkcnt - 1) * write_bl_len;
		start *= write_bl_len;
	}

	if (0) {
		start_cmd = SD_CMD_ERASE_WR_BLK_START;
		end_cmd = SD_CMD_ERASE_WR_BLK_END;
	} else {
		start_cmd = MMC_CMD_ERASE_GROUP_START;
		end_cmd = MMC_CMD_ERASE_GROUP_END;
	}

	cmd.cmdidx = start_cmd;
	cmd.cmdarg = start;
	cmd.resp_type = MMC_RSP_R1;

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = end_cmd;
	cmd.cmdarg = end;

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = MMC_CMD_ERASE;
	cmd.cmdarg = MMC_ERASE_ARG;
	cmd.resp_type = MMC_RSP_R1b;

	err = mmc_send_cmd(&cmd, NULL);
	if (err)
		goto err_out;

	return 0;

err_out:
	SendUartString("mmc erase failed\n");
	return err;
}

ulong mmc_berase( lbaint_t start, lbaint_t blkcnt)
{
	int err = 0;
	lbaint_t blk = 0, blk_r = 0;
	int timeout = 1000;

	while (blk < blkcnt) {
		if (0/*IS_SD(mmc) && mmc->ssr.au*/) {
			;
			//blk_r = ((blkcnt - blk) > mmc->ssr.au) ?
			//	mmc->ssr.au : (blkcnt - blk);
		} else {
			blk_r = ((blkcnt - blk) > erase_grp_size) ?
				erase_grp_size : (blkcnt - blk);
		}
		err = mmc_erase_t(start + blk, blk_r);
		if (err)
			break;

		blk += blk_r;

		/* Waiting for the ready status */
		if (mmc_send_status(timeout))
			return 0;
	}

	return blk;
}

static ulong mmc_write_blocks(lbaint_t start,
		lbaint_t blkcnt, const void *src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout = 1000;

/*	if ((start + blkcnt) > mmc_get_blk_desc(mmc)->lba) {
		printf("MMC: block number 0x" LBAF " exceeds max(0x" LBAF ")\n",
		       start + blkcnt, mmc_get_blk_desc(mmc)->lba);
		return 0;
	}*/

	if (blkcnt == 0)
		return 0;
	else if (blkcnt == 1)
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;

	if (Capacity == High)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * write_bl_len;

	cmd.resp_type = MMC_RSP_R1;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize = write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if (mmc_send_cmd(&cmd, &data)) {
		SendUartString("mmc write failed\n");
		return 0;
	}

	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(&cmd, NULL)) {
			SendUartString("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	/* Waiting for the ready status */
	if (mmc_send_status(timeout))
		return 0;

	return blkcnt;
}

ulong mmc_bwrite(lbaint_t start, lbaint_t blkcnt,
		 const void *src)
{
	lbaint_t cur, blocks_todo = blkcnt;
	unsigned char *buf = (unsigned char*)src;


	if (mmc_set_blocklen(read_bl_len))
		return 0;

	do {
		cur = (blocks_todo > CONFIG_SYS_MMC_MAX_BLK_COUNT) ?
			CONFIG_SYS_MMC_MAX_BLK_COUNT : blocks_todo;
		if (mmc_write_blocks(start, cur, buf) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		buf += cur * write_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

unsigned int mmc_write(void *buf, unsigned int offset, unsigned int size, int show_progress)
{
 	int blkstart, blkcnt;	
	unsigned int *pbuf = (unsigned int *)buf;

	if(offset % write_bl_len)
		blkstart = offset / write_bl_len + 1;
	else
		blkstart = offset / write_bl_len;

	if(size % write_bl_len)
		blkcnt = size / write_bl_len + 1;
	else
		blkcnt = size / write_bl_len;

	mmc_bwrite(blkstart,blkcnt, pbuf);

	if (show_progress)
		update_progress_set(100);

	return 0;
}

ulong mmc_bread(lbaint_t start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;
	unsigned char *buf = (unsigned char*)dst;

	if (blkcnt == 0)
		return 0;

	if (mmc_set_blocklen(read_bl_len)) {
		SendUartString("mmc_bread: Failed to set blocklen\n");
		return 0;
	}

	do {
		cur = (blocks_todo > CONFIG_SYS_MMC_MAX_BLK_COUNT) ?
			CONFIG_SYS_MMC_MAX_BLK_COUNT : blocks_todo;
		
		if (mmc_read_blocks(buf, start, cur) != cur) {
			SendUartString("mmc_bread: Failed to read blocks\n");
			return 0;
		}
		blocks_todo -= cur;
		start += cur;
		buf += cur * read_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

unsigned int mmc_read(void *buf, unsigned int offset, unsigned int size)
{
 	int blkstart, blkcnt;	
	unsigned char *pbuf = (unsigned char *)buf;
	unsigned char *tempbuf = (unsigned char *)(0x21000000);
	unsigned int inoffset = 0;
	unsigned int readsize = 0;

	blkstart = offset/read_bl_len;
	inoffset = offset - blkstart*read_bl_len;
	readsize = size + inoffset;
	if(readsize%read_bl_len)
		blkcnt = (readsize/read_bl_len) +1; 
	else
		blkcnt = readsize/read_bl_len;
	mmc_bread(blkstart,blkcnt, tempbuf);
	memcpy(pbuf,tempbuf+inoffset,size);

	return 0;
}

void EmmcClkinit(void)
{
	unsigned int regval = 0;
	regval = rSYS_SDMMC_CLK_CFG;
	regval &= ~(0x1FF << 0);
	regval |= (1 << 7) | (1 << 6) | (4 << 0);
	rSYS_SDMMC_CLK_CFG = regval;
}

void Emmcinit(int chipid)
{
	lg_ulChip = chipid;
	g_MatchSdVcc = 1;	
	EmmcClkinit();
	select_sd_pad(lg_ulChip);
	if(InitSDMMCCard())
	{
		SendUartString("InitSDMMCCard fail.\r\n");
		return;
	}
}

void bootFromEmmc(int chipid)
{
    void (*funPtr)(void);  
    UINT32 *buf = (UINT32*)IMAGE_ENTRY;
    UpFileHeader *header;
    UpFileInfo *appfile;
    UINT32 startoffset;
#ifdef MMU_ENABLE
	UINT32 appsize;
#endif	

    SysInfo *sysinfo = GetSysInfo();

	PrintVariableValueHex("image_offset", sysinfo->image_offset);
    startoffset = sysinfo->image_offset;
    mmc_read(buf, startoffset, 512);
    header = (UpFileHeader*)buf;
    appfile = &header->files[0];
    if (appfile->offset & (0x400 - 1)) {
        SendUartString("\nImage positon is not align to flash pagesize, can't load.\r\n");
        while(1);
    }

	if (sysinfo->app_size)
		appsize = sysinfo->app_size;
	else
		appsize = appfile->size;

	mmc_read(buf, appfile->offset + sysinfo->image_offset, appsize);

#ifdef MMU_ENABLE
	CP15_clean_dcache_for_dma(IMAGE_ENTRY, IMAGE_ENTRY + appsize);
#endif
	
	funPtr = (void (*)(void))IMAGE_ENTRY;
	funPtr();

}

int EmmcBurn(void *buf, unsigned int offset, unsigned int size, int show_progress)
{
 	int blkstart, blkcnt;	
	unsigned int *pbuf = (unsigned int *)buf;

	if(offset % write_bl_len)
		blkstart = offset / write_bl_len + 1;
	else
		blkstart = offset / write_bl_len;

	if(size % write_bl_len)
		blkcnt = size / write_bl_len + 1;
	else
		blkcnt = size / write_bl_len;

	//mmc_berase(blkstart, blkcnt);
	mmc_bwrite(blkstart, blkcnt, pbuf);
	
	if (show_progress)
		update_progress_set(100);
	
	return 0;
}

void EmmcWriteSysInfo(SysInfo *info)
{
	EmmcBurn(info, SYSINFOB_OFFSET, sizeof(SysInfo), 0);
	EmmcBurn(info, SYSINFOA_OFFSET, sizeof(SysInfo), 0);
}

int EmmcReadSysInfo(SysInfo *info)
{
	UINT32 *buf = (UINT32*)IMAGE_ENTRY;
	int startoffset;
	UINT32 checksum;
	UINT32 calc_checksum;

	startoffset = SYSINFOA_OFFSET;
	
	mmc_read(buf,startoffset, sizeof(SysInfo));
	checksum = *(UINT32*)(IMAGE_ENTRY + sizeof(SysInfo) - 4);
	calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, sizeof(SysInfo) - 4, 0xffffffff);
	if (calc_checksum == checksum) {
		memcpy(info, (void*)IMAGE_ENTRY, sizeof(SysInfo));
		return 0;
	}

	buf = (UINT32*)IMAGE_ENTRY;
	startoffset = SYSINFOB_OFFSET;
	
	mmc_read(buf,startoffset, sizeof(SysInfo));
	
	checksum = *(UINT32*)(IMAGE_ENTRY + sizeof(SysInfo) - 4);
	calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, sizeof(SysInfo) - 4, 0xffffffff);
	if (calc_checksum == checksum) {
		memcpy(info, (void*)IMAGE_ENTRY, sizeof(SysInfo));
		return 0;
	}
	
	return -1;
}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
unsigned int EmmcGetUpfileOffset(int type)
{
	SysInfo *sysinfo = GetSysInfo();

	if (type == UPFILE_TYPE_WHOLE) {
		if (sysinfo->image_offset == IMAGE_OFFSET)
			sysinfo->image_offset = IMAGEB_OFFSET;
		else
			sysinfo->image_offset = IMAGE_OFFSET;
		return sysinfo->image_offset;
	} else if (type == UPFILE_TYPE_FIRSTLDR) {
		if (sysinfo->loader_offset == LOADER_OFFSET)
			sysinfo->loader_offset = LOADERB_OFFSET;
		else
			sysinfo->loader_offset = LOADER_OFFSET;
		return sysinfo->loader_offset;
	} else if (type == UPFILE_TYPE_STEPLDR) {
		if (sysinfo->stepldr_offset == STEPLDRA_OFFSET)
			sysinfo->stepldr_offset = STEPLDRB_OFFSET;
		else
			sysinfo->stepldr_offset = STEPLDRA_OFFSET;
		return sysinfo->stepldr_offset;
	} else {
		unsigned int buf[512];
		UpFileHeader *header;
		UpFileInfo *appfile;
		unsigned int offset;
		int i;
		
		mmc_read(buf, sysinfo->image_offset, 512);
		header = (UpFileHeader*)buf;
		if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
			SendUartString("old update file isn't found in flash, can't support module update.\n");
			return 0xffffffff;
		}
		for (i = 0; i < header->filenum; i++) {
			appfile = &header->files[i];
			if ((appfile->magic == UPFILE_APP_MAGIC && type == UPFILE_TYPE_APP) || 
				(appfile->magic == MKTAG('R', 'O', 'M', 'A') && type == UPFILE_TYPE_RESOURCE) ||
				(appfile->magic == MKTAG('B', 'A', 'N', 'I') && type == UPFILE_TYPE_ANIMATION)) {
				offset = appfile->offset;
				if (offset & (512 - 1)) {
					SendUartString("offset isn't align to sector erase size, can't support module update.\n");
					return 0xffffffff;
				}
				return offset + sysinfo->image_offset;
			}
		}
	}

	return 0xffffffff;
}
#endif
