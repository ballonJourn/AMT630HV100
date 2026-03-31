#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "audio.h"

enum
{
    REPLAY_EVT_NONE  = 0x00,
    REPLAY_EVT_START = 0x01,
    REPLAY_EVT_STOP  = 0x02,
};

struct audio_queue_data
{
	uint8_t *data;
	int size;
};

struct audio_device *audio_dev[I2S_NUMS] = {NULL, NULL};

static int audio_send_replay_frame(struct audio_device *audio)
{
    int result = 0;
    uint8_t *data;
    size_t dst_size, src_size;
    uint16_t position, remain_bytes = 0, index = 0;
    struct audio_buf_info *buf_info;
	struct audio_queue_data qdata;

    configASSERT(audio != NULL);

    buf_info = &audio->replay->buf_info;
    /* save current pos */
    position = audio->replay->pos;
    dst_size = buf_info->block_size;

    /* check replay queue is empty */
    if (xQueueIsQueueEmptyFromISR(audio->replay->queue) == pdTRUE)
    {
        /* ack stop event */
        if (audio->replay->event & REPLAY_EVT_STOP) {
			xQueueSendFromISR(audio->replay->cmp, NULL, 0);
			return 0;
        }

        /* send zero frames */
        memset(&buf_info->buffer[audio->replay->pos], 0, dst_size);

        audio->replay->pos += dst_size;
        audio->replay->pos %= buf_info->total_size;
    }
    else
    {
        memset(&buf_info->buffer[audio->replay->pos], 0, dst_size);

        /* copy data from memory pool to hardware device fifo */
        while (index < dst_size)
        {
            result = xQueuePeekFromISR(audio->replay->queue, &qdata);
            if (result != pdTRUE)
            {
                TRACE_DEBUG("under run %d, remain %d", audio->replay->pos, remain_bytes);
                audio->replay->pos -= remain_bytes;
                audio->replay->pos += dst_size;
                audio->replay->pos %= buf_info->total_size;
                audio->replay->read_index = 0;
                result = -1;
                break;
            }
			
			data = qdata.data;
			src_size = qdata.size;
            remain_bytes = configMIN((dst_size - index), (src_size - audio->replay->read_index));
            memcpy(&buf_info->buffer[audio->replay->pos],
                   &data[audio->replay->read_index], remain_bytes);

            index += remain_bytes;
            audio->replay->read_index += remain_bytes;
            audio->replay->pos += remain_bytes;
            audio->replay->pos %= buf_info->total_size;

            if (audio->replay->read_index == src_size)
            {
                audio->replay->read_index = 0;
                xQueueReceiveFromISR(audio->replay->queue, &qdata, 0);
				for (int i = 0; i < AUDIO_REPLAY_MP_BLOCK_COUNT; i++) {
					if (qdata.data == audio->replay->mempool + AUDIO_REPLAY_MP_BLOCK_SIZE * i) {
						audio->replay->mpstatus[i] = 0;
						break;
					}
				}
            }
        }
    }

    if (audio->ops->transmit != NULL)
    {
        if (audio->ops->transmit(audio, &buf_info->buffer[position], NULL, dst_size) != dst_size)
            result = -1;
    }

    return result;
}

static int audio_receive_record_frame(struct audio_device *audio)
{
    struct audio_buf_info *buf_info;
    size_t dst_size;
	int read_pos;
    int result = 0;

    configASSERT(audio != NULL);

	/* ack stop event */
	if (audio->record->event & REPLAY_EVT_STOP) {
		xQueueSendFromISR(audio->record->cmp, NULL, 0);
		return 0;
	}

    buf_info = &audio->record->buf_info;
	if(buf_info->buffer && buf_info->total_size)
	{
		if(audio->record->remain_size <= 0)
		{
			if(audio->record->read_index == buf_info->total_size)
			{
				if(audio->record->receive_cb)
					audio->record->receive_cb(audio);
			}
			audio->record->remain_size = buf_info->total_size;
			audio->record->read_index = 0;
		}

		read_pos = audio->record->read_index;
	    dst_size = buf_info->block_size;

		if(audio->record->remain_size < buf_info->block_size)
			dst_size = audio->record->remain_size;

	    if(audio->ops->transmit)
	    {
	        if(audio->ops->transmit(audio, NULL, &buf_info->buffer[read_pos], dst_size) != dst_size)
			{
				printf("%s() transmit failed.\n", __func__);
	            result = -1;
	        }
			else
			{
				audio->record->read_index += dst_size;
				audio->record->remain_size -= dst_size;
				if(audio->record->remain_size < 0)
				{
					printf("%s() Invalid remain_size:%d.\n", __func__, audio->record->remain_size);
					audio->record->remain_size = 0;
				}
			}
	    }
	}
	else
	{
		if(!buf_info->buffer)
			printf("%s() record buffer is NULL.\n", __func__);
		if(!buf_info->total_size)
			printf("%s() record buffer size is 0.\n", __func__);
		result = -1;
	}
    return result;
}

static int audio_flush_replay_frame(struct audio_device *audio)
{
    int result = 0;

    if (audio->replay->write_index)
    {
    	struct audio_queue_data qdata = {audio->replay->write_data, audio->replay->write_index};
        result = xQueueSend(audio->replay->queue, &qdata, portMAX_DELAY);
        audio->replay->write_index = 0;
    }

    return result;
}

static int audio_replay_start(struct audio_device *audio)
{
    int result = 0;

    if (audio->replay->activated != true)
    {
        /* start playback hardware device */
        if (audio->ops->start)
            result = audio->ops->start(audio, AUDIO_STREAM_REPLAY);

        audio->replay->activated = true;
        TRACE_DEBUG("start audio replay device");
    }

    return result;
}

static int audio_replay_stop(struct audio_device *audio)
{
    int result = 0;

    if (audio->replay->activated == true)
    {
        /* flush replay remian frames */
        audio_flush_replay_frame(audio);

        /* notify irq(or thread) to stop the data transmission */
        audio->replay->event |= REPLAY_EVT_STOP;

        /* waiting for the remaining data transfer to complete */
		xQueueReset(audio->replay->cmp);
		xQueueReceive(audio->replay->cmp, NULL, pdMS_TO_TICKS(2000));
        audio->replay->event &= ~REPLAY_EVT_STOP;

        /* stop playback hardware device */
        if (audio->ops->stop)
            result = audio->ops->stop(audio, AUDIO_STREAM_REPLAY);

        audio->replay->activated = false;
        TRACE_DEBUG("stop audio replay device");
    }

    return result;
}

static int audio_record_start(struct audio_device *audio)
{
    int result = 0;

    if (audio->record->activated != true)
    {
        /* start record hardware device */
        if (audio->ops->start)
            result = audio->ops->start(audio, AUDIO_STREAM_RECORD);

        audio->record->activated = true;
        TRACE_DEBUG("start audio record device");
    }

    return result;
}

static int audio_record_stop(struct audio_device *audio)
{
    int result = 0;

    if (audio->record->activated == true)
    {
		/* notify irq(or thread) to stop the data transmission */
		audio->record->event |= REPLAY_EVT_STOP;

		/* waiting for the remaining data transfer to complete */
		xQueueReset(audio->record->cmp);
		xQueueReceive(audio->record->cmp, NULL, pdMS_TO_TICKS(1000));

        /* stop record hardware device */
        if (audio->ops->stop)
            result = audio->ops->stop(audio, AUDIO_STREAM_RECORD);

		audio->record->event &= ~REPLAY_EVT_STOP;

        audio->record->activated = false;
        TRACE_DEBUG("stop audio record device");
    }

    return result;
}

static int audio_dev_init(struct audio_device *audio)
{
    int result = 0;

    configASSERT(audio != NULL);

    /* initialize replay & record */
    audio->replay = NULL;
    audio->record = NULL;

    /* initialize replay */
    if ((audio->flag & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY)
    {
        struct audio_replay *replay = (struct audio_replay *) pvPortMalloc(sizeof(struct audio_replay));

        if (replay == NULL)
            return -ENOMEM;
        memset(replay, 0, sizeof(struct audio_replay));

		/* alloc mempool */
		replay->mempool = pvPortMalloc(AUDIO_REPLAY_MP_BLOCK_SIZE * AUDIO_REPLAY_MP_BLOCK_COUNT);
		if (!replay->mempool)
			return -ENOMEM;

        /* init queue for audio replay */
		replay->queue = xQueueCreate(CFG_AUDIO_REPLAY_QUEUE_COUNT, sizeof(struct audio_queue_data));

        /* init mutex lock for audio replay */
        replay->lock = xSemaphoreCreateMutex();

		replay->cmp = xQueueCreate(1, 0);

        replay->activated = false;
        audio->replay = replay;

		/* get replay buffer information */
		if (audio->ops->buffer_info)
			audio->ops->buffer_info(audio, &audio->replay->buf_info, AUDIO_FLAG_REPLAY);
    }

    /* initialize record */
    if ((audio->flag & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD)
    {
        struct audio_record *record = (struct audio_record *) pvPortMalloc(sizeof(struct audio_record));
        //uint8_t *buffer;

        if (record == NULL)
            return -ENOMEM;
        memset(record, 0, sizeof(struct audio_record));

        /* init pipe for record*/
        /* buffer = pvPortMalloc(AUDIO_RECORD_PIPE_SIZE);
        if (buffer == NULL)
        {
            vPortFree(record);
            TRACE_ERROR("malloc memory for for record pipe failed");
            return -ENOMEM;
        }
        audio_pipe_init(&record->pipe, "record",
                           (int32_t)(RT_PIPE_FLAG_FORCE_WR | RT_PIPE_FLAG_BLOCK_RD),
                           buffer,
                           RT_AUDIO_RECORD_PIPE_SIZE); */
		record->cmp = xQueueCreate(1, 0);

        record->activated = false;
        audio->record = record;

		/* get record buffer information */
		if (audio->ops->buffer_info)
			audio->ops->buffer_info(audio, &audio->record->buf_info, AUDIO_FLAG_RECORD);
    }

    /* initialize hardware configuration */
    if (audio->ops->init)
        audio->ops->init(audio);

    return result;
}

struct audio_device *audio_dev_open(uint32_t oflag)
{
    struct audio_device *audio = NULL;

#ifdef AUDIO_REPLAY_I2S
    /* initialize the Rx/Tx structure according to open flag */
    if ((oflag & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY)
    {
		audio = audio_dev[AUDIO_REPLAY_I2S];
        if (audio && audio->replay->activated != true)
        {
            TRACE_DEBUG("open audio replay device, oflag = %x\n", oflag);
            audio->replay->write_index = 0;
            audio->replay->read_index = 0;
            audio->replay->pos = 0;
            audio->replay->event = REPLAY_EVT_NONE;
			for (int i = 0; i < AUDIO_REPLAY_MP_BLOCK_COUNT; i++)
				audio->replay->mpstatus[i] = 0;
        }
    }
#endif
#ifdef AUDIO_RECORD_I2S
    if ((oflag & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD)
    {
		audio = audio_dev[AUDIO_RECORD_I2S];

        if (audio && audio->record->activated != true)
        {
            TRACE_DEBUG("open audio record device ,oflag = %x\n", oflag);
            audio->record->event = REPLAY_EVT_NONE;
            audio->record->read_index = 0;
            audio->record->remain_size = 0;
        }
    }
#endif
    return audio;
}

int audio_dev_close(struct audio_device *audio, uint32_t oflag)
{
    configASSERT(audio != NULL);

    if ((oflag & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY)
    {
        /* stop replay stream */
        audio_replay_stop(audio);
    }

    if ((oflag & AUDIO_FLAG_RECORD) == AUDIO_FLAG_RECORD)
    {
        /* stop record stream */
        audio_record_stop(audio);
    }

    return 0;
}

int audio_dev_register_record_callback(struct audio_device *audio, int (*callback)(struct audio_device *audio))
{
    configASSERT(audio != NULL);

    if (((audio->flag & AUDIO_FLAG_RECORD) != AUDIO_FLAG_RECORD) || (audio->record == NULL))
		return -EINVAL;

	audio->record->receive_cb = callback;

	return 0;
}

int audio_dev_record_set_param(struct audio_device *audio, uint8_t *buf, int size)
{
	if(buf && ((size > 0) && (size%32 == 0)))	//size should align with 32 bytes.
	{
		audio->record->buf_info.total_size = size;
		audio->record->remain_size = audio->record->buf_info.total_size;
		audio->record->buf_info.buffer = buf;
		audio->record->remain_size = 0;
		audio->record->read_index = 0;
		return  0;
	}

	return -1;
}


int audio_dev_record_start(struct audio_device *audio)
{
    configASSERT(audio != NULL);

	if (audio->record->activated != true)
	{
		if(!audio->record->buf_info.buffer || !audio->record->buf_info.total_size)
		{
			printf("%s() Invalid buffer:%p or size:%d\n", __func__, audio->record->buf_info.buffer, audio->record->buf_info.total_size);
			return -1;
		}
		audio->record->remain_size = 0;
		audio->record->read_index = 0;
		audio_record_start(audio);
		audio->record->activated = true;
	}
	return 0;
}

int audio_dev_record_stop(struct audio_device *audio)
{
	if (audio->flag == AUDIO_FLAG_RECORD)
    {
        /* stop record stream */
        audio_record_stop(audio);
    }
	return 0;
}

size_t audio_dev_read(struct audio_device *audio, void *buffer, size_t size)
{
    configASSERT(audio != NULL);

    if ((audio->flag != AUDIO_FLAG_RECORD) || (audio->record == NULL))
        return 0;

	printf("%s() Invalid interface.\n", __func__);

    return 0;//device_read(RT_DEVICE(&audio->record->pipe), pos, buffer, size);
}

size_t audio_dev_write(struct audio_device *audio, const void *buffer, size_t size)
{

    uint8_t *ptr;
    uint16_t block_size, remain_bytes, index = 0;

    configASSERT(audio != NULL);

    if (!((audio->flag & AUDIO_FLAG_REPLAY) == AUDIO_FLAG_REPLAY) || (audio->replay == NULL))
        return 0;

    /* push a new frame to replay data queue */
    ptr = (uint8_t *)buffer;
    block_size = AUDIO_REPLAY_MP_BLOCK_SIZE;

    xSemaphoreTake(audio->replay->lock, portMAX_DELAY);
    while (index < size)
    {
        /* request buffer from replay memory pool */
        if (audio->replay->write_index % block_size == 0)
        {
        	uint8_t *mpbuf = NULL;
			uint32_t st = xTaskGetTickCount();
        	while(1) {
				int i;
				portENTER_CRITICAL();
				for (i = 0; i < AUDIO_REPLAY_MP_BLOCK_COUNT; i++) {
					if (!audio->replay->mpstatus[i]) {
						mpbuf = audio->replay->mempool + AUDIO_REPLAY_MP_BLOCK_SIZE * i;
						audio->replay->mpstatus[i] = 1;
						break;
					}
				}
				portEXIT_CRITICAL();
				if (mpbuf)
					break;
				if (xTaskGetTickCount() - st > pdMS_TO_TICKS(1000)) {
					printf("wait mempool free timeout.\n");
					mpbuf = audio->replay->mempool;
					for (i = 1; i < AUDIO_REPLAY_MP_BLOCK_COUNT; i++)
						audio->replay->mpstatus[i] = 0;
					break;
				}
				vTaskDelay(1);
			}
			audio->replay->write_data = mpbuf;
            memset(audio->replay->write_data, 0, block_size);
        }

        /* copy data to replay memory pool */
        remain_bytes = configMIN((block_size - audio->replay->write_index), (size - index));
        memcpy(&audio->replay->write_data[audio->replay->write_index], &ptr[index], remain_bytes);

        index += remain_bytes;
        audio->replay->write_index += remain_bytes;
        audio->replay->write_index %= block_size;

        if (audio->replay->write_index == 0)
        {
        	struct audio_queue_data qdata = {audio->replay->write_data, block_size};
            xQueueSend(audio->replay->queue, &qdata, portMAX_DELAY);
        }
    }
    xSemaphoreGive(audio->replay->lock);

    /* check replay state */
    if (audio->replay->activated != true)
    {
        audio_replay_start(audio);
        audio->replay->activated = true;
    }

    return index;
}

int audio_dev_configure(struct audio_device *audio, struct audio_caps *caps)
{
    int result = 0;

    if (audio->ops->configure != NULL)
    {
        result = audio->ops->configure(audio, caps);
    }

    return result;
}

int audio_register(struct audio_device *audio)
{
    int result = 0;

    configASSERT(audio != NULL);

    audio->rx_indicate = NULL;
    audio->tx_complete = NULL;

    /* initialize audio device */
    result = audio_dev_init(audio);

	audio_dev[audio->id] = audio;

    return result;
}

int audio_samplerate_to_speed(uint32_t bitValue)
{
    int speed = 0;
    switch (bitValue)
    {
    case AUDIO_SAMP_RATE_8K:
        speed = 8000;
        break;
    case AUDIO_SAMP_RATE_11K:
        speed = 11052;
        break;
    case AUDIO_SAMP_RATE_16K:
        speed = 16000;
        break;
    case AUDIO_SAMP_RATE_22K:
        speed = 22050;
        break;
    case AUDIO_SAMP_RATE_32K:
        speed = 32000;
        break;
    case AUDIO_SAMP_RATE_44K:
        speed = 44100;
        break;
    case AUDIO_SAMP_RATE_48K:
        speed = 48000;
        break;
    case AUDIO_SAMP_RATE_96K:
        speed = 96000;
        break;
    case AUDIO_SAMP_RATE_128K:
        speed = 128000;
        break;
    case AUDIO_SAMP_RATE_160K:
        speed = 160000;
        break;
    case AUDIO_SAMP_RATE_172K:
        speed = 176400;
        break;
    case AUDIO_SAMP_RATE_192K:
        speed = 192000;
        break;
    default:
        break;
    }

    return speed;
}

void audio_tx_complete(struct audio_device *audio)
{
    /* try to send next frame */
    audio_send_replay_frame(audio);
}

int audio_rx_complete(struct audio_device *audio)
{
    /* try to receive next frame */
	return audio_receive_record_frame(audio);
}


void audio_rx_done(struct audio_device *audio, uint8_t *pbuf, size_t len)
{
    /* save data to record pipe */
    //device_write(RT_DEVICE(&audio->record->pipe), 0, pbuf, len);

    /* invoke callback */
    /* if (audio->parent.rx_indicate != NULL)
        audio->parent.rx_indicate(&audio->parent, len); */
}
