#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "audio/audio.h"
#include "i2s.h"
#include "soc-dai.h"


#define I2S_SLAVE			0
#define I2S_MASTER			1


typedef struct ark_i2s_private_data {
	struct snd_soc_dai_ops adc_ops;
	struct snd_soc_dai_ops dac_ops;
} ark_i2s_pd;


extern int audio_codec_adc_init(struct snd_soc_dai_ops *ops);
extern int audio_codec_dac_init(struct snd_soc_dai_ops *ops);

#if 0
static void ark_i2s_start_play(struct ark_i2s_data *i2s)
{
	u32 val = readl(i2s->base + I2S_SACR1);
	val &= ~SACR1_DRPL;
  	writel(val, i2s->base + I2S_SACR1);
}

static void ark_i2s_start_record(struct ark_i2s_data *i2s)
{
	u32 val = readl(i2s->base + I2S_SACR1);
	val &= ~SACR1_DREC;
	writel(val, i2s->base + I2S_SACR1);
}
#endif

static void ark_i2s_stop_play(struct ark_i2s_data *i2s)
{
	u32 val = readl(i2s->base + I2S_SACR1);
	val |= SACR1_DRPL;
	writel(val, i2s->base + I2S_SACR1);
}

static void ark_i2s_stop_record(struct ark_i2s_data *i2s)
{
	u32 val = readl(i2s->base + I2S_SACR1);
	val |= SACR1_DREC;
	writel(val, i2s->base + I2S_SACR1);
}

static void ark_i2s_restart(struct ark_i2s_data *i2s, int stream)
{
	u32 val = readl(i2s->base + I2S_SACR1);

	/* record and replay must be started together in full duplex mode,
	 * or dma transfer will timeout.
	 */
	if (i2s->full_duplex) {
		/* close record and replay */
		writel(val | SACR1_DREC | SACR1_DRPL, i2s->base + I2S_SACR1);
		mdelay(1);
	}
	if(stream == AUDIO_STREAM_REPLAY)
		val &= ~SACR1_DRPL;
	else
		val &= ~SACR1_DREC;
	writel(val, i2s->base + I2S_SACR1);
}

int ark_i2s_startup(struct ark_i2s_data *i2s, int stream)
{
	unsigned int val;

	xSemaphoreTake(i2s->mutex, portMAX_DELAY);
	if (stream == AUDIO_STREAM_REPLAY)
	{
		/* close play */
		ark_i2s_stop_play(i2s);
		val = readl(i2s->base + I2S_SACR0);
		val |= SACR0_CHANLOCK | SACR0_RFTH(0x10) | SACR0_TFTH(0xF) | SACR0_ENB;
		if(i2s->cfg[stream].lfirst)
			val |= SACR0_TFIFOFIRSTBIT;
		else
			val &= ~SACR0_TFIFOFIRSTBIT;
		if(i2s->cfg[stream].channels == 1)
			val |= SACR0_SCBIT;	/* single channel */
		else
			val &= ~SACR0_SCBIT;
		if(i2s->cfg[stream].bits > 16)
			val |= SACR0_BITS; /* 32 Bits */
		else
			val &= ~SACR0_BITS;
		if(i2s->cfg[stream].master)
			val |= SACR0_BCKD | SACR0_SYNCD;
		else
			val &= ~(SACR0_BCKD | SACR0_SYNCD);
		if(i2s->dma_txch)
			val |= SACR0_TDMAEN;
		else
			val &= ~SACR0_TDMAEN;
		writel(val, i2s->base + I2S_SACR0);

		/* interrupt clear */
		val = readl(i2s->base + I2S_SAICR);
		val |= (SAICR_TFS | SAICR_TUR);
		writel(val, i2s->base + I2S_SAICR);
		val &= ~(SAICR_TFS | SAICR_TUR);
		writel(val, i2s->base + I2S_SAICR);

		/* interrupt enable */
		val = readl(i2s->base + I2S_SAIMR);
		val |= (SAIMR_TFS | SAIMR_TUR);
		writel(val, i2s->base + I2S_SAIMR);

		/* start replay */
		ark_i2s_restart(i2s, stream);
	}
	else if(stream == AUDIO_STREAM_RECORD)
	{
		ark_i2s_pd *pdata = i2s->extdata;
		/* close record */
		ark_i2s_stop_record(i2s);

		if(pdata && pdata->adc_ops.hw_params)
		{
			struct snd_soc_hw_params params;
			params.rates = i2s->cfg[stream].rates;
			params.channels = i2s->cfg[stream].channels;
			params.bits = i2s->cfg[stream].bits;
			pdata->adc_ops.hw_params(&params);
		}

		val = readl(i2s->base + I2S_SACR0);
		val |= SACR0_CHANLOCK| SACR0_RFTH(0x10) | SACR0_TFTH(0xF) | SACR0_ENB;
		if(i2s->cfg[stream].lfirst)
			val |= SACR0_RFIFIFIRSTBIT;
		else
			val &= ~SACR0_RFIFIFIRSTBIT;
		if(i2s->cfg[stream].channels == 1)
			val |= SACR0_SCBIT;
		else
			val &= ~SACR0_SCBIT;
		if(i2s->cfg[stream].bits > 16)
			val |= SACR0_BITS;	 //32 Bits.
		else
			val &= ~SACR0_BITS;
		if(i2s->dma_rxch)
			val |= SACR0_RDMAEN;
		else
			val &= ~SACR0_RDMAEN;
		if(i2s->cfg[stream].master)
		{
			val |= SACR0_BCKD | SACR0_SYNCD;
			writel(val, i2s->base + I2S_SACR0);
		}
		else
		{
			writel(val, i2s->base + I2S_SACR0);
#if 0
			/* 630hv100 record work in slave mode, bclk need inverse */
			if(i2s->id == 0) {
				val = readl(REGS_SYSCTL_BASE + SYS_PER_CLK_CFG);
				val |= (1<<18);
				writel(val, REGS_SYSCTL_BASE + SYS_PER_CLK_CFG);
			} else if(i2s->id == 1) {
				val = readl(REGS_SYSCTL_BASE + SYS_BUS_CLK1_CFG);
				val |= (1<<6);	//1:bclk inverse; 0:normal.
				writel(val, REGS_SYSCTL_BASE + SYS_BUS_CLK1_CFG);
			}
#endif
		}

		/* interrupt clear */
		val = readl(i2s->base + I2S_SAICR);
		val |= (SAICR_ROR | SAICR_RFS);
		writel(val, i2s->base + I2S_SAICR);
		val &= ~(SAICR_ROR | SAICR_RFS);
		writel(val, i2s->base + I2S_SAICR);

		/* interrupt enable */
		val = readl(i2s->base + I2S_SAIMR);
		val |= (SAIMR_ROR | SAIMR_RFS);
		writel(val, i2s->base + I2S_SAIMR);

		if(pdata && pdata->adc_ops.startup)
		{
			pdata->adc_ops.startup(1);
		}

		/* start record */
		ark_i2s_restart(i2s, stream);
	}
	xSemaphoreGive(i2s->mutex);

	return 0;
}

int ark_i2s_set_volume(struct ark_i2s_data *i2s, int lvol, int rvol)
{
	ark_i2s_pd *pdata = i2s->extdata;
	if(pdata && pdata->dac_ops.set_volume)
	{
		pdata->dac_ops.set_volume(lvol);  //lvol == rvol
	}

	return 0;
}

int ark_i2s_set_rate(struct ark_i2s_data *i2s, int stream, unsigned int rate)
{
	u32 step = 256 * 2, modulo;
	u32 val, freq;

	if (!i2s->nco_reg)
		return 0;

	xSemaphoreTake(i2s->mutex, portMAX_DELAY);
	if(stream == AUDIO_STREAM_REPLAY)
	{
		if(i2s->cfg[stream].rates != rate)
			i2s->cfg[stream].rates = rate;
	}
	else if(stream == AUDIO_STREAM_RECORD)
	{
		if(i2s->cfg[stream].rates != rate)
			i2s->cfg[stream].rates = rate;
	}
	else
	{
		printf("%s, Invalid i2s stream:%d.\n", __func__, stream);
		return -EINVAL;
	}

	/* mclk = rate * 256, mclk = freq * step / (2 * modulo) */
	freq = ulClkGetRate(i2s->clkid);
	modulo = freq / rate;
	val = (step << 16) | modulo;
	writel(val, i2s->nco_reg);

#if 0
	writel(val, 0x6000006C);	//set i2s0 rate.
#endif
	xSemaphoreGive(i2s->mutex);

	return 0;
}

int ark_i2s_set_params(struct ark_i2s_data *i2s, int stream, int rates, int channels, int bits)
{
	int ret = 0;

	xSemaphoreTake(i2s->mutex, portMAX_DELAY);
	if(stream == AUDIO_STREAM_REPLAY)
	{
		i2s->cfg[stream].rates = rates;
		i2s->cfg[stream].channels = channels;
		i2s->cfg[stream].bits = bits;
	}
	else if(stream == AUDIO_STREAM_RECORD)
	{
		i2s->cfg[stream].rates = rates;
		i2s->cfg[stream].channels = channels;
		i2s->cfg[stream].bits = bits;
	}
	else
	{
		printf("%s, Invalid i2s stream:%d.\n", __func__, stream);
		ret = -EINVAL;
	}
	xSemaphoreGive(i2s->mutex);

	ark_i2s_set_rate(i2s, stream, rates);

	return ret;
}

void ark_i2s_stop(struct ark_i2s_data *i2s, int stream)
{
	ark_i2s_pd *pdata = i2s->extdata;
	u32 val;

	xSemaphoreTake(i2s->mutex, portMAX_DELAY);
	if(stream == AUDIO_STREAM_REPLAY)
	{
		ark_i2s_stop_play(i2s);

		if(i2s->dma_txch)
		{
			val = readl(i2s->base + I2S_SACR0);
			val &= ~SACR0_TDMAEN;
			writel(val, i2s->base + I2S_SACR0);
		}

		/* interrupt disable */
		val = readl(i2s->base + I2S_SAIMR);
		val &= ~(SAIMR_TFS | SAIMR_TUR);
		writel(val, i2s->base + I2S_SAIMR);
	}
	else if(stream == AUDIO_STREAM_RECORD)
	{
		ark_i2s_stop_record(i2s);

		if(i2s->dma_rxch)
		{
			val = readl(i2s->base + I2S_SACR0);
			val &= ~SACR0_RDMAEN;
			writel(val, i2s->base + I2S_SACR0);
		}

		/* interrupt disable */
		val = readl(i2s->base + I2S_SAIMR);
		val &= ~(SAIMR_ROR | SAIMR_RFS);
		writel(val, i2s->base + I2S_SAIMR);

		if(pdata && pdata->adc_ops.startup)
			pdata->adc_ops.startup(0);
	}

	val = readl(i2s->base + I2S_SACR1);
	if((val & (SACR1_DRPL | SACR1_DREC)) == (SACR1_DRPL | SACR1_DREC))
	{
		writel(readl(i2s->base + I2S_SACR0) & ~SACR0_ENB, i2s->base + I2S_SACR0);
	}
	xSemaphoreGive(i2s->mutex);
}

void i2s_interrupt_handler(void *param)
{
	struct ark_i2s_data *i2s = (struct ark_i2s_data *)param;
	unsigned int status;
	//unsigned int val;

	if(!i2s)
		return;

	status = readl(i2s->base + I2S_SASR0);
#if 1
	writel(0xFF, i2s->base + I2S_SAICR);

	if (status & SASR0_TUR) {
		//printf("i2s txfifo underrun.\n");
	}
#else
	val = readl(i2s->base + I2S_SAICR);
	if(status & SASR0_TFS)
		val |= SAICR_TFS;
	if(status & SASR0_TUR)
		val |= SAICR_TUR;
	if(status & SASR0_RFS)
		val |= SAICR_RFS;
	if(status & SASR0_ROR)
		val |= SAICR_ROR;
	writel(val, i2s->base + I2S_SAICR);
#endif
	writel(0x0, i2s->base + I2S_SAICR);
}

static void i2s_sw_reset(struct ark_i2s_data *i2s)
{
	writel(SACR0_RST, i2s->base + I2S_SACR0);
	udelay(1);
	writel(0, i2s->base + I2S_SACR0);
}

static int codec_init(struct ark_i2s_data *i2s, int flags)
{
	ark_i2s_pd *pdata = NULL;
	int ret = -1;

	if(!i2s->extdata)
	{
		pdata = (ark_i2s_pd *)pvPortMalloc(sizeof(struct ark_i2s_private_data));
		memset(&pdata->adc_ops, 0, sizeof(ark_i2s_pd));
		i2s->extdata = (void *)pdata;
	}
	else
	{
		pdata = i2s->extdata;
	}

	if(pdata)
	{
		if((flags  & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD)
		{
#if AUDIO_CODEC_ADC_IC != AUDIO_CODEC_ADC_NONE
			ret = audio_codec_adc_init(&pdata->adc_ops);
			if(ret == 0)
			{
				if(pdata->adc_ops.init)
					pdata->adc_ops.init(!i2s->cfg[AUDIO_STREAM_RECORD].master);
			}
#endif
		}
		if((flags & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY)
		{
#if AUDIO_CODEC_DAC_IC != AUDIO_CODEC_DAC_NONE
			ret = audio_codec_dac_init(&pdata->dac_ops);
			if(ret == 0)
			{
				if(pdata->dac_ops.init)
					pdata->dac_ops.init(!i2s->cfg[AUDIO_STREAM_REPLAY].master);
			}
#endif
		}
	}

	return ret;
}

int ark_i2s_init(struct ark_i2s_data *i2s, int flags)
{
	struct ark_i2s_cfg *cfg = NULL;
	int softreset;
	int irqn;

	if(i2s->id == I2S_ID0)
	{
		softreset = softreset_i2s;
		irqn = I2S_IRQn;
	}
	else if(i2s->id == I2S_ID1)
	{
		softreset = softreset_i2s1;
		irqn = I2S1_IRQn;
	}
	else
	{
		printf("%s, Invalid i2s id:%d.\n", __func__, i2s->id);
		return -EINVAL;
	}

	if(!(flags & AUDIO_FLAG_REPLAY_RECORD))
	{
		printf("%s, Invalid flags:0x%x.\n", __func__, flags);
		return -EINVAL;
	}

	if((flags & AUDIO_FLAG_REPLAY_RECORD) == AUDIO_FLAG_REPLAY_RECORD)
		i2s->full_duplex = 1;
	else
		i2s->full_duplex = 0;

	if((flags & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY)
	{
		i2s->dma_txch = dma_request_channel(I2S_DMA_TXCH);
		if (!i2s->dma_txch)
		{
			printf("%s() i2s replay dma_request_channel fail.\n",  __func__);
			return -ENODEV;
		}
		cfg = &i2s->cfg[AUDIO_STREAM_REPLAY];
		memset(cfg, 0, sizeof(struct ark_i2s_cfg));

		cfg->master = I2S_MASTER;
		cfg->lfirst = 0;
	}
	if((flags & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD)
	{
		i2s->dma_rxch = dma_request_channel(I2S_DMA_RXCH);
		if (!i2s->dma_rxch)
		{
			printf("%s() i2s record dma_request_channel fail.\n", __func__);
			return -ENODEV;
		}
		cfg = &i2s->cfg[AUDIO_STREAM_RECORD];
		memset(cfg, 0, sizeof(struct ark_i2s_cfg));

		cfg->master = I2S_MASTER;
		cfg->lfirst = 0;
	}

	i2s->mutex = xSemaphoreCreateMutex();

	sys_soft_reset(softreset);

	i2s_sw_reset(i2s);

	request_irq(irqn, 0, i2s_interrupt_handler, i2s);

	codec_init(i2s, flags);

	return 0;
}
