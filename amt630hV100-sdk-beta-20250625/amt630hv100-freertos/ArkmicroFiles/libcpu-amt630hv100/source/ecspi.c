#include <string.h>
#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"


#ifdef SPI1_SUPPORT

#define ARK_ECSPI_CTRL					0x08
#define ARK_ECSPI_CTRL_ENABLE			(1 <<  0)
#define ARK_ECSPI_CTRL_XCH				(1 <<  2)
#define ARK_ECSPI_CTRL_SMC				(1 << 3)
#define ARK_ECSPI_CTRL_MODE_MASK		(0xf << 4)
#define ARK_ECSPI_CTRL_DRCTL(drctl)		((drctl) << 16)
#define ARK_ECSPI_CTRL_POSTDIV_OFFSET	8
#define ARK_ECSPI_CTRL_PREDIV_OFFSET	12
#define ARK_ECSPI_CTRL_CS(cs)			((cs) << 18)
#define ARK_ECSPI_CTRL_BL_OFFSET		20
#define ARK_ECSPI_CTRL_BL_MASK			(0xfff << 20)
#define ARK_ECSPI_CONFIG				0x0c
#define ARK_ECSPI_CONFIG_SCLKPHA(cs)	(1 << ((cs) +  0))
#define ARK_ECSPI_CONFIG_SCLKPOL(cs)	(1 << ((cs) +  4))
#define ARK_ECSPI_CONFIG_SBBCTRL(cs)	(1 << ((cs) +  8))
#define ARK_ECSPI_CONFIG_SSBPOL(cs)		(1 << ((cs) + 12))
#define ARK_ECSPI_CONFIG_SCLKCTL(cs)	(1 << ((cs) + 20))
#define ARK_ECSPI_INT					0x10
#define ARK_ECSPI_INT_TEEN				(1 <<  0)
#define ARK_ECSPI_INT_RREN				(1 <<  3)
#define ARK_ECSPI_INT_TCEN				(1 <<  7)
#define ARK_ECSPI_DMA					0x14
#define ARK_ECSPI_DMA_TX_WML(wml)		((wml) & 0x3f)
#define ARK_ECSPI_DMA_RX_WML(wml)		((((wml) & 0x3f) - 1) << 16)
#define ARK_ECSPI_DMA_RXT_WML(wml)		(((wml) & 0x3f) << 24)
#define ARK_ECSPI_DMA_TEDEN				(1 << 7)
#define ARK_ECSPI_DMA_RXDEN				(1 << 23)
#define ARK_ECSPI_DMA_RXTDEN			(1UL << 31)
#define ARK_ECSPI_STAT					0x18
#define ARK_ECSPI_STAT_REN				(1 << 8)
#define ARK_ECSPI_STAT_RR				(1 << 3)
#define ARK_ECSPI_TESTREG				0x20
#define ARK_ECSPI_TESTREG_LBC			BIT(31)


#define SPI1_CS0_GPIO					23
#define USE_DMA_MIN_THRESHOLD			32
#define USE_DMA_MAX_THRESHOLD			512
#define MALLOC_DMA_MEM_SIZE				0x1000
#define ARK_ECSPI_RXDATA				0x50
#define ARK_ECSPI_TXDATA				0x460

/* generic defines to abstract from the different register layouts */
#define ARK_INT_RR						(1 << 0) /* Receive data ready interrupt */
#define ARK_INT_TE						(1 << 1) /* Transmit FIFO empty interrupt */
#define ARK_INT_TC						(1 << 2) /* Transfer Completed interrupt */

/* The maximum  bytes that a sdma BD can transfer.*/
#define MAX_SDMA_BD_BYTES				(1 << 15)
#define ARK_ECSPI_CTRL_MAX_BURST		512

struct ark_ecspi_data;
struct ark_spi_devtype_data {
	void (*intctrl)(struct ark_ecspi_data *, int);
	int (*config)(struct ark_ecspi_data *);
	void (*trigger)(struct ark_ecspi_data *);
	int (*rx_available)(struct ark_ecspi_data *);
	void (*reset)(struct ark_ecspi_data *);
	unsigned int fifo_size;
	bool has_dmamode;
};

struct ark_ecspi_data {
	struct spi_slave slave;
	QueueHandle_t xfer_done;
	unsigned int base;
	unsigned int irq;
	unsigned int spi_clk;
	unsigned int spi_bus_clk;

	unsigned int speed_hz;
	unsigned int bits_per_word;
	unsigned int spi_drctl;

	unsigned int count, remainder;
	void (*tx)(struct ark_ecspi_data *);
	void (*rx)(struct ark_ecspi_data *);
	unsigned char *rx_buf;
	const unsigned char *tx_buf;
	unsigned int txfifo; /* number of words pushed in tx FIFO */
	unsigned int read_u32;
	unsigned int word_mask;
	unsigned int cs_gpio;
	bool is_arke;

	/* DMA */
	bool usedma;
	u32 wml;
	QueueHandle_t dma_rx_completion;
	QueueHandle_t dma_tx_completion;
	struct spi_message dma_message;
	struct spi_message pio_message;
    struct dma_chan *dma_tx;
    struct dma_chan *dma_rx;
	char *rx_dummy_buffer;
	char *tx_dummy_buffer;

	const struct ark_spi_devtype_data *devtype_data;
};

static void ark_spi_buf_rx_u8(struct ark_ecspi_data *aspi)
{
	unsigned int val = readl(aspi->base + ARK_ECSPI_RXDATA);

	if (aspi->rx_buf) {
		if (aspi->is_arke)
			*(u8*)aspi->rx_buf = val & 0xff;
		else
			*(u8*)aspi->rx_buf = (val >> 24) & 0xff;
		aspi->rx_buf += 1;
	}
}

static void ark_spi_buf_rx_u16(struct ark_ecspi_data *aspi)
{
	unsigned int val = readl(aspi->base + ARK_ECSPI_RXDATA);

	if (aspi->rx_buf) {
		if (aspi->is_arke)
			*(u16*)aspi->rx_buf = val & 0xffff;
		else
			*(u16*)aspi->rx_buf = (val >> 16) & 0xffff;
		aspi->rx_buf += 2;
	}
}

static void ark_spi_buf_tx_u8(struct ark_ecspi_data *aspi)
{
	u32 val = 0;

	if (aspi->tx_buf) {
		if (aspi->is_arke)
			val = *(u8 *)aspi->tx_buf;
		else
			val = *(u8 *)aspi->tx_buf << 24;
		aspi->tx_buf += 1;
	}

	aspi->count -= 1;
	writel(val, aspi->base + ARK_ECSPI_TXDATA);
}

static void ark_spi_buf_tx_u16(struct ark_ecspi_data *aspi)
{
	u32 val = 0;

	if (aspi->tx_buf) {
		if (aspi->is_arke)
			val = *(u16 *)aspi->tx_buf;
		else
			val = *(u16 *)aspi->tx_buf << 16;
		aspi->tx_buf += 2;
	}

	aspi->count -=2;
	writel(val, aspi->base + ARK_ECSPI_TXDATA);
}

static int ark_spi_bytes_per_word(const int bits_per_word)
{
	return DIV_ROUND_UP(bits_per_word, BITS_PER_BYTE);
}

static void ark_spi_buf_rx_swap_u32(struct ark_ecspi_data *aspi)
{
	unsigned int val = readl(aspi->base + ARK_ECSPI_RXDATA);

	if (aspi->rx_buf) {
		val &= aspi->word_mask;
		*(u32 *)aspi->rx_buf = val;
		aspi->rx_buf += sizeof(u32);
	}
}

static void ark_spi_buf_rx_swap(struct ark_ecspi_data *aspi)
{
	unsigned int bytes_per_word;

	bytes_per_word = ark_spi_bytes_per_word(aspi->bits_per_word);
	if (aspi->read_u32) {
		ark_spi_buf_rx_swap_u32(aspi);
		return;
	}

	if (bytes_per_word == 1)
		ark_spi_buf_rx_u8(aspi);
	else if (bytes_per_word == 2)
		ark_spi_buf_rx_u16(aspi);
}

static void ark_spi_buf_tx_swap_u32(struct ark_ecspi_data *aspi)
{
	u32 val = 0;

	if (aspi->tx_buf) {
		val = *(u32 *)aspi->tx_buf;
		val &= aspi->word_mask;
		aspi->tx_buf += sizeof(u32);
	}
	aspi->count -= sizeof(u32);
	writel(val, aspi->base + ARK_ECSPI_TXDATA);
}

static void ark_spi_buf_tx_swap(struct ark_ecspi_data *aspi)
{
	u32 ctrl, val;
	unsigned int bytes_per_word;

	if (aspi->count == aspi->remainder) {
		ctrl = readl(aspi->base + ARK_ECSPI_CTRL);
		ctrl &= ~ARK_ECSPI_CTRL_BL_MASK;
		if (aspi->count > ARK_ECSPI_CTRL_MAX_BURST) {
			aspi->remainder = aspi->count %
					     ARK_ECSPI_CTRL_MAX_BURST;
			val = ARK_ECSPI_CTRL_MAX_BURST * 8 - 1;
		} else if (aspi->count >= sizeof(u32)) {
			aspi->remainder = aspi->count % sizeof(u32);
			val = (aspi->count - aspi->remainder) * 8 - 1;
		} else {
			aspi->remainder = 0;
			val = aspi->bits_per_word - 1;
			aspi->read_u32 = 0;
		}
		ctrl |= (val << ARK_ECSPI_CTRL_BL_OFFSET);
		writel(ctrl, aspi->base + ARK_ECSPI_CTRL);
	}

	if (aspi->count >= sizeof(u32)) {
		ark_spi_buf_tx_swap_u32(aspi);
		return;
	}

	bytes_per_word = ark_spi_bytes_per_word(aspi->bits_per_word);
	if (bytes_per_word == 1)
		ark_spi_buf_tx_u8(aspi);
	else if (bytes_per_word == 2)
		ark_spi_buf_tx_u16(aspi);
}

/* ARK eCSPI */
static unsigned int ark_ecspi_clkdiv(struct ark_ecspi_data *aspi,
				      unsigned int fspi, unsigned int *fres)
{
	/*
	 * there are two 4-bit dividers, the pre-divider divides by
	 * $pre, the post-divider by 2^$post
	 */
	unsigned int pre, post;
	unsigned int fin = aspi->spi_clk;

	if (fspi > fin)
		return 0;

	post = fls(fin) - fls(fspi);
	if (fin > fspi << post)
		post++;

	/* now we have: (fin <= fspi << post) with post being minimal */

	post = configMAX(4U, post) - 4;
	if (post > 0xf) {
		TRACE_ERROR("cannot set clock freq: %u (base freq: %u)\n",
				fspi, fin);
		return 0xff;
	}

	pre = DIV_ROUND_UP(fin, fspi << post) - 1;

	TRACE_DEBUG("%s: fin: %u, fspi: %u, post: %u, pre: %u\n",
			__func__, fin, fspi, post, pre);

	/* Resulting frequency for the SCLK line. */
	*fres = (fin / (pre + 1)) >> post;

	return (pre << ARK_ECSPI_CTRL_PREDIV_OFFSET) |
		(post << ARK_ECSPI_CTRL_POSTDIV_OFFSET);
}

static void ark_ecspi_intctrl(struct ark_ecspi_data *aspi, int enable)
{
	unsigned val = 0;

	if (enable & ARK_INT_TE)
		val |= ARK_ECSPI_INT_TEEN;

	if (enable & ARK_INT_RR)
		val |= ARK_ECSPI_INT_RREN;

	if (enable & ARK_INT_TC)
		val |= ARK_ECSPI_INT_TCEN;

	writel(val, aspi->base + ARK_ECSPI_INT);
}

static void ark_ecspi_trigger(struct ark_ecspi_data *aspi)
{
	u32 reg;

	reg = readl(aspi->base + ARK_ECSPI_CTRL);
	reg |= ARK_ECSPI_CTRL_XCH;
	writel(reg, aspi->base + ARK_ECSPI_CTRL);
}

static int ark_ecspi_config(struct ark_ecspi_data *aspi)
{
	unsigned int ctrl = ARK_ECSPI_CTRL_ENABLE;
	unsigned int clk = aspi->speed_hz, delay, reg;
	unsigned int cfg = readl(aspi->base + ARK_ECSPI_CONFIG);
	unsigned int chip_select = aspi->slave.cs;

	/*
	 * The hardware seems to have a race condition when changing modes. The
	 * current assumption is that the selection of the channel arrives
	 * earlier in the hardware than the mode bits when they are written at
	 * the same time.
	 * So set master mode for all channels as we do not support slave mode.
	 */
#ifndef SPI1_SLAVE_MODE
	ctrl |= ARK_ECSPI_CTRL_MODE_MASK;
#endif

	/*
	 * Enable SPI_RDY handling (falling edge/level triggered).
	 */
	if (aspi->slave.mode & SPI_READY)
		ctrl |= ARK_ECSPI_CTRL_DRCTL(aspi->spi_drctl);

	/* set clock speed */
	ctrl |= ark_ecspi_clkdiv(aspi, aspi->speed_hz, &clk);
	aspi->spi_bus_clk = clk;

	/* set chip select to use */
	ctrl |= ARK_ECSPI_CTRL_CS(chip_select);

	if (aspi->usedma) {
#ifdef SPI1_SLAVE_MODE
		if (aspi->dma_message.send_buf)
			ctrl |= (aspi->dma_message.length * 8 - 1) << ARK_ECSPI_CTRL_BL_OFFSET;
		else
			ctrl |= (32 - 1) << ARK_ECSPI_CTRL_BL_OFFSET;
#else
		ctrl |= (32 - 1) << ARK_ECSPI_CTRL_BL_OFFSET;
#endif
	} else {
		ctrl |= (aspi->bits_per_word - 1) << ARK_ECSPI_CTRL_BL_OFFSET;
	}

	cfg |= ARK_ECSPI_CONFIG_SBBCTRL(chip_select);

	if (aspi->slave.mode & SPI_CPHA)
		cfg |= ARK_ECSPI_CONFIG_SCLKPHA(chip_select);
	else
		cfg &= ~ARK_ECSPI_CONFIG_SCLKPHA(chip_select);

	if (aspi->slave.mode & SPI_CPOL) {
		cfg |= ARK_ECSPI_CONFIG_SCLKPOL(chip_select);
		cfg |= ARK_ECSPI_CONFIG_SCLKCTL(chip_select);
	} else {
		cfg &= ~ARK_ECSPI_CONFIG_SCLKPOL(chip_select);
		cfg &= ~ARK_ECSPI_CONFIG_SCLKCTL(chip_select);
	}
	if (aspi->slave.mode & SPI_CS_HIGH)
		cfg |= ARK_ECSPI_CONFIG_SSBPOL(chip_select);
	else
		cfg &= ~ARK_ECSPI_CONFIG_SSBPOL(chip_select);

	if (aspi->usedma) {
		ctrl |= ARK_ECSPI_CTRL_SMC;
	}

	/* CTRL register always go first to bring out controller from reset */
	writel(ctrl, aspi->base + ARK_ECSPI_CTRL);

	reg = readl(aspi->base + ARK_ECSPI_TESTREG);
	if (aspi->slave.mode & SPI_LOOP)
		reg |= ARK_ECSPI_TESTREG_LBC;
	else
		reg &= ~ARK_ECSPI_TESTREG_LBC;
	writel(reg, aspi->base + ARK_ECSPI_TESTREG);

	writel(cfg, aspi->base + ARK_ECSPI_CONFIG);

	/*
	 * Wait until the changes in the configuration register CONFIGREG
	 * propagate into the hardware. It takes exactly one tick of the
	 * SCLK clock, but we will wait two SCLK clock just to be sure. The
	 * effect of the delay it takes for the hardware to apply changes
	 * is noticable if the SCLK clock run very slow. In such a case, if
	 * the polarity of SCLK should be inverted, the GPIO ChipSelect might
	 * be asserted before the SCLK polarity changes, which would disrupt
	 * the SPI communication as the device on the other end would consider
	 * the change of SCLK polarity as a clock tick already.
	 */
	delay = (2 * 1000000) / clk;
	if (delay < 10)	/* SCLK is faster than 100 kHz */
		udelay(delay);
	else			/* SCLK is _very_ slow */
		udelay(delay + 10);

	/* enable rx fifo */
	writel(ARK_ECSPI_STAT_REN, aspi->base + ARK_ECSPI_STAT);

	/*
	 * Configure the DMA register: setup the watermark
	 * and enable DMA request.
	 */
	if (aspi->usedma) {
		writel(ARK_ECSPI_DMA_RX_WML(aspi->wml) |
			ARK_ECSPI_DMA_TX_WML(aspi->wml) |
			ARK_ECSPI_DMA_RXT_WML(aspi->wml) |
			ARK_ECSPI_DMA_TEDEN | ARK_ECSPI_DMA_RXDEN |
			ARK_ECSPI_DMA_RXTDEN, aspi->base + ARK_ECSPI_DMA);
	} else {
		writel(0, aspi->base + ARK_ECSPI_DMA);
#ifdef SPI1_SLAVE_MODE
		aspi->devtype_data->intctrl(aspi, ARK_INT_RR/* | ARK_INT_TC*/);
#endif
	}

	return 0;
}

static int ark_ecspi_rx_available(struct ark_ecspi_data *aspi)
{
	return readl(aspi->base + ARK_ECSPI_STAT) & ARK_ECSPI_STAT_RR;
}

static void ark_ecspi_reset(struct ark_ecspi_data *aspi)
{
	/* drain receive buffer */
	while (ark_ecspi_rx_available(aspi))
		readl(aspi->base + ARK_ECSPI_RXDATA);
}

static struct ark_spi_devtype_data ark_ecspi_devtype_data = {
	.intctrl = ark_ecspi_intctrl,
	.config = ark_ecspi_config,
	.trigger = ark_ecspi_trigger,
	.rx_available = ark_ecspi_rx_available,
	.reset = ark_ecspi_reset,
	.fifo_size = 64,
#ifdef SPI1_DMA_SUPPORT
	.has_dmamode = true,
#else
	.has_dmamode = false,
#endif
};

static void ark_spi_chipselect(struct ark_ecspi_data *aspi, int is_active)
{
	int dev_is_lowactive = !(aspi->slave.mode & SPI_CS_HIGH);

	if (aspi->slave.mode & SPI_NO_CS)
		return;

	gpio_direction_output(aspi->cs_gpio, is_active ^ dev_is_lowactive);
}

static void ark_spi_push(struct ark_ecspi_data *aspi)
{
	while (aspi->txfifo < aspi->devtype_data->fifo_size) {
		if (!aspi->count)
			break;
		if (aspi->txfifo && (aspi->count == aspi->remainder))
			break;
		aspi->tx(aspi);
		aspi->txfifo++;
	}
	aspi->devtype_data->trigger(aspi);
}

static void ark_spi_isr(void *param)
{
	struct ark_ecspi_data *aspi = param;

	while (aspi->devtype_data->rx_available(aspi)) {
		aspi->rx(aspi);
		aspi->txfifo--;
	}

	if (aspi->count) {
		ark_spi_push(aspi);
		return;
	}

	if (aspi->txfifo) {
		/* No data left to push, but still waiting for rx data,
		 * enable receive data available interrupt.
		 */
		aspi->devtype_data->intctrl(
				aspi, ARK_INT_RR);
		return;
	}

	aspi->devtype_data->intctrl(aspi, 0);
	xQueueSendFromISR(aspi->xfer_done, NULL, 0);
}

static int ark_spi_setupxfer(struct ark_ecspi_data *aspi,
					struct spi_configuration *configuration)
{
	u32 mask;

	if (!configuration)
		return 0;

	aspi->slave.mode = configuration->mode;
	aspi->bits_per_word = configuration->data_width;
	aspi->speed_hz  = configuration->max_hz;

	/* Initialize the functions for transfer */
	aspi->remainder = 0;
	aspi->read_u32  = 1;

	mask = (1 << aspi->bits_per_word) - 1;
	aspi->rx = ark_spi_buf_rx_swap;
	aspi->tx = ark_spi_buf_tx_swap;

	if (aspi->bits_per_word <= 8)
		aspi->word_mask = mask << 24 | mask << 16
						| mask << 8 | mask;
	else if (aspi->bits_per_word <= 16)
		aspi->word_mask = mask << 16 | mask;
	else
		aspi->word_mask = mask;

	//aspi->devtype_data->config(aspi);

	return 0;
}

static int ark_spi_calculate_timeout(struct ark_ecspi_data *aspi, int size)
{
	unsigned long timeout = 0;

	/* Time with actual data transfer and CS change delay related to HW */
	timeout = (8 + 4) * size / aspi->spi_bus_clk;

	/* Add extra second for scheduler related activities */
	timeout += 1;

	/* Double calculated timeout */
	return pdMS_TO_TICKS(2 * timeout * MSEC_PER_SEC);
}

static bool ark_spi_can_dma(struct ark_ecspi_data *aspi, struct spi_message *transfer)
{
	const u32 mszs[] = {1, 4, 8, 16};
	int idx = ARRAY_SIZE(mszs) - 1;
	struct spi_message *dma_xfer = &aspi->dma_message;
	struct spi_message *pio_xfer = &aspi->pio_message;
	int len, remainder;

	if (!aspi->dma_rx)
		return false;

	pio_xfer->length = 0;
	memcpy(dma_xfer, transfer, sizeof(struct spi_message));
	remainder = transfer->length & 3;
	len = transfer->length - remainder;

	if (len < USE_DMA_MIN_THRESHOLD)
		return false;
	else if (len > USE_DMA_MAX_THRESHOLD)
		return false;

	if ((u32)transfer->send_buf & 3 || (u32)transfer->recv_buf & 3)
		return false;

	if (remainder) {
		dma_xfer->length = len;

		memcpy(pio_xfer, transfer, sizeof(struct spi_message));
		pio_xfer->length = remainder;
		if (pio_xfer->send_buf)
			pio_xfer->send_buf = (u8*)pio_xfer->send_buf + len;
		if (pio_xfer->recv_buf)
			pio_xfer->recv_buf = (u8*)pio_xfer->recv_buf + len;
	}

	/* dw dma busrt should be 16,8,4,1 */
	for (; idx >= 0; idx--) {
		if (!(len % (mszs[idx] * 4)))
			break;
	}

	aspi->wml = mszs[idx];

	return true;
}

static void ark_spi_sdma_exit(struct ark_ecspi_data *aspi)
{
	if (aspi->dma_rx) {
		dma_release_channel(aspi->dma_rx);
		aspi->dma_rx = NULL;
	}

	if (aspi->dma_tx) {
		dma_release_channel(aspi->dma_tx);
		aspi->dma_tx = NULL;
	}

	if (aspi->rx_dummy_buffer) {
		vPortFree(aspi->rx_dummy_buffer);
		aspi->rx_dummy_buffer = NULL;
	}

	if (aspi->tx_dummy_buffer) {
		vPortFree(aspi->tx_dummy_buffer);
		aspi->tx_dummy_buffer = NULL;
	}
}

static int ark_spi_sdma_init(struct ark_ecspi_data *aspi)
{
	int ret;

	aspi->wml = aspi->devtype_data->fifo_size / 2;

	/* Prepare for TX DMA: */
	aspi->dma_tx = dma_request_channel(SPI1_TX_DMA_CH);
	if (IS_ERR(aspi->dma_tx)) {
		ret = PTR_ERR(aspi->dma_tx);
		TRACE_DEBUG("can't get the TX DMA channel, error %d!\n", ret);
		aspi->dma_tx = NULL;
		goto err;
	}

	/* Prepare for RX : */
	aspi->dma_rx = dma_request_channel(SPI1_RX_DMA_CH);
	if (IS_ERR(aspi->dma_rx)) {
		ret = PTR_ERR(aspi->dma_rx);
		TRACE_DEBUG("can't get the RX DMA channel, error %d\n", ret);
		aspi->dma_rx = NULL;
		goto err;
	}

	aspi->rx_dummy_buffer = pvPortMalloc(MALLOC_DMA_MEM_SIZE);
	if (!aspi->rx_dummy_buffer) {
		ret = -ENOMEM;
		goto err;
	}

	aspi->tx_dummy_buffer = pvPortMalloc(MALLOC_DMA_MEM_SIZE);
	if (!aspi->tx_dummy_buffer) {
		ret = -ENOMEM;
		goto err;
	}

	aspi->dma_rx_completion = xQueueCreate(1, 0);
	aspi->dma_tx_completion = xQueueCreate(1, 0);

	return 0;
err:
	ark_spi_sdma_exit(aspi);
	return ret;
}

static void ark_spi_dma_rx_callback(void *cookie, unsigned mask)
{
	struct ark_ecspi_data *aspi = (struct ark_ecspi_data *)cookie;
	struct spi_message *dma_message = &aspi->dma_message;

	ark_ecspi_reset(aspi);
	/* Invalidate cache after read */
	/* rx_dummy_buffer shoule align to CACHE_LINE_SIZE */
	CP15_invalidate_dcache_for_dma((u32)aspi->rx_dummy_buffer,
		(u32)aspi->rx_dummy_buffer + dma_message->length);
	if (dma_message->recv_buf)
		memcpy(dma_message->recv_buf, aspi->rx_dummy_buffer, dma_message->length);
	xQueueSendFromISR(aspi->dma_rx_completion, NULL, 0);
}

static void ark_spi_dma_tx_callback(void *cookie, unsigned int mask)
{
	struct ark_ecspi_data *aspi = (struct ark_ecspi_data *)cookie;

	xQueueSendFromISR(aspi->dma_tx_completion, NULL, 0);
}

static int ark_spi_dma_transfer(struct ark_ecspi_data *aspi, struct spi_message *message)
{
	struct dma_config rx = {0}, tx = {0};
	unsigned long transfer_timeout;

	rx.direction = DMA_DEV_TO_MEM;
	rx.src_id = SPI1_RX;
	rx.src_addr = aspi->base + ARK_ECSPI_RXDATA;
	rx.dst_addr = (unsigned int)aspi->rx_dummy_buffer;
	rx.dst_addr_width = rx.src_addr_width = DMA_BUSWIDTH_4_BYTES;
	rx.src_maxburst = rx.dst_maxburst = aspi->wml;
	rx.transfer_size = message->length;
	rx.dst_ahbmaster_sel = 0;
	rx.src_ahbmaster_sel = 0;
	dma_config_channel(aspi->dma_rx, &rx);
	dma_register_complete_callback(aspi->dma_rx, ark_spi_dma_rx_callback, aspi);

	tx.direction = DMA_MEM_TO_DEV;
	tx.dst_id = SPI1_TX;
	tx.src_addr = (unsigned int)aspi->tx_dummy_buffer;
	tx.dst_addr = aspi->base + ARK_ECSPI_TXDATA;
	tx.dst_addr_width = tx.src_addr_width = DMA_BUSWIDTH_4_BYTES;
	tx.src_maxburst = tx.dst_maxburst = aspi->wml;
	tx.transfer_size = message->length;
	tx.dst_ahbmaster_sel = 0;
	tx.src_ahbmaster_sel = 0;
	dma_config_channel(aspi->dma_tx, &tx);
	dma_register_complete_callback(aspi->dma_tx, ark_spi_dma_tx_callback, aspi);

	xQueueReset(aspi->dma_rx_completion);
	/* Flush cache before read */
	CP15_flush_dcache_for_dma((u32)aspi->rx_dummy_buffer,
		(u32)aspi->rx_dummy_buffer + message->length);
	dma_start_channel(aspi->dma_rx);

	memset(aspi->tx_dummy_buffer, 0xff, message->length);
	if (message->send_buf)
		memcpy(aspi->tx_dummy_buffer, message->send_buf, message->length);

	xQueueReset(aspi->dma_tx_completion);
	/* Flush cache before write */
	CP15_flush_dcache_for_dma((u32)aspi->tx_dummy_buffer,
		(u32)aspi->tx_dummy_buffer + message->length);
	dma_start_channel(aspi->dma_tx);

#ifdef SPI1_SLAVE_MODE
	//add IO SYNC here

	transfer_timeout = portMAX_DELAY;
#else
	transfer_timeout = ark_spi_calculate_timeout(aspi, message->length);
#endif

	/* Wait SDMA to finish the data transfer.*/
	if (xQueueReceive(aspi->dma_tx_completion, NULL,
			transfer_timeout) != pdTRUE) {
		printf("I/O Error in DMA TX\n");
		dma_stop_channel(aspi->dma_tx);
		dma_stop_channel(aspi->dma_rx);
		return -ETIMEDOUT;
	}

	if (xQueueReceive(aspi->dma_rx_completion, NULL,
			transfer_timeout) != pdTRUE) {
		printf("I/O Error in DMA RX\n");
		aspi->devtype_data->reset(aspi);
		dma_stop_channel(aspi->dma_rx);
		return -ETIMEDOUT;
	}

	return message->length;
}

static int ark_spi_pio_xfer(struct ark_ecspi_data *aspi, struct spi_message *message)
{
	unsigned long transfer_timeout;
	int ret;
	void *tx_dummy_buf = NULL;
	void *rx_dummy_buf = NULL;

	if ((unsigned int)message->send_buf & 3) {
		tx_dummy_buf = pvPortMalloc(message->length);
		if (!tx_dummy_buf) return -ENOMEM;
		aspi->tx_buf = tx_dummy_buf;
		memcpy(tx_dummy_buf, message->send_buf, message->length);
	} else aspi->tx_buf = message->send_buf;
	if ((unsigned int)message->recv_buf & 3) {
		rx_dummy_buf = pvPortMalloc(message->length);
		if (!rx_dummy_buf) return -ENOMEM;
		aspi->rx_buf = rx_dummy_buf;
	} else aspi->rx_buf = message->recv_buf;

	aspi->remainder = aspi->count = message->length;
	aspi->read_u32 = 1;
	aspi->txfifo = 0;
	xQueueReset(aspi->xfer_done);

	ark_spi_push(aspi);

	aspi->devtype_data->intctrl(aspi, ARK_INT_TE);

	transfer_timeout = ark_spi_calculate_timeout(aspi, message->length);
	if (xQueueReceive(aspi->xfer_done, NULL, transfer_timeout) != pdTRUE) {
		TRACE_ERROR("I/O Error in PIO\n");
		aspi->devtype_data->reset(aspi);
		if (message->cs_release)
			ark_spi_chipselect(aspi, 0);
		ret = -ETIMEDOUT;
	} else {
		if (rx_dummy_buf)
			memcpy(message->recv_buf, rx_dummy_buf, message->length);
		ret = message->length;
	}

	if (rx_dummy_buf) vPortFree(rx_dummy_buf);
	if (tx_dummy_buf) vPortFree(tx_dummy_buf);

	return ret;
}

static int ecspi_configure(struct spi_slave *slave, struct spi_configuration *configuration)
{
	struct ark_ecspi_data *aspi = (struct ark_ecspi_data *)slave;

	return ark_spi_setupxfer(aspi, configuration);
}

static int ecspi_xfer(struct spi_slave *slave, struct spi_message *message)
{
	struct ark_ecspi_data *aspi = (struct ark_ecspi_data *)slave;
	int ret = 0;

#ifndef SPI1_SLAVE_MODE
	if (message->cs_take)
		ark_spi_chipselect(aspi, 1);
#endif

	if (ark_spi_can_dma(aspi, message))
		aspi->usedma = 1;
	else
		aspi->usedma = 0;

	aspi->devtype_data->config(aspi);

	if (aspi->usedma) {
		if ((ret = ark_spi_dma_transfer(aspi, &aspi->dma_message)) < 0)
			goto end;
		if (aspi->pio_message.length > 0 &&
			(ret = ark_spi_pio_xfer(aspi, &aspi->pio_message)) < 0)
			goto end;
		ret = message->length;
		goto end;
	}

	ret = ark_spi_pio_xfer(aspi, message);
end:
#ifndef SPI1_SLAVE_MODE
	if (message->cs_release)
		ark_spi_chipselect(aspi, 0);
#endif

	return ret;
};

static int ark_ecspi_probe(struct ark_ecspi_data *aspi, char *spi_bus_name)
{
	int ret;

	aspi->devtype_data = &ark_ecspi_devtype_data;

	aspi->xfer_done = xQueueCreate(1, 0);
	request_irq(aspi->irq, 0, ark_spi_isr, aspi);

	if (aspi->devtype_data->has_dmamode) {
		ret = ark_spi_sdma_init(aspi);
		if (ret < 0)
			TRACE_ERROR("dma setup error %d, use pio\n", ret);
	}

	aspi->devtype_data->reset(aspi);

	aspi->devtype_data->intctrl(aspi, 0);

	strncpy(aspi->slave.name, spi_bus_name, 16);
	spi_add_slave(&aspi->slave);

	return 0;
}

int ecspi_init(void)
{
    struct ark_ecspi_data *aspi1 = pvPortMalloc(sizeof(struct ark_ecspi_data));
	if (!aspi1)
		return -ENOMEM;
	memset(aspi1, 0, sizeof(struct ark_ecspi_data));

	sys_soft_reset(softreset_ssp1);

	aspi1->base = REGS_SPI1_BASE;
	aspi1->irq = SPI1_IRQn;
	aspi1->spi_clk = ulClkGetRate(CLK_SPI1);
	aspi1->cs_gpio = SPI1_CS0_GPIO;
	aspi1->slave.mode = SPI_MODE_0;
	aspi1->slave.cs = 0;
	aspi1->slave.xfer = ecspi_xfer;
	aspi1->slave.configure = ecspi_configure;

	ark_ecspi_probe(aspi1, "spi1");

    return 0;
}
#endif
