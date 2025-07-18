#include "FreeRTOS.h"
#include "chip.h"
#include "timers.h"
#include "board.h"
#include "i2s.h"
#include "audio.h"

#define TX_FIFO_SIZE (4096)
#define RX_FIFO_SIZE (4096)


#define I2S_DAC_NCO_REG			0x6000006C
#define I2S1_DAC_NCO_REG		0x60000148


struct ark_i2s_data amt630hv100_i2s_dac[I2S_NUMS] = {
	{
		.base = REGS_I2S_BASE,
		.nco_reg = I2S_DAC_NCO_REG,
		.id = I2S_ID0,
		.clkid = CLK_I2S,
		.extdata = NULL,
		.dma_txch = NULL,
		.dma_rxch = NULL,
	},
	{
		.base = REGS_I2S1_BASE,
		.nco_reg = I2S1_DAC_NCO_REG,
		.id = I2S_ID1,
		.clkid = CLK_I2S1,
		.extdata = NULL,
		.dma_txch = NULL,
		.dma_rxch = NULL,
	}
};

struct sound_device
{
	struct ark_i2s_data *i2s;
    struct audio_device audio;
    struct audio_configure replay_config;
    struct audio_configure record_config;
	TimerHandle_t guard_tx_timer;
	TimerHandle_t guard_rx_timer;
    uint8_t volume;
    uint8_t *tx_fifo;
    uint8_t *rx_fifo;
};

static struct sound_device snd_dev[I2S_NUMS] = {0};

static int ark_audio_init(struct audio_device *audio)
{
	struct sound_device *sdev;

    configASSERT(audio != NULL);

	sdev = (struct sound_device *)audio->user_data;
    configASSERT(sdev != NULL);

	ark_i2s_init(sdev->i2s, audio->flag);

    return 0;
}

static int ark_audio_start(struct audio_device *audio, int stream)
{
	struct sound_device *sdev;

    configASSERT(audio != NULL);

	sdev = (struct sound_device *)audio->user_data;
    configASSERT(sdev != NULL);

    if (stream == AUDIO_STREAM_REPLAY)
    {
        struct audio_caps caps;
        caps.main_type = AUDIO_TYPE_OUTPUT;
        caps.sub_type = AUDIO_DSP_SAMPLERATE;
        audio->ops->getcaps(audio, &caps);
        audio->ops->configure(audio, &caps);
		audio_tx_complete(audio);
		ark_i2s_startup(sdev->i2s, stream);
    }
	else if (stream == AUDIO_STREAM_RECORD)
	{
        struct audio_caps caps;
        caps.main_type = AUDIO_TYPE_INPUT;
        caps.sub_type = AUDIO_DSP_PARAM;
        audio->ops->getcaps(audio, &caps);
        audio->ops->configure(audio, &caps);
		if(audio_rx_complete(audio) == 0)
		{
			if(sdev->guard_rx_timer)
				xTimerStopFromISR(sdev->guard_rx_timer, 0);
			ark_i2s_startup(sdev->i2s, stream);
			if(sdev->guard_rx_timer)
			{
				xTimerResetFromISR(sdev->guard_rx_timer, 0);
				xTimerStartFromISR(sdev->guard_rx_timer, 0);
			}
		}
		else
			printf("%s() audio_rx_complete failed.\n", __func__);
	}
    return 0;
}

static int ark_audio_stop(struct audio_device *audio, int stream)
{
	struct sound_device *sdev;

	configASSERT(audio != NULL);

	sdev = (struct sound_device *)audio->user_data;
    configASSERT(sdev != NULL);
    configASSERT(sdev->i2s != NULL);

	ark_i2s_stop(sdev->i2s, stream);

	if(stream == AUDIO_STREAM_REPLAY)
		dma_stop_channel(sdev->i2s->dma_txch);
	else if(stream == AUDIO_STREAM_RECORD)
		dma_stop_channel(sdev->i2s->dma_rxch);
    return 0;
}

static int ark_audio_getcaps(struct audio_device *audio, struct audio_caps *caps)
{
	struct sound_device *sdev;
    int result = 0;

    configASSERT(audio != NULL);

	sdev = (struct sound_device *)audio->user_data;
    configASSERT(sdev != NULL);

    switch (caps->main_type)
    {
    case AUDIO_TYPE_QUERY: /* qurey the types of hw_codec device */
    {
        switch (caps->sub_type)
        {
        case AUDIO_TYPE_QUERY:
            caps->udata.mask = AUDIO_TYPE_OUTPUT | AUDIO_TYPE_MIXER;
            break;

        default:
            result = -1;
            break;
        }

        break;
    }

    case AUDIO_TYPE_OUTPUT: /* Provide capabilities of OUTPUT unit */
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            caps->udata.config.samplerate   = sdev->replay_config.samplerate;
            caps->udata.config.channels     = sdev->replay_config.channels;
            caps->udata.config.samplebits   = sdev->replay_config.samplebits;
            break;

        case AUDIO_DSP_SAMPLERATE:
            caps->udata.config.samplerate   = sdev->replay_config.samplerate;
            break;

        case AUDIO_DSP_CHANNELS:
            caps->udata.config.channels     = sdev->replay_config.channels;
            break;

        case AUDIO_DSP_SAMPLEBITS:
            caps->udata.config.samplebits   = sdev->replay_config.samplebits;
            break;

        default:
            result = -1;
            break;
        }

        break;
    }
    case AUDIO_TYPE_INPUT:
	{
		switch (caps->sub_type)
		{
		case AUDIO_DSP_PARAM:
			caps->udata.config.samplerate	= sdev->record_config.samplerate;
			caps->udata.config.channels 	= sdev->record_config.channels;
			caps->udata.config.samplebits	= sdev->record_config.samplebits;
			break;
		case AUDIO_DSP_SAMPLERATE:
			caps->udata.config.samplerate	= sdev->record_config.samplerate;
			break;
		case AUDIO_DSP_CHANNELS:
			caps->udata.config.channels 	= sdev->record_config.channels;
			break;
		case AUDIO_DSP_SAMPLEBITS:
			caps->udata.config.samplebits	= sdev->record_config.samplebits;
			break;
		default:
			result = -1;
			break;
		}
		break;
	}
    case AUDIO_TYPE_MIXER: /* report the Mixer Units */
    {
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_QUERY:
            caps->udata.mask = AUDIO_MIXER_VOLUME;
            break;

        case AUDIO_MIXER_VOLUME:
            caps->udata.value = sdev->volume;
            break;

        default:
            result = -1;
            break;
        }

        break;
    }

    default:
        result = -1;
        break;
    }

    return result;
}

static int ark_audio_configure(struct audio_device *audio, struct audio_caps *caps)
{
	struct sound_device *sdev;
    int result = 0;

    configASSERT(audio != NULL);

	sdev = (struct sound_device *)audio->user_data;
    configASSERT(sdev != NULL);

    switch (caps->main_type)
    {
    case AUDIO_TYPE_MIXER:
    {
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_MUTE:
        {
			result = ark_i2s_set_volume(sdev->i2s, 0, 0);
			sdev->volume = 0;
			break;
        }

        case AUDIO_MIXER_VOLUME:
        {
			int volume = caps->udata.value;
			result = ark_i2s_set_volume(sdev->i2s, volume, volume);
			sdev->volume = volume;
			break;
        }
        }
        break;
    }
    case AUDIO_TYPE_OUTPUT:
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
        {
            struct audio_configure config = caps->udata.config;
            sdev->replay_config.channels = config.channels;
            sdev->replay_config.samplebits = config.samplebits;
            sdev->replay_config.samplerate = config.samplerate;
			//ark_i2s_set_rate(sdev->i2s, config.samplerate);
			ark_i2s_set_params(sdev->i2s, AUDIO_STREAM_REPLAY, config.samplerate, config.channels, config.samplebits);
            break;
        }

        case AUDIO_DSP_SAMPLERATE:
        {
            struct audio_configure config = caps->udata.config;
            sdev->replay_config.samplerate = config.samplerate;
			ark_i2s_set_rate(sdev->i2s, AUDIO_STREAM_REPLAY, config.samplerate);
            break;
        }

        default:
            result = -1;
            break;
        }
        break;
    }
	case AUDIO_TYPE_INPUT:
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
        {
            struct audio_configure config = caps->udata.config;
            sdev->record_config.channels = config.channels;
            sdev->record_config.samplebits = config.samplebits;
            sdev->record_config.samplerate = config.samplerate;
			ark_i2s_set_params(sdev->i2s, AUDIO_STREAM_RECORD, config.samplerate, config.channels, config.samplebits);
            break;
        }
        case AUDIO_DSP_SAMPLERATE:
        {
            struct audio_configure config = caps->udata.config;
            sdev->record_config.samplerate = config.samplerate;
			ark_i2s_set_rate(sdev->i2s, AUDIO_STREAM_RECORD, config.samplerate);
            break;
        }
        default:
            result = -1;
            break;
        }
        break;
    }
    }

    return result;
}

static void ark_i2s_dma_tx_callback(void *param, unsigned int mask)
{
	struct sound_device *sdev = (struct sound_device *)param;

    configASSERT(sdev != NULL);

	if(sdev->guard_tx_timer)
		xTimerStopFromISR(sdev->guard_tx_timer, 0);
	audio_tx_complete(&sdev->audio);
}

static void ark_i2s_dma_rx_callback(void *param, unsigned int mask)
{
	struct sound_device *sdev = (struct sound_device *)param;

    configASSERT(sdev != NULL);

	if(sdev->guard_rx_timer)
		xTimerStopFromISR(sdev->guard_rx_timer, 0);

	audio_rx_complete(&sdev->audio);
}

#if 1
static void ark_i2s_dma_tx_timeout_callback(TimerHandle_t timer)
{
	struct sound_device *sdev = (struct sound_device *)pvTimerGetTimerID(timer);

    configASSERT(sdev != NULL);
    configASSERT(sdev->i2s != NULL);

	if(sdev->audio.flag & AUDIO_FLAG_REPLAY == AUDIO_FLAG_REPLAY) {
		printf("i2s%d dma tx timeout.\n", sdev->i2s->id);
		if(sdev->guard_tx_timer)
			xTimerStopFromISR(sdev->guard_tx_timer, 0);
		audio_tx_complete(&sdev->audio);
	}
}

static void ark_i2s_dma_rx_timeout_callback(TimerHandle_t timer)
{
	struct sound_device *sdev = (struct sound_device *)pvTimerGetTimerID(timer);

    configASSERT(sdev != NULL);
    configASSERT(sdev->i2s != NULL);

	if(sdev->audio.flag & AUDIO_FLAG_RECORD == AUDIO_FLAG_RECORD) {
		printf("i2s:%d dma rx timeout.\n", sdev->i2s->id);
		if(sdev->guard_rx_timer)
			xTimerStopFromISR(sdev->guard_rx_timer, 0);
		audio_rx_complete(&sdev->audio);
	}
}
#else
static void ark_i2s_dma_timeout_callback(TimerHandle_t timer)
{
	struct sound_device *sdev = (struct sound_device *)pvTimerGetTimerID(timer);

    configASSERT(sdev != NULL);

	if((sdev->audio.flag & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY) {
		printf("i2s dma tx timeout.\n");
		if(sdev->guard_tx_timer)
			xTimerStopFromISR(sdev->guard_tx_timer, 0);
		audio_tx_complete(&sdev->audio);
	} else if((sdev->audio.flag & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD) {
		printf("i2s dma rx timeout.\n");
		if(sdev->guard_rx_timer)
			xTimerStopFromISR(sdev->guard_rx_timer, 0);
        ark_i2s_restart_audio(sdev->i2s);
		audio_rx_complete(&sdev->audio);
	}
}
#endif

static size_t ark_audio_transmit(struct audio_device *audio, const void *writeBuf, void *readBuf, size_t size)
{
	struct ark_i2s_data *i2s;
	struct dma_config cfg = {0};
	struct sound_device *sdev = (struct sound_device *)audio->user_data;
	int ret;

    configASSERT(sdev != NULL);

	i2s = sdev->i2s;

	cfg.dst_maxburst = 16;
	cfg.src_maxburst = 16;
	cfg.transfer_size = size;
	cfg.dst_ahbmaster_sel = 0;
	cfg.src_ahbmaster_sel = 1;

	if(writeBuf)
	{
		cfg.dst_addr_width = DMA_BUSWIDTH_4_BYTES;
		cfg.src_addr_width = DMA_BUSWIDTH_4_BYTES;
		cfg.src_addr = (dma_addr_t)writeBuf;
		cfg.dst_addr = i2s->base + I2S_SADR;
		cfg.direction = DMA_MEM_TO_DEV;
		if(i2s->id == I2S_ID1)
			cfg.dst_id = I2S1_TX;
		else /*if(i2s->id == I2S_ID0)*/
			cfg.dst_id = I2S_TX;

		ret = dma_config_channel(i2s->dma_txch, &cfg);
		if (ret) {
			printf("%s, dma_config_channel tx failed.\n", __func__);
			return -1;
		}

		dma_register_complete_callback(i2s->dma_txch, ark_i2s_dma_tx_callback, sdev);

		/* clean cache before write */
		CP15_clean_dcache_for_dma((u32)writeBuf, (u32)writeBuf + size);
		dma_start_channel(i2s->dma_txch);
		if(sdev->guard_tx_timer)
		{
			xTimerResetFromISR(sdev->guard_tx_timer, 0);
			xTimerStartFromISR(sdev->guard_tx_timer, 0);
		}
	}
	else if(readBuf)
	{
		if(i2s->cfg[AUDIO_STREAM_RECORD].channels == 2) {
			cfg.dst_addr_width = DMA_BUSWIDTH_4_BYTES;
			cfg.src_addr_width = DMA_BUSWIDTH_4_BYTES;
		} else {
			cfg.dst_addr_width = DMA_BUSWIDTH_2_BYTES;
			cfg.src_addr_width = DMA_BUSWIDTH_2_BYTES;
		}
		cfg.src_addr = i2s->base + I2S_SADR;
		cfg.dst_addr = (dma_addr_t)readBuf;
		cfg.direction = DMA_DEV_TO_MEM;
		if(i2s->id == I2S_ID1)
			cfg.src_id = I2S1_RX;
		else /*if(i2s->id == I2S_ID0)*/
			cfg.src_id = I2S_RX;

		ret = dma_config_channel(i2s->dma_rxch, &cfg);
		if (ret) {
			printf("%s, dma_config_channel rx failed.\n", __func__);
			return -1;
		}

		dma_register_complete_callback(i2s->dma_rxch, ark_i2s_dma_rx_callback, sdev);

		/* clean cache before read */
		CP15_flush_dcache_for_dma((u32)readBuf, (u32)readBuf + size);

		dma_start_channel(i2s->dma_rxch);
		if(sdev->guard_rx_timer)
		{
			xTimerResetFromISR(sdev->guard_rx_timer, 0);
			xTimerStartFromISR(sdev->guard_rx_timer, 0);
		}
	}

	return size;
}

static void ark_audio_buffer_info(struct audio_device *audio, struct audio_buf_info *info, int flags)
{
    configASSERT(audio != NULL);
	struct sound_device *sdev = (struct sound_device *)audio->user_data;
    configASSERT(sdev != NULL);
    /**
     *               TX_FIFO
     * +----------------+----------------+
     * |     block1     |     block2     |
     * +----------------+----------------+
     *  \  block_size  /
     */
	if(flags == AUDIO_FLAG_REPLAY)
	{
	    info->buffer      = sdev->tx_fifo;
	    info->total_size  = TX_FIFO_SIZE;
	    info->block_size  = TX_FIFO_SIZE / 2;
	    info->block_count = 2;
	}
	else if(flags == AUDIO_FLAG_RECORD)
	{
	    info->buffer      = sdev->rx_fifo;
	    info->total_size  = RX_FIFO_SIZE;
	    info->block_size  = RX_FIFO_SIZE/2;
	    info->block_count = 2;
	}
}

static struct audio_ops audio_ops =
{
    .getcaps = ark_audio_getcaps,
    .configure = ark_audio_configure,
    .init = ark_audio_init,
    .start = ark_audio_start,
    .stop = ark_audio_stop,
    .transmit = ark_audio_transmit,
    .buffer_info = ark_audio_buffer_info,
};

int sound_init(void)
{
	uint8_t *tx_fifo = NULL;
	struct sound_device *sdev;
	int flags;
	int i;

	for(i=0; i<I2S_NUMS; i++)
	{
		flags = 0;

#ifdef AUDIO_REPLAY_I2S
		if(AUDIO_REPLAY_I2S == i) {
			flags |= AUDIO_FLAG_REPLAY;
		}
#endif
#ifdef AUDIO_RECORD_I2S
		if(AUDIO_RECORD_I2S == i) {
			flags |= AUDIO_FLAG_RECORD;
		}
#endif
		if(!(flags & AUDIO_FLAG_REPLAY_RECORD))
			continue;

		sdev = &snd_dev[i];
		sdev->i2s = &amt630hv100_i2s_dac[i];

		/* init default configuration */
		if((flags & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY)
		{
			tx_fifo = pvPortMalloc(TX_FIFO_SIZE);
			if (!tx_fifo)
			{
				printf("%s, pvPortMalloc failed\n", __func__);
				return -ENOMEM;
			}
			sdev->tx_fifo = tx_fifo;
			sdev->replay_config.samplerate = 44100;		//will be reset when open() audio.
			sdev->replay_config.channels   = 2;
			sdev->replay_config.samplebits = 16;
			sdev->volume                   = 100;
			sdev->guard_tx_timer = xTimerCreate("Replay Timer", pdMS_TO_TICKS(200), pdFALSE,
													//NULL, ark_i2s_dma_timeout_callback);
													sdev, ark_i2s_dma_tx_timeout_callback);
		}
		if((flags & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD)
		{
			sdev->rx_fifo = NULL;
			sdev->record_config.samplerate = 16000;	//will be reset when open() audio.
			sdev->record_config.channels   = 2;
			sdev->record_config.samplebits = 16;
			sdev->volume                   = 100;
			sdev->guard_rx_timer = xTimerCreate("Record Timer", pdMS_TO_TICKS(200), pdFALSE,
													sdev, ark_i2s_dma_rx_timeout_callback);
		}

		sdev->audio.ops = &audio_ops;
		sdev->audio.user_data = (void *)sdev;
		sdev->audio.flag = flags;
		sdev->audio.id = i;
		audio_register(&sdev->audio);
	}

    return 0;
}
