#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "mmc.h"
#include "sdio.h"
#include "sdmmc.h"
#include "mmcsd_core.h"

#ifdef SDMMC_SUPPORT
#define SDMMC_TICK_DELAY(t)					vTaskDelay(t)
#define SDMMC_MAX_TICK_COUNT				pdMS_TO_TICKS(10)

/* static inline uint32_t MMC_GetCardStatus(struct ark_mmc_obj *mmc_obj)
{
    uint32_t card_status = readl(mmc_obj->base + SDMMC_CDETECT);

    return card_status & 0x1;
} */

static inline uint32_t MMC_GetWaterlevel(struct ark_mmc_obj *mmc_obj)
{
    return (readl(mmc_obj->base + SDMMC_STATUS) >> 17) & 0x1fff;
}

static inline uint32_t MMC_GetStatus(struct ark_mmc_obj *mmc_obj)
{
    return readl(mmc_obj->base + SDMMC_STATUS);
}

static inline uint32_t MMC_GetRawInterrupt(struct ark_mmc_obj *mmc_obj)
{
    return readl(mmc_obj->base + SDMMC_RINTSTS);
}

static inline uint32_t MMC_GetUnmaskedInterrupt(struct ark_mmc_obj *mmc_obj)
{
    return readl(mmc_obj->base + SDMMC_MINTSTS);
}

static inline uint32_t MMC_ClearRawInterrupt(struct ark_mmc_obj *mmc_obj, uint32_t interrupts)
{
    return writel(interrupts, mmc_obj->base + SDMMC_RINTSTS);
}

static inline uint32_t MMC_GetInterruptMask(struct ark_mmc_obj *mmc_obj)
{
    return readl(mmc_obj->base + SDMMC_INTMASK);
}

static inline uint32_t MMC_SetInterruptMask(struct ark_mmc_obj *mmc_obj, uint32_t mask)
{
    return writel(mask, mmc_obj->base + SDMMC_INTMASK);
}

static inline void MMC_SetByteCount(struct ark_mmc_obj *mmc_obj, uint32_t bytes)
{
    writel(bytes, mmc_obj->base + SDMMC_BYTCNT);
}

static inline void MMC_SetBlockSize(struct ark_mmc_obj *mmc_obj, uint32_t size)
{
    writel(size, mmc_obj->base + SDMMC_BLKSIZ);
}

static inline uint32_t MMC_GetResponse(struct ark_mmc_obj *mmc_obj, int resp_num)
{
    return readl(mmc_obj->base + SDMMC_RESP0 + resp_num * 4);
}

/* static inline uint32_t MMC_IsFifoEmpty(struct ark_mmc_obj *mmc_obj)
{
    return (readl(mmc_obj->base + SDMMC_STATUS) >> 2) & 0x1;
}

static inline uint32_t MMC_IsDataStateBusy(struct ark_mmc_obj *mmc_obj)
{
    return (readl(mmc_obj->base + SDMMC_STATUS) >> 10) & 0x1;
} */

int MMC_UpdateClockRegister(struct ark_mmc_obj *mmc_obj, int div)
{
    uint32_t start_tick, tick, timeout;

    start_tick = tick = xTaskGetTickCount();
    timeout = tick + configTICK_RATE_HZ / 10; //100ms in total

    /* disable clock */
    writel(0, mmc_obj->base + SDMMC_CLKENA);
    writel(0, mmc_obj->base + SDMMC_CLKSRC);

    /* inform CIU */
    writel(1<<31 | 1<<21, mmc_obj->base + SDMMC_CMD);
    while(readl(mmc_obj->base + SDMMC_CMD) & 0x80000000)
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            printf("ERROR: %s, update clock timeout\n", __func__);
            return -1;
        }
		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }

    /* set clock to desired speed */
    writel(div, mmc_obj->base + SDMMC_CLKDIV);

    /* inform CIU */
    writel(1<<31 | 1<<21, mmc_obj->base + SDMMC_CMD);

    start_tick = xTaskGetTickCount();
    while(readl(mmc_obj->base + SDMMC_CMD) & 0x80000000)
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            printf("ERROR: %s, update clock timeout\n", __func__);
            return -1;
        }
		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }

    /* enable clock */
    writel(1, mmc_obj->base + SDMMC_CLKENA);

    /* inform CIU */
    writel(1<<31 | 1<<21, mmc_obj->base + SDMMC_CMD);

    start_tick = xTaskGetTickCount();
    while(readl(mmc_obj->base + SDMMC_CMD) & 0x80000000)
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            printf("ERROR: %s, update clock timeout\n", __func__);
            return -1;
        }
		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }

    return 0;
}

int MMC_SetCardWidth(struct ark_mmc_obj *mmc_obj, int width)
{
    switch(width)
    {
    case SDMMC_CTYPE_1BIT:
        writel(0, mmc_obj->base + SDMMC_CTYPE);
        break;
    case SDMMC_CTYPE_4BIT:
        writel(1, mmc_obj->base + SDMMC_CTYPE);
        break;
    default:
        printf("ERROR: %s, card width %d is not supported\n", __func__, width);
        return -1;
        break;
    }
    return 0;
}

int MMC_SendCommand(struct ark_mmc_obj *mmc_obj, uint32_t cmd, uint32_t arg, uint32_t flags)
{
    uint32_t tick, start_tick, timeout;

    start_tick = tick = xTaskGetTickCount();
    timeout = tick + configTICK_RATE_HZ; //1s

    writel(arg, mmc_obj->base + SDMMC_CMDARG);
    flags |= 1<<31 | 1<<29 | cmd;
    writel(flags, mmc_obj->base + SDMMC_CMD);

    while(readl(mmc_obj->base + SDMMC_CMD) & SDMMC_CMD_START)
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            printf("ERROR: %s, send cmd timeout\n", __func__);
            return -1;
        }

		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }

    //fixme: check HLE_INT_STATUS
    return 0;
}

int MMC_ResetFifo(struct ark_mmc_obj *mmc_obj)
{
    uint32_t reg, tick, start_tick, timeout;

    start_tick = tick = xTaskGetTickCount();
    timeout = tick + configTICK_RATE_HZ / 10; //100ms

    reg = readl(mmc_obj->base + SDMMC_CTRL);
    reg |= SDMMC_CTRL_FIFO_RESET;
    writel(reg, mmc_obj->base + SDMMC_CTRL);

    //wait until fifo reset finish
    while(readl(mmc_obj->base + SDMMC_CTRL) & SDMMC_CTRL_FIFO_RESET)
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            printf("ERROR: %s, FIFO reset timeout\n", __func__);
            return -1;
        }

		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }

    return 0;
}

int MMC_Reset(struct ark_mmc_obj *mmc_obj)
{
    uint32_t reg, start_tick, tick, timeout;

    reg = readl(mmc_obj->base + SDMMC_CTRL);
    reg |= SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET | SDMMC_CTRL_DMA_RESET;
    writel(reg, mmc_obj->base + SDMMC_CTRL);

    start_tick = tick = xTaskGetTickCount();
    timeout = tick + configTICK_RATE_HZ / 10; //100ms
    while(readl(mmc_obj->base + SDMMC_CTRL) & (SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET | SDMMC_CTRL_DMA_RESET))
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            printf("ERROR: %s, CTRL dma|fifo|ctrl reset timeout\n", __func__);
            return -1;
        }

		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }
    return 0;
}


#define DW_MCI_DMA_THRESHOLD	32


/* DMA interface functions */
static void dw_mci_stop_dma(struct ark_mmc_obj *mmc_obj)
{
	if (mmc_obj->using_dma) {
		mmc_obj->dma_ops->stop(mmc_obj);
		mmc_obj->dma_ops->cleanup(mmc_obj);
	}

	/* Data transfer was stopped by the interrupt handler */
	//set_bit(EVENT_XFER_COMPLETE, &host->pending_events);
}

static void dw_mci_dma_cleanup(struct ark_mmc_obj *mmc_obj)
{
	return;
}

static void dw_mci_edmac_stop_dma(struct ark_mmc_obj *mmc_obj)
{
	dma_stop_channel(mmc_obj->dms->ch);
}

static void dw_mci_dmac_complete_callback(void *param, unsigned int mask)
{
	struct ark_mmc_obj *mmc_obj = param;
	//struct mmcsd_data *data = mmc_obj->data;

	dev_vdbg(mmc_obj->dev, "DMA complete\n");

	mmc_obj->dma_ops->cleanup(mmc_obj);

	xQueueSendFromISR(mmc_obj->transfer_completion, NULL, 0);

	/*
	 * If the card was removed, data will be NULL. No point in trying to
	 * send the stop command or waiting for NBUSY in this case.
	 */
	/* if (data) {
		set_bit(EVENT_XFER_COMPLETE, &host->pending_events);
		tasklet_schedule(&host->tasklet);
	} */
}

static int dw_mci_edmac_start_dma(struct ark_mmc_obj *mmc_obj, struct mmcsd_data *data)
{
	struct dma_config cfg = {0};
	const u32 mszs[] = {1, 4, 8, 16, 32, 64, 128, 256};
	u32 fifoth_val;
	u32 dma_address;
	int ret = 0;

	/* Set external dma config: burst size, burst width */
	cfg.dst_addr_width = DMA_BUSWIDTH_4_BYTES;
	cfg.src_addr_width = DMA_BUSWIDTH_4_BYTES;

	/* Match burst msize with external dma config */
	fifoth_val = readl(mmc_obj->base + SDMMC_FIFOTH);
	cfg.dst_maxburst = mszs[(fifoth_val >> 28) & 0x7];
	cfg.src_maxburst = cfg.dst_maxburst;

	/* Read/write dst/src AHB select master */
	cfg.dst_ahbmaster_sel = 0;
	cfg.src_ahbmaster_sel = 1;

	dma_address = (u32)data->buf;
	if (dma_address & (ARCH_DMA_MINALIGN - 1)) {
		if (data->flags & DATA_DIR_WRITE) {
			mmc_obj->tx_dummy_buffer = pvPortMalloc(data->blks * data->blksize);
			if (!mmc_obj->tx_dummy_buffer)
				return -ENOMEM;
			memcpy(mmc_obj->tx_dummy_buffer, data->buf, data->blks * data->blksize);
			dma_address = (u32)mmc_obj->tx_dummy_buffer;
		} else if (data->flags & DATA_DIR_READ) {
			mmc_obj->rx_dummy_buffer = pvPortMalloc(data->blks * data->blksize);
			if (!mmc_obj->rx_dummy_buffer)
				return -ENOMEM;
			dma_address = (u32)mmc_obj->rx_dummy_buffer;
		}
		mmc_obj->dummy_buffer_used = 1;
	} else {
		mmc_obj->dummy_buffer_used = 0;
	}
	cfg.transfer_size = data->blks * data->blksize;

	if (data->flags & DATA_DIR_WRITE) {
		cfg.direction = DMA_MEM_TO_DEV;
		cfg.src_addr = dma_address;
		cfg.dst_addr = mmc_obj->base + SDMMC_FIFO;
		cfg.dst_id = SDMMC0_RTX;
	}
	else {
		cfg.direction = DMA_DEV_TO_MEM;
		cfg.src_addr = mmc_obj->base + SDMMC_FIFO;
		cfg.dst_addr = dma_address;
		cfg.src_id = SDMMC0_RTX;
	}

	ret = dma_config_channel(mmc_obj->dms->ch, &cfg);
	if (ret) {
		printf("Failed to config edmac.\n");
		return -EBUSY;
	}

	mmc_obj->data = data;

	/* Set dw_mci_dmac_complete_dma as callback */
	dma_register_complete_callback(mmc_obj->dms->ch, dw_mci_dmac_complete_callback, mmc_obj);

	/* Flush cache before write */
	if (data->flags & DATA_DIR_WRITE)
		CP15_clean_dcache_for_dma(dma_address,
				dma_address + data->blks * data->blksize);
	/* Invalidate cache before read */
	else if (data->flags & DATA_DIR_READ)
		CP15_flush_dcache_for_dma(dma_address,
				dma_address + data->blks * data->blksize);
	dma_start_channel(mmc_obj->dms->ch);

	return 0;
}

static int dw_mci_edmac_init(struct ark_mmc_obj *mmc_obj)
{
	/* Request external dma channel */
	mmc_obj->dms = pvPortMalloc(sizeof(struct dw_mci_dma_slave));
	if (!mmc_obj->dms)
		return -ENOMEM;
	memset(mmc_obj->dms, 0, sizeof(struct dw_mci_dma_slave));

	mmc_obj->dms->ch = dma_request_channel(SDMMC_DMA_CH);
	if (!mmc_obj->dms->ch) {
		printf("Failed to get external DMA channel.\n");
		vPortFree(mmc_obj->dms);
		mmc_obj->dms = NULL;
		return -ENXIO;
	}

	return 0;
}

static void dw_mci_edmac_exit(struct ark_mmc_obj *mmc_obj)
{
	if (mmc_obj->dms) {
		if (mmc_obj->dms->ch) {
			dma_release_channel(mmc_obj->dms->ch);
			mmc_obj->dms->ch = NULL;
		}
		vPortFree(mmc_obj->dms);
		mmc_obj->dms = NULL;
	}
}

static struct dw_mci_dma_ops dw_mci_edmac_ops = {
	.init = dw_mci_edmac_init,
	.exit = dw_mci_edmac_exit,
	.start = dw_mci_edmac_start_dma,
	.stop = dw_mci_edmac_stop_dma,
	.cleanup = dw_mci_dma_cleanup,
};

static void dw_mci_init_dma(struct ark_mmc_obj *mmc_obj)
{
	/*
	* Check tansfer mode from HCON[17:16]
	* Clear the ambiguous description of dw_mmc databook:
	* 2b'00: No DMA Interface -> Actually means using Internal DMA block
	* 2b'01: DesignWare DMA Interface -> Synopsys DW-DMA block
	* 2b'10: Generic DMA Interface -> non-Synopsys generic DMA block
	* 2b'11: Non DW DMA Interface -> pio only
	* Compared to DesignWare DMA Interface, Generic DMA Interface has a
	* simpler request/acknowledge handshake mechanism and both of them
	* are regarded as external dma master for dw_mmc.
	*/
	mmc_obj->use_dma = DMA_INTERFACE_NODMA;//SDMMC_GET_TRANS_MODE(readl(mmc_obj->base + SDMMC_HCON));
	if (mmc_obj->use_dma == DMA_INTERFACE_IDMA) {
		mmc_obj->use_dma = TRANS_MODE_IDMAC;
	} else if (mmc_obj->use_dma == DMA_INTERFACE_DWDMA ||
		   mmc_obj->use_dma == DMA_INTERFACE_GDMA) {
		mmc_obj->use_dma = TRANS_MODE_EDMAC;
	} else {
		goto no_dma;
	}

	/* Determine which DMA interface to use */
	if (mmc_obj->use_dma == TRANS_MODE_IDMAC) {
		/*
		* Check ADDR_CONFIG bit in HCON to find
		* IDMAC address bus width
		*/
	} else {
		/* TRANS_MODE_EDMAC: check dma bindings again */
		mmc_obj->dma_ops = &dw_mci_edmac_ops;
		dev_info(mmc_obj->dev, "Using external DMA controller.\n");
	}

	if (mmc_obj->dma_ops->init && mmc_obj->dma_ops->start &&
	    mmc_obj->dma_ops->stop && mmc_obj->dma_ops->cleanup) {
		if (mmc_obj->dma_ops->init(mmc_obj)) {
			printf("%s: Unable to initialize DMA Controller.\n",
				__func__);
			goto no_dma;
		}
	} else {
		printf("DMA initialization not found.\n");
		goto no_dma;
	}

	return;

no_dma:
	dev_info(mmc_obj->dev, "Using PIO mode.\n");
	mmc_obj->use_dma = TRANS_MODE_PIO;
}



void MMC_Init(struct ark_mmc_obj *mmc_obj)
{
    uint32_t reg;

    if(mmc_obj->mmc_reset)
        mmc_obj->mmc_reset(mmc_obj);

	if (mmc_obj->id == 0)
		vClkEnable(CLK_SDMMC0);
	/* else if (mmc_obj->id == 1)
		vClkEnable(CLK_SDMMC1); */

	MMC_Reset(mmc_obj);

	dw_mci_init_dma(mmc_obj);

    MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_ALL);
    MMC_SetInterruptMask(mmc_obj, 0x0);

    reg = readl(mmc_obj->base + SDMMC_CTRL);
    reg |= SDMMC_CTRL_INT_ENABLE;
    writel(reg, mmc_obj->base + SDMMC_CTRL);

    //set timeout param
    writel(0xffffffff, mmc_obj->base + SDMMC_TMOUT);

    //set fifo
    reg = readl(mmc_obj->base + SDMMC_FIFOTH);
    reg = ((reg >> 16) & 0xfff) + 1;
    mmc_obj->fifoth_val = SDMMC_SET_FIFOTH(0x3, reg / 2 - 1, reg / 2);
    writel(mmc_obj->fifoth_val, mmc_obj->base + SDMMC_FIFOTH);

	MMC_SetInterruptMask(mmc_obj, SDMMC_INT_CD);
}

static int ark_mmc_write_pio(struct mmc_driver *mmc_drv)
{
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    struct mmcsd_cmd *cmd = mmc_drv->cmd;
    struct mmcsd_data *data = NULL;
	uint32_t status;
	uint32_t len;
	uint32_t remain, fcnt;
	uint32_t *buf;
	int i;

    if(cmd)
        data = cmd->data;

    if(!data)
    {
        printf("ERROR: %s, data is NULL\n", __func__);
        return -EIO;
    }

	do {
		if (data->blks * data->blksize == data->bytes_xfered)
			break;

		buf = data->buf + data->bytes_xfered / 4;
		remain = data->blks * data->blksize - data->bytes_xfered;

		do {
			fcnt = (SDMMC_FIFO_DEPTH - MMC_GetWaterlevel(mmc_obj)) * 4;
			len = configMIN(remain, fcnt);
			if (!len)
				break;
			for (i = 0; i < len / 4; i ++) {
				writel(*buf++, mmc_obj->base + SDMMC_FIFO);
			}
			data->bytes_xfered += len;
			remain -= len;
		} while (remain);

		status = readl(mmc_obj->base + SDMMC_MINTSTS);
		writel(SDMMC_INT_TXDR, mmc_obj->base + SDMMC_MINTSTS);
	} while (status & SDMMC_INT_TXDR); /* if TXDR write again */

	return 0;
}

static int ark_mmc_read_pio(struct mmc_driver *mmc_drv, bool dto)
{
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    struct mmcsd_cmd *cmd = mmc_drv->cmd;
    struct mmcsd_data *data = NULL;
	u32 status;
	unsigned int len;
	unsigned int remain, fcnt;
	uint32_t *buf;
	int i;

    if(cmd)
        data = cmd->data;

    if(!data)
    {
        printf("ERROR: %s, data is NULL\n", __func__);
        return -EIO;
    }

	do {
		if (data->blks * data->blksize == data->bytes_xfered)
			break;

		buf = data->buf + data->bytes_xfered / 4;
		remain = data->blks * data->blksize - data->bytes_xfered;

		do {
			fcnt = MMC_GetWaterlevel(mmc_obj) * 4;
			len = configMIN(remain, fcnt);
			if (!len)
				break;
			for (i = 0; i < len / 4; i ++) {
				*buf++ = readl(mmc_obj->base + SDMMC_FIFO);
			}
			data->bytes_xfered += len;
			remain -= len;
		} while (remain);
		status = readl(mmc_obj->base + SDMMC_MINTSTS);
		writel(SDMMC_INT_RXDR, mmc_obj->base + SDMMC_RINTSTS);
	/* if the RXDR is ready read again */
	} while ((status & SDMMC_INT_RXDR) ||
		 (dto && MMC_GetWaterlevel(mmc_obj)));

	return 0;
}

static void ark_mmc_set_iocfg(struct mmcsd_host *host, struct mmcsd_io_cfg *io_cfg)
{
    uint32_t clksrc, clkdiv;
    struct mmc_driver *mmc_drv = host->private_data;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;

    TRACE_DEBUG("%s start\n", __func__);

    //fixme: read from PMU
    //why io_cfg->clock == 0 ?
    if(io_cfg->clock)
    {
    	if (mmc_obj->id == 0)
			clksrc = ulClkGetRate(CLK_SDMMC0);
		/* else if (mmc_obj->id == 1)
			clksrc = ulClkGetRate(CLK_SDMMC1); */
		else
			return;
		clkdiv = clksrc / io_cfg->clock / 2;
        MMC_UpdateClockRegister(mmc_obj, clkdiv);
        TRACE_DEBUG("io_cfg->clock: %lu, clock in: %lu, clkdiv: %d\n", io_cfg->clock, clkdiv, clkdiv);
    }

    if (io_cfg->bus_width == MMCSD_BUS_WIDTH_4)
    {
        MMC_SetCardWidth(mmc_obj, SDMMC_CTYPE_4BIT);
        TRACE_DEBUG("set to 4-bit mode\n");
    }
    else
    {
        MMC_SetCardWidth(mmc_obj, SDMMC_CTYPE_1BIT);
        TRACE_DEBUG("set to 1-bit mode\n");
    }


    /* maybe switch power to the card */
    switch (io_cfg->power_mode)
    {
        case MMCSD_POWER_OFF:
            break;
        case MMCSD_POWER_UP:
            break;
        case MMCSD_POWER_ON:
            break;
        default:
            printf("ERROR: %s, unknown power_mode %d\n", __func__, io_cfg->power_mode);
            break;
    }
    TRACE_DEBUG("%s end\n", __func__);
}

static void ark_mmc_enable_sdio_irq(struct mmcsd_host *host, int32_t enable)
{
    struct mmc_driver *mmc_drv = host->private_data;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    uint32_t reg;

    TRACE_DEBUG("%s start\n", __func__);

    if (enable)
    {
        MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_SDIO);
        reg = MMC_GetInterruptMask(mmc_obj);
        reg |= SDMMC_INT_SDIO;
        MMC_SetInterruptMask(mmc_obj, reg);
    }
    else
    {
        reg = MMC_GetInterruptMask(mmc_obj);
        reg &= ~SDMMC_INT_SDIO;
        MMC_SetInterruptMask(mmc_obj, reg);
    }

}

static int32_t ark_mmc_get_card_status(struct mmcsd_host *host)
{
    struct mmc_driver *mmc_drv = host->private_data;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;

    return !(readl(mmc_obj->base + SDMMC_CDETECT) & 0x1);
}

static int ark_mmc_send_command(struct mmc_driver *mmc_drv, struct mmcsd_cmd *cmd)
{
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    struct mmcsd_req *req = mmc_drv->req;
    //fixme: cmd->data or req->data
    struct mmcsd_data *data = cmd->data;
    int ret;

    uint32_t cmd_flags = 0;

    TRACE_DEBUG("%s, start\n", __func__);

    if (!cmd)
    {
        //fixme: stop dma
        printf("ERROR: %s, cmd is NULL\n", __func__);
        return -1;
    }

    if (data)
    {
        cmd_flags |= SDMMC_CMD_DAT_EXP;
        /* always set data start - also set direction flag for read */
        if (data->flags & DATA_DIR_WRITE)
            cmd_flags |= SDMMC_CMD_DAT_WR;

        if (data->flags & DATA_STREAM)
            cmd_flags |= SDMMC_CMD_STRM_MODE;
    }

    if (cmd == req->stop)
        cmd_flags |= SDMMC_CMD_STOP;
    else
        cmd_flags |= SDMMC_CMD_PRV_DAT_WAIT;

    switch (resp_type(cmd))
    {
        case RESP_NONE:
            break;
        case RESP_R1:
        case RESP_R5:
        case RESP_R6:
        case RESP_R7:
        case RESP_R1B:
            cmd_flags |= SDMMC_CMD_RESP_EXP;
            cmd_flags |= SDMMC_CMD_RESP_CRC;
            break;
        case RESP_R2:
            cmd_flags |= SDMMC_CMD_RESP_EXP;
            cmd_flags |= SDMMC_CMD_RESP_CRC;
            cmd_flags |= SDMMC_CMD_RESP_LONG;
            break;
        case RESP_R3:
        case RESP_R4:
            cmd_flags |= SDMMC_CMD_RESP_EXP;
            break;
        default:
            printf("ERROR: %s, unknown cmd type %x\n", __func__, resp_type(cmd));
            return -1;
    }

    if (cmd->cmd_code == GO_IDLE_STATE)
        cmd_flags |= SDMMC_CMD_INIT;

    /* CMD 11 check switch voltage */
    if (cmd->cmd_code == READ_DAT_UNTIL_STOP)
        cmd_flags |= SDMMC_CMD_VOLT_SWITCH;

    TRACE_DEBUG("cmd code: %d, args: 0x%x, resp type: 0x%x, flag: 0x%x\n", cmd->cmd_code, cmd->arg, resp_type(cmd), cmd_flags);
    ret = MMC_SendCommand(mmc_obj, cmd->cmd_code, cmd->arg, cmd_flags);

    if(ret)
    {
        printf("ERROR: %s, Send command timeout, cmd: %d, status: 0x%x\n", __func__, cmd->cmd_code, MMC_GetStatus(mmc_obj));
		return -1;
	}

	return 0;
}

static void dw_mci_adjust_fifoth(struct ark_mmc_obj *mmc_obj, struct mmcsd_data *data)
{
	unsigned int blksz = data->blksize;
	const u32 mszs[] = {1, 4, 8, 16, 32, 64, 128, 256};
	u32 fifo_width = 4;
	u32 blksz_depth = blksz / fifo_width, fifoth_val;
	u32 msize = 0, rx_wmark = 1, tx_wmark, tx_wmark_invers;
	int idx = ARRAY_SIZE(mszs) - 1;

	/* pio should ship this scenario */
	if (!mmc_obj->use_dma)
		return;

	tx_wmark = SDMMC_FIFO_DEPTH / 2;
	tx_wmark_invers = SDMMC_FIFO_DEPTH - tx_wmark;

	/*
	 * MSIZE is '1',
	 * if blksz is not a multiple of the FIFO width
	 */
	if (blksz % fifo_width)
		goto done;

	do {
		if (!((blksz_depth % mszs[idx]) ||
		     (tx_wmark_invers % mszs[idx]))) {
			msize = idx;
			rx_wmark = mszs[idx] - 1;
			break;
		}
	} while (--idx > 0);
	/*
	 * If idx is '0', it won't be tried
	 * Thus, initial values are uesed
	 */
done:
	fifoth_val = SDMMC_SET_FIFOTH(msize, rx_wmark, tx_wmark);
	writel(fifoth_val, mmc_obj->base + SDMMC_FIFOTH);
}

static int dw_mci_submit_data_dma(struct ark_mmc_obj *mmc_obj, struct mmcsd_data *data)
{
	u32 temp;

	mmc_obj->using_dma = 0;

	/* If we don't have a channel, we can't do DMA */
	if (!mmc_obj->use_dma)
		return -1;

	if (data->blks * data->blksize < DW_MCI_DMA_THRESHOLD ||
		data->blksize & 3 || (u32)data->buf & 3) {
		return -1;
	}

	mmc_obj->using_dma = 1;

	temp = MMC_GetInterruptMask(mmc_obj);
	temp |= SDMMC_INT_DATA_OVER | SDMMC_INT_DATA_ERROR;
	temp  &= ~(SDMMC_INT_RXDR | SDMMC_INT_TXDR);
	MMC_SetInterruptMask(mmc_obj, temp);

	/* Enable the DMA interface */
	temp = readl(mmc_obj->base + SDMMC_CTRL);
	temp |= SDMMC_CTRL_DMA_ENABLE;
	writel(temp, mmc_obj->base + SDMMC_CTRL);

	/*
	 * Decide the MSIZE and RX/TX Watermark.
	 * If current block size is same with previous size,
	 * no need to update fifoth.
	 */
	if (mmc_obj->prev_blksz != data->blksize)
		dw_mci_adjust_fifoth(mmc_obj, data);

	if (mmc_obj->dma_ops->start(mmc_obj, data)) {
		mmc_obj->dma_ops->stop(mmc_obj);
		/* We can't do DMA, try PIO for this one */
		dev_dbg(mmc_obj->dev,
			"%s: fall back to PIO mode for current transfer\n",
			__func__);
		mmc_obj->using_dma = 0;
		return -1;
	}

	return 0;
}

static int ark_mmc_prepare_data(struct mmc_driver *mmc_drv)
{
    struct mmcsd_cmd *cmd = mmc_drv->cmd;
    struct mmcsd_data *data = cmd->data;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    uint32_t data_size;
	uint32_t reg;

    if(!data)
    {
        MMC_SetBlockSize(mmc_obj, 0);
        MMC_SetByteCount(mmc_obj, 0);
        return 0;
    }

    TRACE_DEBUG("%s, start\n", __func__);

    if(MMC_ResetFifo(mmc_obj))
    {
        return -1;
    }

    data_size = data->blks * data->blksize;

    MMC_SetBlockSize(mmc_obj, data->blksize);
	data->bytes_xfered = 0;

    if(data_size % 4)
    {
        printf("ERROR: data_size should be a multiple of 4, but now is %d\n", data_size);
    }
    MMC_SetByteCount(mmc_obj, data_size);

    TRACE_DEBUG("%s, set blk size: 0x%x, byte count: 0x%x\n", __func__, data->blksize, data_size);

	if (dw_mci_submit_data_dma(mmc_obj, data)) {
		reg = readl(mmc_obj->base + SDMMC_CTRL);
		reg &= ~SDMMC_CTRL_DMA_ENABLE;
		writel(reg, mmc_obj->base + SDMMC_CTRL);
	} else {
		mmc_obj->prev_blksz = data->blksize;
	}

    TRACE_DEBUG("%s, end\n", __func__);

	return 0;
}

int ark_mmc_wait_card_idle(struct ark_mmc_obj *mmc_obj)
{
    uint32_t start_tick, tick, timeout;

    start_tick = tick = xTaskGetTickCount();
    timeout = tick + configTICK_RATE_HZ*3; //3s

    while(MMC_GetStatus(mmc_obj) & SDMMC_STATUS_BUSY)
    {
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
			if (MMC_GetStatus(mmc_obj) & SDMMC_STATUS_BUSY)
				return -1;
			else
				return 0;
        }

		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }

    return 0;
}

static int ark_mmc_get_response(struct mmc_driver *mmc_drv, struct mmcsd_cmd *cmd)
{
    int i;
    uint32_t tick, start_tick, timeout, status;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;

    cmd->resp[0] = 0;
    cmd->resp[1] = 0;
    cmd->resp[2] = 0;
    cmd->resp[3] = 0;

    start_tick = tick = xTaskGetTickCount();
    timeout = tick + configTICK_RATE_HZ / 2; //500ms
    //fixme: spin_lock_irqsave?
    do
    {
        status = MMC_GetRawInterrupt(mmc_obj);
        tick = xTaskGetTickCount();
        if(tick > timeout)
        {
            TRACE_DEBUG("ERROR: %s, get response timeout(cmd is not received by card), RINTSTS: 0x%x, cmd: %d\n", __func__, status, cmd->cmd_code);
            return -1;
        }

		if((tick - start_tick) >= SDMMC_MAX_TICK_COUNT) {
			SDMMC_TICK_DELAY(1);
			start_tick = tick;
		}
    }
    while(!(status & SDMMC_INT_CMD_DONE));

    MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_CMD_DONE);

    for (i = 0; i < 4; i++)
    {
        if (resp_type(cmd) == RESP_R2)
        {
            cmd->resp[i] = MMC_GetResponse(mmc_obj, 3 - i);
            //fixme : R2 must delay some time here ,when use UHI card, need check why
            //1ms
            //vTaskDelay(configTICK_RATE_HZ / 100);
        }
        else
        {
            cmd->resp[i] = MMC_GetResponse(mmc_obj, i);
        }
    }

    TRACE_DEBUG("resp: 0x%x, 0x%x, 0x%x, 0x%x\n", cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

    if (status & SDMMC_INT_RTO)
    {
        MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_RTO);
        TRACE_DEBUG("ERROR: %s, get response timeout, RINTSTS: 0x%x\n", __func__, status);
        return -1;
    }

    else if (status & (SDMMC_INT_RCRC | SDMMC_INT_RESP_ERR))
    {
        MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_RCRC | SDMMC_INT_RESP_ERR);
        printf("ERROR: %s, response error or response crc error, RINTSTS: 0x%x\n", __func__, status);
        //return -1;
    }

    return 0;
}

static int ark_mmc_start_transfer(struct mmc_driver *mmc_drv)
{
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    struct mmcsd_cmd *cmd = mmc_drv->cmd;
    struct mmcsd_data *data = NULL;
    int ret;
    uint32_t interrupt, status, reg;
	uint32_t timeout;

    if(cmd)
        data = cmd->data;

    if(!data)
    {
        return 0;
    }

    TRACE_DEBUG("%s, start\n", __func__);

    //fixme: spin_lock_irqsave(&host->lock, flags);
	if (!mmc_obj->using_dma) {
		//fifo mode open data interrupts
	    reg = MMC_GetInterruptMask(mmc_obj);
	    reg |= SDMMC_INT_STATUS_DATA;
	    MMC_SetInterruptMask(mmc_obj, reg);
	}

    //fixme: spin_unlock_irqrestore(&host->lock, flags);
    timeout = configTICK_RATE_HZ + pdMS_TO_TICKS(data->blks * data->blksize / 1024);
    ret = xQueueReceive(mmc_obj->transfer_completion, NULL, timeout);

	if (mmc_obj->using_dma) {
		if (mmc_obj->dummy_buffer_used) {
			if (data->flags & DATA_DIR_WRITE) {
				if (mmc_obj->tx_dummy_buffer) {
					vPortFree(mmc_obj->tx_dummy_buffer);
					mmc_obj->tx_dummy_buffer = NULL;
				}
			} else if (data->flags & DATA_DIR_READ) {
				if (mmc_obj->rx_dummy_buffer) {
					memcpy(data->buf, mmc_obj->rx_dummy_buffer, data->blks * data->blksize);
					vPortFree(mmc_obj->rx_dummy_buffer);
					mmc_obj->rx_dummy_buffer = NULL;
				}
			}
		}
	} else {
	    reg = MMC_GetInterruptMask(mmc_obj);
	    reg &= ~SDMMC_INT_STATUS_DATA;
	    MMC_SetInterruptMask(mmc_obj, reg);
	}

    if(ret != pdTRUE || mmc_obj->result)
    {
        //fixme: error handle
        if (mmc_obj->using_dma)
			dw_mci_stop_dma(mmc_obj);
        cmd->err = ret;
        interrupt = MMC_GetRawInterrupt(mmc_obj);
        status = MMC_GetStatus(mmc_obj);
        printf("ERROR: %s, transfer timeout, ret: %d, RINTSTS: 0x%x, STATUS: 0x%x\n", __func__, ret, interrupt, status);
        return -1;
    }

    data->bytes_xfered = data->blks * data->blksize;

    return 0;
}

static void ark_mmc_complete_request(struct mmc_driver *mmc_drv)
{
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;

    mmc_drv->cmd = NULL;
    mmc_drv->req = NULL;
    mmc_drv->data = NULL;


    MMC_SetBlockSize(mmc_obj, 0);
    MMC_SetByteCount(mmc_obj, 0);

    mmcsd_req_complete(mmc_drv->host);
}

static void ark_mmc_request(struct mmcsd_host *host, struct mmcsd_req *req)
{
    int ret;
    struct mmc_driver *mmc_drv = host->private_data;
    struct mmcsd_cmd *cmd = req->cmd;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    unsigned int mmc_task_prio;

    TRACE_DEBUG("%s start\n", __func__);
	mmc_task_prio = uxTaskPriorityGet(NULL);
	if(mmc_task_prio < configMAX_PRIORITIES - 2) {
		vTaskPrioritySet(NULL, configMAX_PRIORITIES - 2);
	}
    mmc_drv->req = req;
    mmc_drv->cmd = cmd;

    if (mmc_drv->host->transfer_err > 5) {
		goto out;
    }

	if (mmc_obj->transfer_completion == NULL)
		mmc_obj->transfer_completion = xQueueCreate(1, 0);
	else
		xQueueReset(mmc_obj->transfer_completion);

    ret = ark_mmc_wait_card_idle(mmc_obj);

    if (ret)
    {
        printf("ERROR: %s, data transfer timeout, status: 0x%x\r\n", __func__, MMC_GetStatus(mmc_obj));
        if (MMC_GetStatus(mmc_obj) & SDMMC_STATUS_BUSY)
            goto out;
    }

	mmc_obj->result = 0;
    if (ark_mmc_prepare_data(mmc_drv))
		goto out;
    if (ark_mmc_send_command(mmc_drv, cmd))
		goto out;
    ret = ark_mmc_get_response(mmc_drv, cmd);
    if(ret)
    {
        cmd->err = ret;
        mmc_drv->host->transfer_err++;
        printf("%s,get response returns %d, cmd: %d err count: %d\r\n", __func__, ret, cmd->cmd_code, mmc_drv->host->transfer_err);
        goto out;
    }
    ark_mmc_start_transfer(mmc_drv);

    if(req->stop)
    {
        /* send stop command */
        TRACE_DEBUG("%s send stop\n", __func__);
        ark_mmc_send_command(mmc_drv, req->stop);
    }
    mmc_drv->host->transfer_err = 0;

out:
    ark_mmc_complete_request(mmc_drv);
	if(mmc_task_prio < configMAX_PRIORITIES - 2) {
		vTaskPrioritySet(NULL, mmc_task_prio);
	}
    TRACE_DEBUG("%s end\n", __func__);
}

static const struct mmcsd_host_ops ark_mmc_ops =
{
    .request            = ark_mmc_request,
    .set_iocfg          = ark_mmc_set_iocfg,
    .enable_sdio_irq    = ark_mmc_enable_sdio_irq,
    .get_card_status    = ark_mmc_get_card_status,
};

static void ark_mmc_interrupt(void *param)
{
    struct mmc_driver *mmc_drv = (struct mmc_driver *)param;
    struct ark_mmc_obj *mmc_obj = (struct ark_mmc_obj *)mmc_drv->priv;
    struct mmcsd_cmd *cmd = mmc_drv->cmd;
    struct mmcsd_data *data = NULL;
    uint32_t status;

    if (cmd && cmd->data)
    {
        data = cmd->data;
    }

    status = MMC_GetUnmaskedInterrupt(mmc_obj);
    TRACE_DEBUG("unmasked interrupts: 0x%x\n", status);

	if (status & SDMMC_CMD_ERROR_FLAGS) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_CMD_ERROR_FLAGS);

		mmc_obj->result = -1;
		xQueueSendFromISR(mmc_obj->transfer_completion, NULL, 0);
	}

	if (status & SDMMC_DATA_ERROR_FLAGS) {
		/* if there is an error report DATA_ERROR */
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_DATA_ERROR_FLAGS);
		mmc_obj->result = -1;
		xQueueSendFromISR(mmc_obj->transfer_completion, NULL, 0);
	}

	if (status & SDMMC_INT_DATA_OVER) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_DATA_OVER);
		if (data && data->flags & DATA_DIR_READ) {
			if (!mmc_obj->using_dma && data->bytes_xfered != data->blks * data->blksize)
				ark_mmc_read_pio(mmc_drv, 1);
		}
		if (!mmc_obj->using_dma)
			xQueueSendFromISR(mmc_obj->transfer_completion, NULL, 0);
	}

	if (status & SDMMC_INT_RXDR) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_RXDR);
		if (data && data->flags & DATA_DIR_READ)
			ark_mmc_read_pio(mmc_drv, 0);
	}

	if (status & SDMMC_INT_TXDR) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_TXDR);
		if (data && data->flags & DATA_DIR_WRITE)
			ark_mmc_write_pio(mmc_drv);
	}

	if (status & SDMMC_INT_CMD_DONE) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_CMD_DONE);
	}

	if (status & SDMMC_INT_CD) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_CD);
#ifndef WIFI_SUPPORT
		mmcsd_change_from_isr(mmc_drv->host);
#endif
	}

	if (status & SDMMC_INT_SDIO) {
		MMC_ClearRawInterrupt(mmc_obj, SDMMC_INT_SDIO);
		sdio_irq_wakeup_isr(mmc_drv->host);
	}
}

#define SYS_SOFT_RSTA	0x5c
void ark_mmc_reset(struct ark_mmc_obj *mmc_obj)
{
	unsigned int val;
	unsigned int offset;

	if (mmc_obj->id == 0)
		offset = 4;
	/* else if (mmc_obj->id == 1)
		offset = 31; */
	else
		return;

	val = readl(REGS_SYSCTL_BASE + SYS_SOFT_RSTA);
	writel(val & ~(1 << offset), REGS_SYSCTL_BASE + SYS_SOFT_RSTA);
	udelay(10);
	writel(val | (1 << offset), REGS_SYSCTL_BASE + SYS_SOFT_RSTA);
}

static struct ark_mmc_obj mmc0_obj =
{
    .id = 0,
    .irq = SDMMC0_IRQn,
    .base = REGS_SDMMC0_BASE,
    .mmc_reset = ark_mmc_reset,
    .dma_args = {SDMMC0_RTX, 1, 0},
};

int ark_mmc_probe(struct ark_mmc_obj *mmc_obj)
{
    struct mmc_driver *mmc_drv;
    struct mmcsd_host *host;

    TRACE_DEBUG("%s start\n", __func__);

    mmc_drv = (struct mmc_driver*)pvPortMalloc(sizeof(struct mmc_driver));
    memset(mmc_drv, 0, sizeof(struct mmc_driver));
    mmc_drv->priv = mmc_obj;

    host = mmcsd_alloc_host();
    if (!host)
    {
        printf("ERROR: %s, failed to malloc host\n", __func__);
        return -ENOMEM;
    }

    host->ops = &ark_mmc_ops;
    host->freq_min = MMC_FEQ_MIN;
    host->freq_max = MMC_FEQ_MAX;
    host->valid_ocr = VDD_32_33 | VDD_33_34;

    host->flags = MMCSD_MUTBLKWRITE | MMCSD_SUP_HIGHSPEED | MMCSD_BUSWIDTH_4;
    host->max_blk_size = 512;
    //fixme: max_blk_count?
    host->max_blk_count = 2048;
    host->private_data = mmc_drv;

    mmc_drv->host = host;

    MMC_Init(mmc_obj);

    if(mmc_obj->id == 0)
	{
		request_irq(mmc_obj->irq, 0, ark_mmc_interrupt, mmc_drv);
    }
    /* else if(mmc_obj->id == 1)
	{
		request_irq(mmc_obj->irq, 0, ark_mmc_interrupt, mmc_drv);
    } */
#ifndef WIFI_SUPPORT
	if (mmc_obj->id == 0)
	{
	#if DEVICE_TYPE_SELECT != EMMC_FLASH
		if (ark_mmc_get_card_status(host))
	#endif
			mmcsd_change(host);
	}
	else if (mmc_obj->id == 1)
#endif
	{
		ark_mmc_enable_sdio_irq(host, 1);
		mmcsd_change(host);
	}

    TRACE_DEBUG("%s end\n", __func__);

    return 0;
}

int mmc_init(void)
{
	ark_mmc_probe(&mmc0_obj);

	return 0;
}

#endif
