#include "FreeRTOS.h"
#include "chip.h"

#define DMA_CH_NUM	4
#define DMA_BLOCK_SIZE	0xfff

#define	rDMACIntStatus				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x000))
#define	rDMACIntTCStatus			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x004))
#define	rDMACIntTCClear				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x008))
#define	rDMACIntErrorStatus			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x00C))
#define	rDMACIntErrClr				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x010))
#define	rDMACRawIntTCStatus			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x014))
#define	rDMACRawIntErrorStatus		*((volatile unsigned int *)(REGS_DMAC_BASE + 0x018))
#define	rDMACEnbldChns				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x01C))
#define	rDMACSoftBReq				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x020))
#define	rDMACSoftSReq				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x024))
#define	rDMACSoftLBReq				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x028))
#define	rDMACSoftLSReq				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x02C))
#define	rDMACConfiguration			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x030))
#define	rDMACSync					*((volatile unsigned int *)(REGS_DMAC_BASE + 0x034))
#define	rDMACCxSrcAddr(x)			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x100 + 0x00 + (x)*0x20))
#define	rDMACCxDestAddr(x)			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x100 + 0x04 + (x)*0x20))
#define	rDMACCxLLI(x)				*((volatile unsigned int *)(REGS_DMAC_BASE + 0x100 + 0x08 + (x)*0x20))
#define	rDMACCxControl(x)			*((volatile unsigned int *)(REGS_DMAC_BASE + 0x100 + 0x0C + (x)*0x20))
#define	rDMACCxConfiguration(x)		*((volatile unsigned int *)(REGS_DMAC_BASE + 0x100 + 0x10 + (x)*0x20))


static struct dma_chan dma_ch[DMA_CH_NUM] = {0};
static SemaphoreHandle_t dma_mutex;
static QueueHandle_t dma_m2m_done = NULL;


struct dma_chan *dma_request_channel(int favorite_ch)
{
	int i;

	configASSERT (favorite_ch >= 0 && favorite_ch < DMA_CH_NUM)

	xSemaphoreTake(dma_mutex, portMAX_DELAY);

	if (!dma_ch[favorite_ch].in_use) {
		dma_ch[favorite_ch].chan_id = favorite_ch;
		dma_ch[favorite_ch].in_use = 1;
		xSemaphoreGive(dma_mutex);
		return &dma_ch[favorite_ch];
	}

	for (i = 0; i < DMA_CH_NUM; i++) {
		if (!dma_ch[i].in_use) {
			dma_ch[i].chan_id = i;
			dma_ch[i].in_use = 1;
			xSemaphoreGive(dma_mutex);
			return &dma_ch[i];
		}
	}

	xSemaphoreGive(dma_mutex);

	return NULL;
}


void dma_release_channel(struct dma_chan *chan)
{
	/* This channel is not in use, bail out */
	if (!chan->in_use)
		return;

	dma_stop_channel(chan);

	xSemaphoreTake(dma_mutex, portMAX_DELAY);

	/* This channel is not in use anymore, free it */
	chan->irq_callback = NULL;
	chan->callback_param = NULL;
	chan->in_use = 0;

	xSemaphoreGive(dma_mutex);
}

/*
 * Fix sconfig's burst size according to dw_dmac. We need to convert them as:
 * 1 -> 0, 4 -> 1, 8 -> 2, 16 -> 3.
 *
 * NOTE: burst size 2 is not supported by controller.
 *
 * This can be done by finding least significant bit set: n & (n - 1)
 */
static void convert_burst(u32 *maxburst)
{
	if (*maxburst > 1)
		*maxburst = fls(*maxburst) - 2;
	else
		*maxburst = 0;
}

int dma_config_channel(struct dma_chan *chan, struct dma_config *config)
{
	unsigned int ctl;
	unsigned int cfg;
	unsigned int src_width, dst_width;
	unsigned int src_id = 0, dst_id = 0;
	unsigned int di = 0, si = 0;
	unsigned int data_width = (1 << DMA_BUSWIDTH_4_BYTES);

	convert_burst(&config->src_maxburst);
	convert_burst(&config->dst_maxburst);

	if (config->direction == DMA_MEM_TO_DEV) {
		src_width = __ffs(data_width | config->src_addr | config->transfer_size);
		dst_width = config->dst_addr_width;
		dst_id = config->dst_id;
		si = 1;
	} else if (config->direction == DMA_DEV_TO_MEM) {
		src_width = config->src_addr_width;
		dst_width = __ffs(data_width | config->dst_addr | config->transfer_size);
		src_id = config->src_id;
		di = 1;
	} else if (config->direction == DMA_MEM_TO_MEM) {
		src_width = __ffs(data_width | config->src_addr | config->transfer_size);
		dst_width = __ffs(data_width | config->dst_addr | config->transfer_size);
		si = 1;
		di = 1;
	}

	ctl = (1 << 31) |   /* [31] I Read/write Terminal count interrupt enable bit */
		  (0 << 28) | /* [30:28] Prot Read/write Protection */
		  (di << 27) |  /* [27] DI Read/write Destination increment */
		  (si << 26) |  /* [26] SI Read/write Source increment */
		  (config->dst_ahbmaster_sel << 25) |   /* [25] D Read/write Destination AHB master select */
		  (config->src_ahbmaster_sel << 24) |   /* [24] S Read/write Source AHB master select */
		  (dst_width << 21) | /* [23:21] DWidth Read/write Destination transfer width */
		  (src_width << 18) | /* [20:18] SWidth Read/write Source transfer width */
		  (config->dst_maxburst << 15) | /* [17:15] DBSize Read/write Destination burst size */
		  (config->src_maxburst << 12) | /* [14:12] SBSize Read/write Source burst size */
		  0; /*	[11:0] TransferSize Read/write Transfer size */

	cfg = (0 << 18) | /* [18] H Read/write Halt */
		  (0 << 16) | /* [16] L Read/write Lock */
		  (1 << 15) | /* [15] ITC Read/write Terminal count interrupt mask */
		  (1 << 14) | /* [14] IE Read/write Interrupt error mask */
		  (config->direction << 11) | /* [13:11] FlowCntrl Read/write Flow control and transfer type */
		  (dst_id << 6) | /* [9:6] DestPeripheral Read/write Destination peripheral */
		  (src_id << 1) | /* [4:1] SrcPeripheral Read/write Source peripheral */
		  0; /* [0] Channel enable */

	if ((config->transfer_size >> src_width) > DMA_BLOCK_SIZE) {
		unsigned int blk_size = config->transfer_size >> src_width;
		int lli_num;
		int i;

		lli_num = (blk_size + DMA_BLOCK_SIZE - 1) / DMA_BLOCK_SIZE - 1;
		if (chan->lli) {
			vPortFree(chan->lli);
			chan->lli = NULL;
		}
		chan->lli = pvPortMalloc(sizeof(struct dma_lli) * lli_num);
		if (!chan->lli)
			return -ENOMEM;
		for (i = 0; i < lli_num - 1; i++) {
			chan->lli[i].src_addr = config->src_addr + (si ? (i + 1) : 0) * (DMA_BLOCK_SIZE << src_width);
			chan->lli[i].dst_addr = config->dst_addr + (di ? (i + 1) : 0) * (DMA_BLOCK_SIZE << src_width);
			chan->lli[i].next_lli = (unsigned int)&chan->lli[i + 1];
			chan->lli[i].control = ctl | DMA_BLOCK_SIZE;
			if (!config->blkint_en)
				chan->lli[i].control &= ~(1 << 31);
		}
		chan->lli[i].src_addr = config->src_addr + (si ? (i + 1) : 0) * (DMA_BLOCK_SIZE << src_width);
		chan->lli[i].dst_addr = config->dst_addr + (di ? (i + 1) : 0) * (DMA_BLOCK_SIZE << src_width);
		chan->lli[i].next_lli = 0;
		chan->lli[i].control = ctl | (blk_size - DMA_BLOCK_SIZE * lli_num);
		CP15_clean_dcache_for_dma((unsigned int)chan->lli,
			(unsigned int)chan->lli + sizeof(struct dma_lli) * lli_num);

		rDMACCxSrcAddr(chan->chan_id) = config->src_addr;
		rDMACCxDestAddr(chan->chan_id) = config->dst_addr;
		rDMACCxLLI(chan->chan_id) = (unsigned int)chan->lli | 1;
		rDMACCxControl(chan->chan_id) = ctl & ~(1 << 31) | DMA_BLOCK_SIZE;
		rDMACCxConfiguration(chan->chan_id)	= cfg;
	} else {
		rDMACCxSrcAddr(chan->chan_id) = config->src_addr;
		rDMACCxDestAddr(chan->chan_id) = config->dst_addr;
		rDMACCxLLI(chan->chan_id) = 0;
		rDMACCxControl(chan->chan_id) = ctl | (config->transfer_size >> src_width);
		rDMACCxConfiguration(chan->chan_id)	= cfg;
	}

	return 0;
}

int dma_register_complete_callback(struct dma_chan *chan,
											void (*callback)(void *param, unsigned int mask),
											void *callback_param)
{
	chan->irq_callback = callback;
	chan->callback_param = callback_param;

	return 0;
}

int dma_start_channel(struct dma_chan *chan)
{
	configASSERT(chan && chan->chan_id < DMA_CH_NUM);

	rDMACCxConfiguration(chan->chan_id) |= (1 << 0);

	return 0;
}

int dma_stop_channel(struct dma_chan *chan)
{
	unsigned int timeout = xTaskGetTickCount() + 1000;

	configASSERT(chan && chan->chan_id < DMA_CH_NUM);

	xSemaphoreTake(dma_mutex, portMAX_DELAY);

	if(!(rDMACEnbldChns & (1 << chan->chan_id))) {
		xSemaphoreGive(dma_mutex);
		return 0;
	}

	// A channel can be disabled by clearing the Enable bit.
	rDMACCxConfiguration(chan->chan_id) &= ~1;

	// waiting
	while(rDMACEnbldChns & (1 << chan->chan_id)) {
		if(xTaskGetTickCount() >= timeout) {
			printf ("dma_stop_channel %d timeout\n", chan->chan_id);
			xSemaphoreGive(dma_mutex);
			return -1;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	if (chan->lli) {
		vPortFree(chan->lli);
		chan->lli = NULL;
	}

	xSemaphoreGive(dma_mutex);
	return 0;
}

static void dma_m2m_callback(void *param, unsigned int mask)
{
	if(dma_m2m_done)
		xQueueSendFromISR(dma_m2m_done, NULL, 0);
}

int dma_m2mcpy(unsigned int dst_addr, unsigned int src_addr, int size)
{
	struct dma_config cfg = {0};
	int ret = -1;

	struct dma_chan *dma_ch = dma_request_channel(0);
	if (!dma_ch) {
		printf("%s() dma_request_channel fail.\n",  __func__);
		return -1;
	}

	cfg.dst_addr_width = DMA_BUSWIDTH_4_BYTES;
	cfg.dst_maxburst = 256;
	cfg.src_addr_width = DMA_BUSWIDTH_4_BYTES;
	cfg.src_maxburst = 256;
	cfg.transfer_size = size;
	cfg.src_addr = src_addr;
	cfg.dst_addr = dst_addr;
	cfg.direction = DMA_MEM_TO_MEM;
	cfg.dst_ahbmaster_sel = 0;
	cfg.src_ahbmaster_sel = 1;

	dma_clean_range(src_addr, src_addr + size);
	dma_inv_range(dst_addr, dst_addr + size);

	ret = dma_config_channel(dma_ch, &cfg);
	if (ret) {
		printf("%s, dma_config_channel failed.\n", __func__);
		goto exit;
	}

	dma_register_complete_callback(dma_ch, dma_m2m_callback, NULL);

	xQueueReset(dma_m2m_done);

	dma_start_channel(dma_ch);
	if (xQueueReceive(dma_m2m_done, NULL, pdMS_TO_TICKS(1000)) != pdTRUE) {
		printf("dma_m2mcpy wait timeout.\n");
		ret = -ETIMEDOUT;
		goto exit;
	}

	dma_stop_channel(dma_ch);
	ret = 0;
exit:
	if(dma_ch)
		dma_release_channel(dma_ch);
	return ret;
}


static void dma_int_handler(void *param)
{
	unsigned int err_status, tfr_status;
	struct dma_chan *chan;
	unsigned int irqmask = 0;
	int i;

	err_status = rDMACIntErrorStatus;
	tfr_status = rDMACIntTCStatus;

	rDMACIntTCClear = tfr_status;
	rDMACIntErrClr = err_status;
	
	for(i= 0; i< DMA_CH_NUM; i++) {
		irqmask = 0;

		if (err_status & (1 << i)) {
			irqmask |= DMA_INT_ERR;
		}

		if (tfr_status & (1 << i)) {
			irqmask |= DMA_INT_TC;
		}

		if (!irqmask)
			continue;

		chan = &dma_ch[i];
		if (chan->irq_callback)
			chan->irq_callback(chan->callback_param, irqmask);
	}
}

int dma_init(void)
{
	dma_mutex = xSemaphoreCreateMutex();
	dma_m2m_done = xQueueCreate(1, 0);

	sys_soft_reset(softreset_dma);

	request_irq(DMA_IRQn, 0, dma_int_handler, NULL);

	/* Clear all interrupts on all channels. */
	rDMACIntTCClear = 0xff;
	rDMACIntErrClr = 0xff;
	
	rDMACConfiguration |= (1<<0);	// [0] E Read/write PrimeCell DMAC enable

	return 0;
}
