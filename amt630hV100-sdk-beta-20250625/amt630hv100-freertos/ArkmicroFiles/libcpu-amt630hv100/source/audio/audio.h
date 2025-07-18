#ifndef __AUDIO_H__
#define __AUDIO_H__

/* AUDIO command */
#define _AUDIO_CTL(a) (0x10 + a)

#define AUDIO_CTL_GETCAPS                   _AUDIO_CTL(1)
#define AUDIO_CTL_CONFIGURE                 _AUDIO_CTL(2)
#define AUDIO_CTL_START                     _AUDIO_CTL(3)
#define AUDIO_CTL_STOP                      _AUDIO_CTL(4)
#define AUDIO_CTL_GETBUFFERINFO             _AUDIO_CTL(5)

/* Audio Device Types */
#define AUDIO_TYPE_QUERY                    0x00
#define AUDIO_TYPE_INPUT                    0x01
#define AUDIO_TYPE_OUTPUT                   0x02
#define AUDIO_TYPE_MIXER                    0x04

/* Supported Sampling Rates */
#define AUDIO_SAMP_RATE_8K                  0x0001
#define AUDIO_SAMP_RATE_11K                 0x0002
#define AUDIO_SAMP_RATE_16K                 0x0004
#define AUDIO_SAMP_RATE_22K                 0x0008
#define AUDIO_SAMP_RATE_32K                 0x0010
#define AUDIO_SAMP_RATE_44K                 0x0020
#define AUDIO_SAMP_RATE_48K                 0x0040
#define AUDIO_SAMP_RATE_96K                 0x0080
#define AUDIO_SAMP_RATE_128K                0x0100
#define AUDIO_SAMP_RATE_160K                0x0200
#define AUDIO_SAMP_RATE_172K                0x0400
#define AUDIO_SAMP_RATE_192K                0x0800

/* Supported Bit Rates */
#define AUDIO_BIT_RATE_22K                  0x01
#define AUDIO_BIT_RATE_44K                  0x02
#define AUDIO_BIT_RATE_48K                  0x04
#define AUDIO_BIT_RATE_96K                  0x08
#define AUDIO_BIT_RATE_128K                 0x10
#define AUDIO_BIT_RATE_160K                 0x20
#define AUDIO_BIT_RATE_172K                 0x40
#define AUDIO_BIT_RATE_192K                 0x80

/* Support Dsp(input/output) Units controls */
#define AUDIO_DSP_PARAM                     0           /* get/set all params */
#define AUDIO_DSP_SAMPLERATE                1           /* samplerate */
#define AUDIO_DSP_CHANNELS                  2           /* channels */
#define AUDIO_DSP_SAMPLEBITS                3           /* sample bits width */

/* Supported Mixer Units controls */
#define AUDIO_MIXER_QUERY                   0x0000
#define AUDIO_MIXER_MUTE                    0x0001
#define AUDIO_MIXER_VOLUME                  0x0002
#define AUDIO_MIXER_BASS                    0x0004
#define AUDIO_MIXER_MID                     0x0008
#define AUDIO_MIXER_TREBLE                  0x0010
#define AUDIO_MIXER_EQUALIZER               0x0020
#define AUDIO_MIXER_LINE                    0x0040
#define AUDIO_MIXER_DIGITAL                 0x0080
#define AUDIO_MIXER_MIC                     0x0100
#define AUDIO_MIXER_VITURAL                 0x0200
#define AUDIO_MIXER_EXTEND                  0x8000    /* extend mixer command */

#define AUDIO_VOLUME_MAX                    (100)
#define AUDIO_VOLUME_MIN                    (0)

#define CFG_AUDIO_REPLAY_QUEUE_COUNT        8

/**
 * audio flags defitions
 */
//#define AUDIO_FLAG_REPLAY		0
//#define AUDIO_FLAG_RECORD		1

#define AUDIO_REPLAY_MP_BLOCK_SIZE 		4096
#define AUDIO_REPLAY_MP_BLOCK_COUNT 	4
#define AUDIO_RECORD_PIPE_SIZE 			2048

//typedef int (*audio_record_callback)(struct audio_device *audio, void *buffer, int size);

enum
{
    AUDIO_STREAM_REPLAY = 0,
    AUDIO_STREAM_RECORD = 1,
};

/* the preferred number and size of audio pipeline buffer for the audio device */
struct audio_buf_info
{
    uint8_t *buffer;
    uint16_t block_size;
    uint16_t block_count;
    uint32_t total_size;
};

struct audio_device;
struct audio_caps;
struct audio_configure;
struct audio_ops
{
    int (*getcaps)(struct audio_device *audio, struct audio_caps *caps);
    int (*configure)(struct audio_device *audio, struct audio_caps *caps);
    int (*init)(struct audio_device *audio);
    int (*start)(struct audio_device *audio, int stream);
    int (*stop)(struct audio_device *audio, int stream);
    size_t (*transmit)(struct audio_device *audio, const void *writeBuf, void *readBuf, size_t size);
    /* get page size of codec or private buffer's info */
    void (*buffer_info)(struct audio_device *audio, struct audio_buf_info *info, int flags);
};

struct audio_configure
{
    uint32_t samplerate;
    uint16_t channels;
    uint16_t samplebits;
};

struct audio_caps
{
    int main_type;
    int sub_type;

    union
    {
        uint32_t mask;
        int     value;
        struct audio_configure config;
    } udata;
};

struct audio_replay
{
    QueueHandle_t queue;
    SemaphoreHandle_t lock;
    QueueHandle_t cmp;
    struct audio_buf_info buf_info;
	uint8_t *mempool;
	uint8_t mpstatus[AUDIO_REPLAY_MP_BLOCK_COUNT];
    uint8_t *write_data;
    uint16_t write_index;
    uint16_t read_index;
    uint32_t pos;
    uint8_t event;
    bool activated;
};

struct audio_record
{
    QueueHandle_t cmp;
    struct audio_buf_info buf_info;
    int read_index;
	int remain_size;
    uint8_t event;
    bool activated;
	int (*receive_cb)(struct audio_device *audio);
};

struct audio_device
{
    struct audio_ops *ops;
    struct audio_replay *replay;
    struct audio_record *record;
	/* device call back */
    int (*rx_indicate)(struct audio_device *audio, size_t size);
    int (*tx_complete)(struct audio_device *audio, void *buffer);
	uint32_t flag;
	uint32_t id;
	void *user_data;	/**< device private data */
};

int audio_register(struct audio_device *audio);
void audio_tx_complete(struct audio_device *audio);
int audio_rx_complete(struct audio_device *audio);
void audio_rx_done(struct audio_device *audio, uint8_t *pbuf, size_t len);
struct audio_device *audio_dev_open(uint32_t oflag);
int audio_dev_close(struct audio_device *audio, uint32_t oflag);
size_t audio_dev_read(struct audio_device *audio, void *buffer, size_t size);
size_t audio_dev_write(struct audio_device *audio, const void *buffer, size_t size);
int audio_dev_configure(struct audio_device *audio, struct audio_caps *caps);
int audio_dev_register_record_callback(struct audio_device *audio, int (*callback)(struct audio_device *audio));
int audio_dev_record_set_param(struct audio_device *audio, uint8_t *buf, int size);
int audio_dev_record_start(struct audio_device *audio);
int audio_dev_record_stop(struct audio_device *audio);


/* Device Control Commands */
#define CODEC_CMD_RESET             0
#define CODEC_CMD_SET_VOLUME        1
#define CODEC_CMD_GET_VOLUME        2
#define CODEC_CMD_SAMPLERATE        3
#define CODEC_CMD_EQ                4
#define CODEC_CMD_3D                5

#define CODEC_VOLUME_MAX            (63)

#endif /* __AUDIO_H__ */
