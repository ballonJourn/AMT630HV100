#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "pinctrl.h"
#include "os_adapt.h"

#include <string.h>

#define SPI0_CS0_GPIO	32
#define SPI0_IO0_GPIO	34

/* Register offsets */
#define DW_SPI_CTRL0			0x00
#define DW_SPI_CTRL1			0x04
#define DW_SPI_SSIENR			0x08
#define DW_SPI_MWCR			0x0c
#define DW_SPI_SER			0x10
#define DW_SPI_BAUDR			0x14
#define DW_SPI_TXFLTR			0x18
#define DW_SPI_RXFLTR			0x1c
#define DW_SPI_TXFLR			0x20
#define DW_SPI_RXFLR			0x24
#define DW_SPI_SR			0x28
#define DW_SPI_IMR			0x2c
#define DW_SPI_ISR			0x30
#define DW_SPI_RISR			0x34
#define DW_SPI_TXOICR			0x38
#define DW_SPI_RXOICR			0x3c
#define DW_SPI_RXUICR			0x40
#define DW_SPI_MSTICR			0x44
#define DW_SPI_ICR			0x48
#define DW_SPI_DMACR			0x4c
#define DW_SPI_DMATDLR			0x50
#define DW_SPI_DMARDLR			0x54
#define DW_SPI_IDR			0x58
#define DW_SPI_VERSION			0x5c
#define DW_SPI_DR			0x60
#define DW_SPI_QSPI_CTRL0	0xf4

/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET			0

#define SPI_FRF_OFFSET			4
#define SPI_FRF_SPI			0x0
#define SPI_FRF_SSP			0x1
#define SPI_FRF_MICROWIRE		0x2
#define SPI_FRF_RESV			0x3

#define SPI_MODE_OFFSET			6
#define SPI_SCPH_OFFSET			6
#define SPI_SCOL_OFFSET			7

#define SPI_TMOD_OFFSET			8
#define SPI_TMOD_MASK			(0x3 << SPI_TMOD_OFFSET)
#define	SPI_TMOD_TR			0x0		/* xmit & recv */
#define SPI_TMOD_TO			0x1		/* xmit only */
#define SPI_TMOD_RO			0x2		/* recv only */
#define SPI_TMOD_EPROMREAD		0x3		/* eeprom read mode */

#define SPI_SLVOE_OFFSET		10
#define SPI_SRL_OFFSET			11
#define SPI_CFS_OFFSET			12
#define SPI_DFS32_OFFSET		16

#define SPI_DAF_OFFSET			21
#define SPI_DAF_STANDARD		0
#define SPI_DAF_DUAL			1
#define SPI_DAF_QUAD			2

/* Bit fields in QSPI_CTRLR0 */
#define SPI_TRANS_TYPE_OFFSET	0
#define SPI_ADDR_LENGTH_OFFSET	2
#define SPI_INST_LENGTH_OFFSET	8
#define SPI_WAIT_CYCLES_OFFSET	11



/* Bit fields in SR, 7 bits */
#define SR_MASK				0x7f		/* cover 7 bits */
#define SR_BUSY				(1 << 0)
#define SR_TF_NOT_FULL			(1 << 1)
#define SR_TF_EMPT			(1 << 2)
#define SR_RF_NOT_EMPT			(1 << 3)
#define SR_RF_FULL			(1 << 4)
#define SR_TX_ERR			(1 << 5)
#define SR_DCOL				(1 << 6)

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI			(1 << 0)
#define SPI_INT_TXOI			(1 << 1)
#define SPI_INT_RXUI			(1 << 2)
#define SPI_INT_RXOI			(1 << 3)
#define SPI_INT_RXFI			(1 << 4)
#define SPI_INT_MSTI			(1 << 5)

/* Bit fields in DMACR */
#define SPI_DMA_RDMAE			(1 << 0)
#define SPI_DMA_TDMAE			(1 << 1)

/* TX RX interrupt level threshold, max can be 256 */
#define SPI_INT_THRESHOLD		32

enum dw_ssi_type {
	SSI_MOTO_SPI = 0,
	SSI_TI_SSP,
	SSI_NS_MICROWIRE,
};

struct dw_spi;
struct dw_spi_dma_ops {
	int (*dma_init)(struct dw_spi *dws);
	void (*dma_exit)(struct dw_spi *dws);
	int (*dma_setup)(struct dw_spi *dws, struct spi_message *message);
	bool (*can_dma)(struct dw_spi *dws, struct spi_message *message);
	int (*dma_transfer)(struct dw_spi *dws, struct spi_message *message);
	void (*dma_stop)(struct dw_spi *dws);
};

/* Slave spi_dev related */
struct chip_data {
	u8 cs;			/* chip select pin */
	u8 tmode;		/* TR/TO/RO/EEPROM */
	u8 type;		/* SPI/SSP/MicroWire */

	u8 poll_mode;		/* 1 means use poll mode */

	u8 enable_dma;
	u16 clk_div;		/* baud rate divider */
	u16 qspi_clk_div;
	u32 speed_hz;		/* baud rate */
	void (*cs_control)(u32 command);
};

struct dw_spi {
	struct spi_slave	slave;
	QueueHandle_t xfer_done;
	enum dw_ssi_type	type;

	void __iomem		*regs;
	struct clk     		*clk;
	unsigned long		paddr;
	int			irq;
	u32			fifo_len;	/* depth of the FIFO buffer */
	u32			max_freq;	/* max bus freq supported */

	u16			bus_num;
	u16			num_cs;		/* supported slave numbers */
	u32 		cs_gpio;

	/* Current message transfer state info */
	size_t			len;
	void			*tx;
	void			*tx_end;
	void			*rx;
	void			*rx_end;
	u32				rxlevel;
	int			dma_mapped;
	char		*rx_dummy_buffer;
	u8			n_bytes;	/* current is a 1/2 bytes op */
	u32			dma_width;
	void		(*transfer_handler)(struct dw_spi *dws);
	u32			current_freq;	/* frequency in hz */
	u32			current_qspi_freq;
	int			xfer_ret;

	/* DMA info */
	int			dma_inited;
	struct dma_chan		*txchan;
	struct dma_chan		*rxchan;
	unsigned long		dma_chan_busy;
	dma_addr_t		dma_addr; /* phy address of the Data register */
	const struct dw_spi_dma_ops *dma_ops;
	void			*dma_tx;
	void			*dma_rx;

	/* Bus interface info */
	void			*priv;
	struct chip_data *chip;
};

/*
 * Each SPI slave device to work with dw_api controller should
 * has such a structure claiming its working mode (poll or PIO/DMA),
 * which can be save in the "controller_data" member of the
 * struct spi_device.
 */
struct dw_spi_chip {
	u8 poll_mode;	/* 1 for controller polling mode */
	u8 type;	/* SPI/SSP/MicroWire */
	void (*cs_control)(u32 command);
};

static inline u32 dw_readl(struct dw_spi *dws, u32 offset)
{
	return readl((u32)dws->regs + offset);
}

static inline void dw_writel(struct dw_spi *dws, u32 offset, u32 val)
{
	writel(val, (u32)dws->regs + offset);
}

static inline u32 dw_read_io_reg(struct dw_spi *dws, u32 offset)
{
	return dw_readl(dws, offset);
}

static inline void dw_write_io_reg(struct dw_spi *dws, u32 offset, u32 val)
{
	dw_writel(dws, offset, val);
}

static inline void spi_enable_chip(struct dw_spi *dws, int enable)
{
	dw_writel(dws, DW_SPI_SSIENR, (enable ? 1 : 0));
}

static inline void spi_set_clk(struct dw_spi *dws, u16 div)
{
	dw_writel(dws, DW_SPI_BAUDR, div);
}

/* Disable IRQ bits */
static inline void spi_mask_intr(struct dw_spi *dws, u32 mask)
{
	u32 new_mask;

	new_mask = dw_readl(dws, DW_SPI_IMR) & ~mask;
	dw_writel(dws, DW_SPI_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_umask_intr(struct dw_spi *dws, u32 mask)
{
	u32 new_mask;

	new_mask = dw_readl(dws, DW_SPI_IMR) | mask;
	dw_writel(dws, DW_SPI_IMR, new_mask);
}

/*
 * This does disable the SPI controller, interrupts, and re-enable the
 * controller back. Transmit and receive FIFO buffers are cleared when the
 * device is disabled.
 */
static inline void spi_reset_chip(struct dw_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_mask_intr(dws, 0xff);
	spi_enable_chip(dws, 1);
}

/* static inline void spi_shutdown_chip(struct dw_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_set_clk(dws, 0);
} */

/* Return the max entries we can fill into tx fifo */
static inline u32 tx_max(struct dw_spi *dws)
{
	u32 tx_left, tx_room, rxtx_gap;

	tx_left = ((u32)dws->tx_end - (u32)dws->tx) / dws->n_bytes;
	tx_room = dws->fifo_len - dw_readl(dws, DW_SPI_TXFLR);

	/*
	 * Another concern is about the tx/rx mismatch, we
	 * though to use (dws->fifo_len - rxflr - txflr) as
	 * one maximum value for tx, but it doesn't cover the
	 * data which is out of tx/rx fifo and inside the
	 * shift registers. So a control from sw point of
	 * view is taken.
	 */
	rxtx_gap =  (((u32)dws->rx_end - (u32)dws->rx) - ((u32)dws->tx_end - (u32)dws->tx))
			/ dws->n_bytes;

	return configMIN(tx_left, configMIN(tx_room, (u32) (dws->fifo_len - rxtx_gap)));
}

/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct dw_spi *dws)
{
	u32 rx_left = ((u32)dws->rx_end - (u32)dws->rx) / dws->n_bytes;
	return configMIN(rx_left, dw_readl(dws, DW_SPI_RXFLR));
}

static void dw_writer(struct dw_spi *dws)
{
	u32 max = tx_max(dws);
	u32 txw = 0;

	while (max--) {
		/* Set the tx word if the transfer's original "tx" is not null */
		if ((u32)dws->tx_end - dws->len) {
			if (dws->n_bytes == 1)
				txw = *(u8 *)(dws->tx);
			else if (dws->n_bytes == 2)
				txw = *(u16 *)(dws->tx);
			else
				txw = *(u32 *)(dws->tx);
		}
		dw_write_io_reg(dws, DW_SPI_DR, txw);
		dws->tx = (u8*)dws->tx + dws->n_bytes;
	}
}

static void dw_reader(struct dw_spi *dws)
{
	u32 max = rx_max(dws);
	u32 rxw;

	while (max--) {
		rxw = dw_read_io_reg(dws, DW_SPI_DR);
		/* Care rx only if the transfer's original "rx" is not null */
		if ((u32)dws->rx_end - dws->len) {
			if (dws->n_bytes == 1)
				*(u8 *)(dws->rx) = rxw;
			else if (dws->n_bytes == 2)
				*(u16 *)(dws->rx) = rxw;
			else
				*(u32 *)(dws->rx) = rxw;
		}
		dws->rx = (u8*)dws->rx + dws->n_bytes;
	}
}

static void int_error_stop(struct dw_spi *dws, const char *msg)
{
	spi_reset_chip(dws);

	dev_err(&dws->master->dev, "%s\n", msg);
}

static void interrupt_transfer(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);

	/* Error handling */
	if (irq_status & (SPI_INT_TXOI | SPI_INT_RXOI | SPI_INT_RXUI)) {
		dw_readl(dws, DW_SPI_ICR);
		int_error_stop(dws, "interrupt_transfer: fifo overrun/underrun");
		xQueueSendFromISR(dws->xfer_done, NULL, 0);
		return;
	}

	dw_reader(dws);
	if (dws->rx_end == dws->rx) {
		spi_mask_intr(dws, SPI_INT_TXEI);
		xQueueSendFromISR(dws->xfer_done, NULL, 0);
		return;
	}
	if (irq_status & SPI_INT_TXEI) {
		spi_mask_intr(dws, SPI_INT_TXEI);
		dw_writer(dws);
		/* Enable TX irq always, it will be disabled when RX finished */
		spi_umask_intr(dws, SPI_INT_TXEI);
	}

	return;
}

static void qspi_read_interrupt(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);
	//u32 rxw;
	//int i;

	/* Error handling */
	if (irq_status & (SPI_INT_RXOI | SPI_INT_RXUI)) {
		dw_readl(dws, DW_SPI_ICR);
		int_error_stop(dws, "qspi_read_interrupt: fifo overrun/underrun");
		xQueueSendFromISR(dws->xfer_done, NULL, 0);
		return;
	}

	if (irq_status & SPI_INT_RXFI) {
		/*for (i = 0; i < dws->rxlevel; i++) {
			rxw = dw_read_io_reg(dws, DW_SPI_DR);
			if (dws->n_bytes == 1)
				*(u8 *)(dws->rx) = rxw;
			else if (dws->n_bytes == 2)
				*(u16 *)(dws->rx) = rxw;
			else
				*(u32 *)(dws->rx) = rxw;
			dws->rx = (u8*)dws->rx + dws->n_bytes;
		}*/
		dw_reader(dws);
	}

	if (dws->rx_end == dws->rx) {
		xQueueSendFromISR(dws->xfer_done, NULL, 0);
		return;
	}

	return;
}

static void dma_transfer(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);

	printf("status=0x%x.\n", irq_status);
	if (!irq_status)
		return;

	dw_readl(dws, DW_SPI_ICR);
	int_error_stop(dws, "dma_transfer: fifo overrun/underrun");
	dws->xfer_ret = 1;

	xQueueSendFromISR(dws->xfer_done, NULL, 0);

	return;
}

static void dw_spi_irq(void *param)
{
	struct dw_spi *dws = param;
	u16 irq_status = dw_readl(dws, DW_SPI_ISR) & 0x3f;

	if (!irq_status)
		return;

	dws->transfer_handler(dws);
}

/* Must be called inside pump_transfers() */
static int poll_transfer(struct dw_spi *dws)
{
	do {
		dw_writer(dws);
		dw_reader(dws);
		taskYIELD();
	} while (dws->rx_end > dws->rx);

	return 0;
}

static void dw_spi_chipselect(struct dw_spi *dws, int is_active)
{
	int dev_is_lowactive = !(dws->slave.mode & SPI_CS_HIGH);

	if (dws->slave.mode & SPI_NO_CS)
		return;

	gpio_direction_output(dws->cs_gpio, is_active ^ dev_is_lowactive);
}

static int dw_spi_calculate_timeout(struct dw_spi *dws, int size)
{
	unsigned long timeout = 0;

	/* Time with actual data transfer and CS change delay related to HW */
	timeout = (8 + 4) * size / dws->current_freq;

	/* Add extra second for scheduler related activities */
	timeout += 1;

	/* Double calculated timeout */
	return pdMS_TO_TICKS(2 * timeout * MSEC_PER_SEC);
}

static int dw_spi_transfer_one(struct spi_slave *slave, struct spi_message *message)
{
	struct dw_spi *dws = (struct dw_spi *)slave;
	struct chip_data *chip = dws->chip;
	u8 imask = 0;
	u16 txlevel = 0;
	u32 cr0;
	u32 bits_per_word = 0;
	unsigned long transfer_timeout;
	int ret = 0;

	chip->tmode = SPI_TMOD_TR;

	dws->dma_mapped = 0;
	dws->tx = (void *)message->send_buf;
	dws->tx_end = (u8*)dws->tx + message->length;
	dws->rx = message->recv_buf;
	dws->rx_end = (u8*)dws->rx + message->length;
	dws->len = message->length;

	spi_enable_chip(dws, 0);

	spi_set_clk(dws, chip->clk_div);

	if (message->cs_take)
		dw_spi_chipselect(dws, 1);

	if (message->length & 1 || (u32)dws->tx & 1 || (u32)dws->rx & 1)
		bits_per_word = 8;
	else if (message->length & 3 || (u32)dws->tx & 3 || (u32)dws->rx & 3)
		bits_per_word = 16;
	else
		bits_per_word = 32;

	//printk("len=%d, bits_per_word=%d.\n", transfer->len, bits_per_word);

	if (bits_per_word == 8) {
		dws->n_bytes = 1;
		dws->dma_width = DMA_BUSWIDTH_1_BYTE;
	} else if (bits_per_word == 16) {
		dws->n_bytes = 2;
		dws->dma_width = DMA_BUSWIDTH_2_BYTES;
	} else if (bits_per_word == 32) {
		dws->n_bytes = 4;
		dws->dma_width = DMA_BUSWIDTH_4_BYTES;
	} else {
		ret = -EINVAL;
		goto end;
	}
	/* Default SPI mode is SCPOL = 0, SCPH = 0 */
	cr0 = ((bits_per_word - 1) << SPI_DFS32_OFFSET)
		| (chip->type << SPI_FRF_OFFSET)
		| ((dws->slave.mode & 3) << SPI_MODE_OFFSET)
		| (chip->tmode << SPI_TMOD_OFFSET);

	/*
	 * Adjust transfer mode if necessary. Requires platform dependent
	 * chipselect mechanism.
	 */
	if (chip->cs_control) {
		if (dws->rx && dws->tx)
			chip->tmode = SPI_TMOD_TR;
		else if (dws->rx)
			chip->tmode = SPI_TMOD_RO;
		else
			chip->tmode = SPI_TMOD_TO;

		cr0 &= ~SPI_TMOD_MASK;
		cr0 |= (chip->tmode << SPI_TMOD_OFFSET);
	}
	dw_writel(dws, DW_SPI_CTRL0, cr0);

	/* For poll mode just disable all interrupts */
	spi_mask_intr(dws, 0xff);

	/*
	 * Interrupt mode
	 * we only need set the TXEI IRQ, as TX/RX always happen syncronizely
	 */
	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_setup(dws, message);
		if (ret < 0) {
			spi_enable_chip(dws, 1);
			goto end;
		}
	} else if (!chip->poll_mode) {
		dw_writel(dws, DW_SPI_DMACR, 0);

		txlevel = configMIN(dws->fifo_len / 2, dws->len / dws->n_bytes);
		dw_writel(dws, DW_SPI_TXFLTR, txlevel);

		/* Set the interrupt mask */
		imask |= SPI_INT_TXEI | SPI_INT_TXOI |
			 SPI_INT_RXUI | SPI_INT_RXOI;
		spi_umask_intr(dws, imask);

		dws->transfer_handler = interrupt_transfer;
	}

	xQueueReset(dws->xfer_done);

	dw_writel(dws, DW_SPI_SER, BIT(dws->slave.cs));
	spi_enable_chip(dws, 1);

	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_transfer(dws, message);
		if (ret < 0)
			goto end;
	}

	if (chip->poll_mode) {
		ret = poll_transfer(dws);
		goto end;
    }

	transfer_timeout = dw_spi_calculate_timeout(dws, message->length);
	if (xQueueReceive(dws->xfer_done, NULL, transfer_timeout) != pdTRUE) {
		int_error_stop(dws, "transfer timeout");
		ret = -ETIMEDOUT;
		goto end;
	}

end:
	if (message->cs_release)
		dw_spi_chipselect(dws, 0);
	return ret;
}

static int dw_qspi_read(struct spi_slave *slave, struct qspi_message *qspi_message)
{
	struct dw_spi *dws = (struct dw_spi *)slave;
	struct chip_data *chip = dws->chip;
	struct spi_message *message = (struct spi_message *)&qspi_message->message;
	u8 imask = 0;
	u16 rxlevel = 0;
	u32 cr0, qspi_cr0, ndf;
	u32 bits_per_word = 0;
	u32 addr;
	unsigned long transfer_timeout;
	u32 xfer_len = 0;
	int ret = 0;

	chip->tmode = SPI_TMOD_RO;

	if (message->length & 1)
		bits_per_word = 8;
	else if (message->length & 3)
		bits_per_word = 16;
	else
		bits_per_word = 32;

	if (bits_per_word == 8) {
		dws->n_bytes = 1;
		dws->dma_width = DMA_BUSWIDTH_1_BYTE;
	} else if (bits_per_word == 16) {
		dws->n_bytes = 2;
		dws->dma_width = DMA_BUSWIDTH_2_BYTES;
	} else if (bits_per_word == 32) {
		dws->n_bytes = 4;
		dws->dma_width = DMA_BUSWIDTH_4_BYTES;
	} else {
		ret = -EINVAL;
		goto end;
	}

xfer_continue:
	ndf = (message->length - xfer_len) / dws->n_bytes - 1;
	if (ndf > 0xffff) ndf = 0xffff;

	dws->dma_mapped = 1;
	dws->xfer_ret = 0;
	dws->rx = (u8*)message->recv_buf + xfer_len;
	dws->len = (ndf + 1) * dws->n_bytes;
	dws->rx_end = (u8*)dws->rx + dws->len;

	spi_enable_chip(dws, 0);

	spi_set_clk(dws, chip->qspi_clk_div);

	if (message->cs_take)
		dw_spi_chipselect(dws, 1);

	/* Default SPI mode is SCPOL = 0, SCPH = 0 */
	cr0 = ((bits_per_word - 1) << SPI_DFS32_OFFSET)
		| (chip->type << SPI_FRF_OFFSET)
		| ((dws->slave.mode & 3) << SPI_MODE_OFFSET)
		| (chip->tmode << SPI_TMOD_OFFSET);

	if (qspi_message->qspi_data_lines == 4)
		cr0 |= SPI_DAF_QUAD << SPI_DAF_OFFSET;
	else if (qspi_message->qspi_data_lines == 2)
		cr0 |= SPI_DAF_DUAL << SPI_DAF_OFFSET;

	dw_writel(dws, DW_SPI_CTRL0, cr0);

	dw_writel(dws, DW_SPI_CTRL1, ndf);

	qspi_cr0 = ((qspi_message->dummy_cycles & 0xf) << SPI_WAIT_CYCLES_OFFSET) |
					(2 << SPI_INST_LENGTH_OFFSET) |
					((qspi_message->address.size >> 2) << SPI_ADDR_LENGTH_OFFSET);
	if (qspi_message->instruction.qspi_lines == 1 && qspi_message->address.qspi_lines > 1)
		qspi_cr0 |= 1;
	else if (qspi_message->instruction.qspi_lines > 1 && qspi_message->address.qspi_lines > 1)
		qspi_cr0 |= 2;
	dw_writel(dws, DW_SPI_QSPI_CTRL0, qspi_cr0);

	/* For poll mode just disable all interrupts */
	spi_mask_intr(dws, 0xff);

	/*
	 * Interrupt mode
	 * we only need set the TXEI IRQ, as TX/RX always happen syncronizely
	 */
	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_setup(dws, message);
		if (ret < 0) {
			spi_enable_chip(dws, 1);
			goto end;
		}
	} else if (!chip->poll_mode) {
		dw_writel(dws, DW_SPI_DMACR, 0);

		rxlevel = 1 << __ffs(dws->len / dws->n_bytes);
		while (rxlevel > dws->fifo_len / 2)
			rxlevel >>= 1;
		dws->rxlevel = rxlevel;
		dw_writel(dws, DW_SPI_RXFLTR, rxlevel - 1);

		/* Set the interrupt mask */
		imask |= SPI_INT_RXUI | SPI_INT_RXOI | SPI_INT_RXFI;
		spi_umask_intr(dws, imask);

		dws->transfer_handler = qspi_read_interrupt;
	}

	xQueueReset(dws->xfer_done);

	dw_writel(dws, DW_SPI_SER, BIT(dws->slave.cs));
	spi_enable_chip(dws, 1);

	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_transfer(dws, message);
		if (ret < 0)
			goto end;
	}

	addr = qspi_message->address.content + xfer_len;
	if (qspi_message->address.size == 32) {
		addr = ((addr >> 24) & 0xff) | (((addr >> 16) & 0xff) << 8) |
			(((addr >> 8) & 0xff) << 16) | ((addr & 0xff) << 24);
	} else {
		addr = ((addr >> 16) & 0xff) | (((addr >> 8) & 0xff) << 8) | ((addr & 0xff) << 16);
	}

	dw_write_io_reg(dws, DW_SPI_DR, qspi_message->instruction.content);
	dw_write_io_reg(dws, DW_SPI_DR, addr);

	transfer_timeout = dw_spi_calculate_timeout(dws, message->length);
	if (xQueueReceive(dws->xfer_done, NULL, transfer_timeout) != pdTRUE) {
		int_error_stop(dws, "transfer timeout");
		ret = -ETIMEDOUT;
		if (dws->dma_mapped)
			dma_stop_channel(dws->dma_rx);
		goto end;
	}

	if (dws->xfer_ret) {
		dws->xfer_ret = 0;
		ret = -1;
		if (dws->dma_mapped) {
			if (dws->rx_dummy_buffer) {
				vPortFree(dws->rx_dummy_buffer);
				dws->rx_dummy_buffer = NULL;
			}
			dma_stop_channel(dws->dma_rx);
		}
		goto end;
	}
	if (dws->dma_mapped) {
		/* Invalidate cache after dma read, rx and len must align to cacheline(32bytes) */
		portDISABLE_INTERRUPTS();
		if (dws->rx_dummy_buffer)
			CP15_invalidate_dcache_for_dma((uint32_t)dws->rx_dummy_buffer,
				(uint32_t)dws->rx_dummy_buffer + dws->len);
		else
			CP15_invalidate_dcache_for_dma((uint32_t)dws->rx, (uint32_t)dws->rx + dws->len);
		portENABLE_INTERRUPTS();

		if (dws->rx_dummy_buffer) {
			memcpy(dws->rx, dws->rx_dummy_buffer, dws->len);
			vPortFree(dws->rx_dummy_buffer);
			dws->rx_dummy_buffer = NULL;
		}
		dma_stop_channel(dws->dma_rx);
	}

	xfer_len += dws->len;
	if (xfer_len < message->length) {
		if (message->cs_release)
			dw_spi_chipselect(dws, 0);
		udelay(1);
		goto xfer_continue;
	}

end:
	if (message->cs_release)
		dw_spi_chipselect(dws, 0);
	return ret;
}

/* This may be called twice for each spi dev */
int dw_spi_setup(struct spi_slave *slave, struct spi_configuration *configuration)
{
	struct dw_spi *dws = (struct dw_spi *)slave;
	struct chip_data *chip;

	/* Only alloc on first setup */
	chip = dws->chip;
	if (!chip) {
		chip = pvPortMalloc(sizeof(struct chip_data));
		if (!chip)
			return -ENOMEM;
		memset(chip, 0, sizeof(struct chip_data));
		dws->chip = chip;
	}

	dws->slave.mode = configuration->mode;
	chip->clk_div = (DIV_ROUND_UP(dws->max_freq, configuration->max_hz) + 1) & 0xfffe;
	chip->qspi_clk_div = (DIV_ROUND_UP(dws->max_freq, configuration->qspi_max_hz) + 1) & 0xfffe;
	dws->current_freq = dws->max_freq / chip->clk_div;
	dws->current_qspi_freq = dws->max_freq / chip->qspi_clk_div;
	printf("spi max_freq %u, current freq %u, qspi_freq %u.\n", dws->max_freq,
		dws->current_freq, dws->current_qspi_freq);

	gpio_direction_output(dws->cs_gpio,
				!(dws->slave.mode & SPI_CS_HIGH));

	return 0;
}

/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct dw_spi *dws)
{
	spi_reset_chip(dws);

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec
	 */
	if (!dws->fifo_len) {
		u32 fifo;

		for (fifo = 1; fifo < 256; fifo++) {
			dw_writel(dws, DW_SPI_TXFLTR, fifo);
			if (fifo != dw_readl(dws, DW_SPI_TXFLTR))
				break;
		}
		dw_writel(dws, DW_SPI_TXFLTR, 0);

		dws->fifo_len = (fifo == 1) ? 0 : fifo;
		dev_dbg(dev, "Detected FIFO size: %u bytes\n", dws->fifo_len);
	}
}

static int dw_spi_add_host(struct dw_spi *dws)
{
	int ret;

	BUG_ON(dws == NULL);

	dws->type = SSI_MOTO_SPI;
	dws->dma_inited = 0;
	dws->dma_addr = (dma_addr_t)(dws->paddr + DW_SPI_DR);

	ret = request_irq(dws->irq, 0, dw_spi_irq, dws);
	if (ret < 0) {
		dev_err(dev, "can not get IRQ\n");
		goto err_exit;
	}

	/* Basic HW init */
	spi_hw_init(dws);

	if (dws->dma_ops && dws->dma_ops->dma_init) {
		ret = dws->dma_ops->dma_init(dws);
		if (ret) {
			dev_warn(dev, "DMA init failed\n");
			dws->dma_inited = 0;
			dws->dma_mapped = 1;
		}
	}

	return 0;

err_exit:
	return ret;
}

/* static void dw_spi_remove_host(struct dw_spi *dws)
{
	if (dws->dma_ops && dws->dma_ops->dma_exit)
		dws->dma_ops->dma_exit(dws);

	spi_shutdown_chip(dws);

	free_irq(dws->irq);
} */

void dwspi_jedec252_reset(void)
{
	int i;
	int si = 0;

	gpio_direction_output(SPI0_CS0_GPIO, 1);
	gpio_direction_output(SPI0_IO0_GPIO, 1);
	udelay(300);

	for (i = 0; i < 4; i++) {
		gpio_direction_output(SPI0_CS0_GPIO, 0);
		gpio_direction_output(SPI0_IO0_GPIO, si);
		si = !si;
		udelay(300);
		gpio_direction_output(SPI0_CS0_GPIO, 1);
		udelay(300);
	}
}

static void dw_spi_dma_complete_callback(void *param, unsigned int mask)
{
	struct dw_spi *dws = param;

	xQueueSendFromISR(dws->xfer_done, NULL, 0);
}

static int dw_spi_dma_init(struct dw_spi *dws)
{
	dws->dma_rx = dma_request_channel(SPI0_RX_DMA_CH);
	if (!dws->dma_rx) {
		printf("dwspi request dma channel fail.\n");
		return -1;
	}

	return 0;
}

static int dw_spi_dma_setup(struct dw_spi *dws, struct spi_message *message)
{
	dws->rxlevel = dws->fifo_len / 4;

	dw_writel(dws, DW_SPI_DMARDLR, dws->rxlevel - 1);
	dw_writel(dws, DW_SPI_DMACR, SPI_DMA_RDMAE);

	/* Set the interrupt mask */
	spi_umask_intr(dws, SPI_INT_RXUI | SPI_INT_RXOI);

	dws->transfer_handler = dma_transfer;

	return 0;
}

static int dw_spi_dma_transfer(struct dw_spi *dws, struct spi_message *message)
{
	struct dma_config cfg = {0};
	int ret;

	/* Set external dma config: burst size, burst width */
	cfg.dst_addr_width = dws->dma_width;
	cfg.src_addr_width = dws->dma_width;

	/* Match burst msize with external dma config */
	cfg.dst_maxburst = dws->rxlevel;
	cfg.src_maxburst = dws->rxlevel;
	cfg.transfer_size = dws->len;
	cfg.direction = DMA_DEV_TO_MEM;
	cfg.src_addr = REGS_SPI0_BASE + DW_SPI_DR;
	//if (((u32)dws->rx/* | dws->len*/) & (ARCH_DMA_MINALIGN - 1)) {
	if ((u32)dws->rx & (ARCH_DMA_MINALIGN - 1)) {
		dws->rx_dummy_buffer = pvPortMalloc(dws->len);
		if (!dws->rx_dummy_buffer)
			return -ENOMEM;
		cfg.dst_addr = (u32)dws->rx_dummy_buffer;
	} else {
		cfg.dst_addr = (u32)dws->rx;
	}

	/* Invalidate cache before read */
	CP15_flush_dcache_for_dma(cfg.dst_addr,
			cfg.dst_addr + dws->len);
	cfg.src_id = SPI0_RX;

	/* Read/write dst/src AHB select master */
	cfg.dst_ahbmaster_sel = 0;
	cfg.src_ahbmaster_sel = 0;

	ret = dma_config_channel(dws->dma_rx, &cfg);
	if (ret) {
		printf("dwspi failed to config dma.\n");
		return -EBUSY;
	}

	/* Set dw_spi_dma_complete_callback as callback */
	dma_register_complete_callback(dws->dma_rx, dw_spi_dma_complete_callback, dws);

	dma_start_channel(dws->dma_rx);

	return 0;
}

static const struct dw_spi_dma_ops dw_dma_ops = {
	.dma_init	= dw_spi_dma_init,
	.dma_setup	= dw_spi_dma_setup,
	.dma_transfer	= dw_spi_dma_transfer,
};

int dwspi_init(void)
{
	struct dw_spi *dws;
	int ret;

	dwspi_jedec252_reset();
	pinctrl_set_group(PGRP_SPI0);

	dws = pvPortMalloc(sizeof(struct dw_spi));
	if (!dws)
		return -ENOMEM;
	memset(dws, 0, sizeof(struct dw_spi));

	dws->xfer_done = xQueueCreate(1, 0);

	dws->regs = (void __iomem *)REGS_SPI0_BASE;
	dws->irq = SPI0_IRQn;
	dws->bus_num = 0;
	dws->max_freq = ulClkGetRate(CLK_SPI0);
	vClkEnable(CLK_SPI0);
	dws->num_cs = 1;
	dws->cs_gpio = SPI0_CS0_GPIO;
	dws->slave.mode = SPI_MODE_0;
	dws->slave.cs = 0;
	dws->slave.xfer = dw_spi_transfer_one;
	dws->slave.qspi_read = dw_qspi_read;
	dws->slave.configure = dw_spi_setup;

	dws->dma_ops = &dw_dma_ops;

	ret = dw_spi_add_host(dws);
	if (ret)
		goto out;

	strncpy(dws->slave.name, "spi0", 16);
	spi_add_slave(&dws->slave);

	return 0;

out:
	return ret;
}
