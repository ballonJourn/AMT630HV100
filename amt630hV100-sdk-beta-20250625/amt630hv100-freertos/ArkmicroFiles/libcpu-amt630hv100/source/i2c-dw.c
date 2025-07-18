#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "errno.h"


#if defined(HW_I2C0_SUPPORT) || defined(HW_I2C1_SUPPORT)
static char *abort_sources[] = {
	[ABRT_7B_ADDR_NOACK] =
		"slave address not acknowledged (7bit mode)",
	[ABRT_10ADDR1_NOACK] =
		"first address byte not acknowledged (10bit mode)",
	[ABRT_10ADDR2_NOACK] =
		"second address byte not acknowledged (10bit mode)",
	[ABRT_TXDATA_NOACK] =
		"data not acknowledged",
	[ABRT_GCALL_NOACK] =
		"no acknowledgement for a general call",
	[ABRT_GCALL_READ] =
		"read after general call",
	[ABRT_SBYTE_ACKDET] =
		"start byte acknowledged",
	[ABRT_SBYTE_NORSTRT] =
		"trying to send start byte when restart is disabled",
	[ABRT_10B_RD_NORSTRT] =
		"trying to read when restart is disabled (10bit mode)",
	[ABRT_MASTER_DIS] =
		"trying to use disabled adapter",
	[ARB_LOST] =
		"lost arbitration",
	[ABRT_SLAVE_FLUSH_TXFIFO] =
		"read command so flush old data in the TX FIFO",
	[ABRT_SLAVE_ARBLOST] =
		"slave lost the bus while transmitting data to a remote master",
	[ABRT_SLAVE_RD_INTX] =
		"incorrect slave-transmitter mode configuration",
};

u32 dw_readl(struct dw_i2c_dev *dev, int offset)
{
	return readl(dev->base + offset);
}

void dw_writel(struct dw_i2c_dev *dev, u32 b, int offset)
{
	writel(b, dev->base + offset);
}

int i2c_dw_acquire_lock(struct dw_i2c_dev *dev)
{
	int ret;

	if (!dev->acquire_lock)
		return 0;

	ret = dev->acquire_lock(dev);
	if (!ret)
		return 0;

	TRACE_ERROR("couldn't acquire bus ownership\n");

	return ret;
}

void i2c_dw_release_lock(struct dw_i2c_dev *dev)
{
	if (dev->release_lock)
		dev->release_lock(dev);
}

/**
 * i2c_dw_set_reg_access() - Set register access flags
 * @dev: device private data
 *
 * Autodetects needed register access mode and sets access flags accordingly.
 * This must be called before doing any other register access.
 */
int i2c_dw_set_reg_access(struct dw_i2c_dev *dev)
{
	u32 reg;
	int ret;

	ret = i2c_dw_acquire_lock(dev);
	if (ret)
		return ret;

	reg = dw_readl(dev, DW_IC_COMP_TYPE);
	i2c_dw_release_lock(dev);

	if (reg == ___constant_swab32(DW_IC_COMP_TYPE_VALUE)) {
		/* Configure register endianess access */
		dev->flags |= ACCESS_SWAP;
	} else if (reg == (DW_IC_COMP_TYPE_VALUE & 0x0000ffff)) {
		/* Configure register access mode 16bit */
		dev->flags |= ACCESS_16BIT;
	} else if (reg != DW_IC_COMP_TYPE_VALUE) {
		TRACE_ERROR("Unknown Synopsys component type: 0x%08x\n", reg);
		return -ENODEV;
	}

	return 0;
}

u32 i2c_dw_scl_hcnt(u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return (ic_clk * tSYMBOL + 500000) / 1000000 - 8 + offset;
	else
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3).  With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_dw_scl_lcnt().
		 */
		return (ic_clk * (tSYMBOL + tf) + 500000) / 1000000
			- 3 + offset;
}

u32 i2c_dw_scl_lcnt(u32 ic_clk, u32 tLOW, u32 tf, int offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return ((ic_clk * (tLOW + tf) + 500000) / 1000000) - 1 + offset;
}

int i2c_dw_set_sda_hold(struct dw_i2c_dev *dev)
{
	u32 reg;
	int ret;

	ret = i2c_dw_acquire_lock(dev);
	if (ret)
		return ret;

	/* Configure SDA Hold Time if required */
	reg = dw_readl(dev, DW_IC_COMP_VERSION);
	if (reg >= DW_IC_SDA_HOLD_MIN_VERS) {
		if (!dev->sda_hold_time) {
			/* Keep previous hold time setting if no one set it */
			dev->sda_hold_time = dw_readl(dev, DW_IC_SDA_HOLD);
		}

		/*
		 * Workaround for avoiding TX arbitration lost in case I2C
		 * slave pulls SDA down "too quickly" after falling egde of
		 * SCL by enabling non-zero SDA RX hold. Specification says it
		 * extends incoming SDA low to high transition while SCL is
		 * high but it apprears to help also above issue.
		 */
		if (!(dev->sda_hold_time & DW_IC_SDA_HOLD_RX_MASK))
			dev->sda_hold_time |= 1 << DW_IC_SDA_HOLD_RX_SHIFT;

		TRACE_DEBUG("SDA Hold Time TX:RX = %d:%d\n",
			dev->sda_hold_time & ~(u32)DW_IC_SDA_HOLD_RX_MASK,
			dev->sda_hold_time >> DW_IC_SDA_HOLD_RX_SHIFT);
	} else if (dev->sda_hold_time) {
		TRACE_WARNING("Hardware too old to adjust SDA hold time.\n");
		dev->sda_hold_time = 0;
	}

	i2c_dw_release_lock(dev);

	return 0;
}


static inline void __i2c_dw_enable(struct dw_i2c_dev *dev)
{
	dw_writel(dev, 1, DW_IC_ENABLE);
}

static inline void __i2c_dw_disable_nowait(struct dw_i2c_dev *dev)
{
	dw_writel(dev, 0, DW_IC_ENABLE);
}

void __i2c_dw_disable(struct dw_i2c_dev *dev)
{
	int timeout = 100;

	do {
		__i2c_dw_disable_nowait(dev);
		/*
		 * The enable status register may be unimplemented, but
		 * in that case this test reads zero and exits the loop.
		 */
		if ((dw_readl(dev, DW_IC_ENABLE_STATUS) & 1) == 0)
			return;

		/*
		 * Wait 10 times the signaling period of the highest I2C
		 * transfer supported by the driver (for 400KHz this is
		 * 25us) as described in the DesignWare I2C databook.
		 */
		vTaskDelay(pdMS_TO_TICKS(1));
	} while (timeout--);

	TRACE_WARNING("timeout in disabling adapter\n");
}


unsigned long i2c_dw_clk_rate(struct dw_i2c_dev *dev)
{
	/*
	 * Clock is not necessary if we got LCNT/HCNT values directly from
	 * the platform code.
	 */
	if (!dev->get_clk_rate_khz)
		return 0;
	return dev->get_clk_rate_khz(dev);
}

/*
 * Waiting for bus not busy
 */
int i2c_dw_wait_bus_not_busy(struct dw_i2c_dev *dev)
{
	int timeout = TIMEOUT;

	while (dw_readl(dev, DW_IC_STATUS) & DW_IC_STATUS_ACTIVITY) {
		if (timeout <= 0) {
			TRACE_WARNING("timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		timeout--;
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	return 0;
}

int i2c_dw_handle_tx_abort(struct dw_i2c_dev *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;

	if (abort_source & DW_IC_TX_ABRT_NOACK) {
		for (i = 0; i < ARRAY_SIZE(abort_sources); i++) {
			if (abort_source & (1 << i))
				TRACE_DEBUG("%s: %s\n", __func__, abort_sources[i]);
		}
		return -EREMOTEIO;
	}

	for (i = 0; i < ARRAY_SIZE(abort_sources); i++) {
		if (abort_source & (1 << i))
			TRACE_ERROR("%s: %s\n", __func__, abort_sources[i]);
	}

	if (abort_source & DW_IC_TX_ARB_LOST)
		return -EAGAIN;
	else if (abort_source & DW_IC_TX_ABRT_GCALL_READ)
		return -EINVAL; /* wrong msgs[] data */
	else
		return -EIO;
}

void i2c_dw_disable(struct dw_i2c_dev *dev)
{
	/* Disable controller */
	__i2c_dw_disable(dev);

	/* Disable all interupts */
	dw_writel(dev, 0, DW_IC_INTR_MASK);
	dw_readl(dev, DW_IC_CLR_INTR);
}

void i2c_dw_disable_int(struct dw_i2c_dev *dev)
{
	dw_writel(dev, 0, DW_IC_INTR_MASK);
}

u32 i2c_dw_read_comp_param(struct dw_i2c_dev *dev)
{
	return dw_readl(dev, DW_IC_COMP_PARAM_1);
}

static void i2c_dw_configure_fifo_master(struct dw_i2c_dev *dev)
{
	/* Configure Tx/Rx FIFO threshold levels */
	dw_writel(dev, dev->tx_fifo_depth / 2, DW_IC_TX_TL);
	dw_writel(dev, 0, DW_IC_RX_TL);

	/* Configure the I2C master */
	dw_writel(dev, dev->master_cfg, DW_IC_CON);
}

static int i2c_dw_set_timings_master(struct dw_i2c_dev *dev)
{
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
	const char *mode_str, *fp_str = "";
#endif
	u32 comp_param1;
	u32 sda_falling_time, scl_falling_time;
	u32 ic_clk;
	int ret;

	ret = i2c_dw_acquire_lock(dev);
	if (ret)
		return ret;
	comp_param1 = dw_readl(dev, DW_IC_COMP_PARAM_1);
	i2c_dw_release_lock(dev);

	/* Set standard and fast speed dividers for high/low periods */
	sda_falling_time = dev->sda_falling_time ? dev->sda_falling_time : 300; /* ns */
	scl_falling_time = dev->scl_falling_time ? dev->scl_falling_time : 300; /* ns */

	/* Calculate SCL timing parameters for standard mode if not set */
	if (!dev->ss_hcnt || !dev->ss_lcnt) {
		ic_clk = i2c_dw_clk_rate(dev);
		dev->ss_hcnt =
			i2c_dw_scl_hcnt(ic_clk,
					4000,	/* tHD;STA = tHIGH = 4.0 us */
					sda_falling_time,
					0,	/* 0: DW default, 1: Ideal */
					0);	/* No offset */
		dev->ss_lcnt =
			i2c_dw_scl_lcnt(ic_clk,
					4700,	/* tLOW = 4.7 us */
					scl_falling_time,
					0);	/* No offset */
	}
	TRACE_DEBUG("Standard Mode HCNT:LCNT = %d:%d\n", dev->ss_hcnt, dev->ss_lcnt);

	/*
	 * Set SCL timing parameters for fast mode or fast mode plus. Only
	 * difference is the timing parameter values since the registers are
	 * the same.
	 */
	if (dev->clk_freq == 1000000) {
		/*
		 * Check are fast mode plus parameters available and use
		 * fast mode if not.
		 */
		if (dev->fp_hcnt && dev->fp_lcnt) {
			dev->fs_hcnt = dev->fp_hcnt;
			dev->fs_lcnt = dev->fp_lcnt;
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
			fp_str = " Plus";
#endif
		}
	}
	/*
	 * Calculate SCL timing parameters for fast mode if not set. They are
	 * needed also in high speed mode.
	 */
	if (!dev->fs_hcnt || !dev->fs_lcnt) {
		ic_clk = i2c_dw_clk_rate(dev);
		dev->fs_hcnt =
			i2c_dw_scl_hcnt(ic_clk,
					600,	/* tHD;STA = tHIGH = 0.6 us */
					sda_falling_time,
					0,	/* 0: DW default, 1: Ideal */
					0);	/* No offset */
		dev->fs_lcnt =
			i2c_dw_scl_lcnt(ic_clk,
					1300,	/* tLOW = 1.3 us */
					scl_falling_time,
					0);	/* No offset */
	}
	TRACE_DEBUG("Fast Mode%s HCNT:LCNT = %d:%d\n", fp_str, dev->fs_hcnt, dev->fs_lcnt);

	/* Check is high speed possible and fall back to fast mode if not */
	if ((dev->master_cfg & DW_IC_CON_SPEED_MASK) ==
		DW_IC_CON_SPEED_HIGH) {
		if ((comp_param1 & DW_IC_COMP_PARAM_1_SPEED_MODE_MASK)
			!= DW_IC_COMP_PARAM_1_SPEED_MODE_HIGH) {
			TRACE_ERROR("High Speed not supported!\n");
			dev->master_cfg &= ~DW_IC_CON_SPEED_MASK;
			dev->master_cfg |= DW_IC_CON_SPEED_FAST;
			dev->hs_hcnt = 0;
			dev->hs_lcnt = 0;
		} else if (dev->hs_hcnt && dev->hs_lcnt) {
			TRACE_DEBUG("High Speed Mode HCNT:LCNT = %d:%d\n", dev->hs_hcnt, dev->hs_lcnt);
		}
	}

	ret = i2c_dw_set_sda_hold(dev);
	if (ret)
		goto out;

#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
	switch (dev->master_cfg & DW_IC_CON_SPEED_MASK) {
	case DW_IC_CON_SPEED_STD:
		mode_str = "Standard Mode";
		break;
	case DW_IC_CON_SPEED_HIGH:
		mode_str = "High Speed Mode";
		break;
	default:
		mode_str = "Fast Mode";
	}
#endif
	TRACE_DEBUG("Bus speed: %s%s\n", mode_str, fp_str);

out:
	return ret;
}


/**
 * i2c_dw_init() - Initialize the designware I2C master hardware
 * @dev: device private data
 *
 * This functions configures and enables the I2C master.
 * This function is called during I2C init function, and in case of timeout at
 * run time.
 */
static int i2c_dw_init_master(struct dw_i2c_dev *dev)
{
	int ret;

	ret = i2c_dw_acquire_lock(dev);
	if (ret)
		return ret;

	/* Disable the adapter */
	__i2c_dw_disable(dev);

	/* Write standard speed timing parameters */
	dw_writel(dev, dev->ss_hcnt, DW_IC_SS_SCL_HCNT);
	dw_writel(dev, dev->ss_lcnt, DW_IC_SS_SCL_LCNT);

	/* Write fast mode/fast mode plus timing parameters */
	dw_writel(dev, dev->fs_hcnt, DW_IC_FS_SCL_HCNT);
	dw_writel(dev, dev->fs_lcnt, DW_IC_FS_SCL_LCNT);

	/* Write high speed timing parameters if supported */
	if (dev->hs_hcnt && dev->hs_lcnt) {
		dw_writel(dev, dev->hs_hcnt, DW_IC_HS_SCL_HCNT);
		dw_writel(dev, dev->hs_lcnt, DW_IC_HS_SCL_LCNT);
	}

	/* Write SDA hold time if supported */
	if (dev->sda_hold_time)
		dw_writel(dev, dev->sda_hold_time, DW_IC_SDA_HOLD);

	i2c_dw_configure_fifo_master(dev);
	i2c_dw_release_lock(dev);

	return 0;

}

static void i2c_dw_xfer_init(struct dw_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 ic_con, ic_tar = 0;

	/* Disable the adapter */
	__i2c_dw_disable(dev);

	/* If the slave address is ten bit address, enable 10BITADDR */
	ic_con = dw_readl(dev, DW_IC_CON);
	if (msgs[dev->msg_write_idx].flags & I2C_M_TEN) {
		ic_con |= DW_IC_CON_10BITADDR_MASTER;
		/*
		 * If I2C_DYNAMIC_TAR_UPDATE is set, the 10-bit addressing
		 * mode has to be enabled via bit 12 of IC_TAR register.
		 * We set it always as I2C_DYNAMIC_TAR_UPDATE can't be
		 * detected from registers.
		 */
		ic_tar = DW_IC_TAR_10BITADDR_MASTER;
	} else {
		ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
	}

	dw_writel(dev, ic_con, DW_IC_CON);

	/*
	 * Set the slave (target) address and enable 10-bit addressing mode
	 * if applicable.
	 */
	dw_writel(dev, msgs[dev->msg_write_idx].addr | ic_tar, DW_IC_TAR);

	/* Enforce disabled interrupts (due to HW issues) */
	i2c_dw_disable_int(dev);

	dw_writel(dev, 8, DW_IC_TX_PHASE_DELAY);

	/* Enable the adapter */
	__i2c_dw_enable(dev);

	/* Dummy read to avoid the register getting stuck on Bay Trail */
	dw_readl(dev, DW_IC_ENABLE_STATUS);

	/* Clear and enable interrupts */
	dw_readl(dev, DW_IC_CLR_INTR);
	dw_writel(dev, DW_IC_INTR_MASTER_MASK, DW_IC_INTR_MASK);
}

/*
 * Initiate (and continue) low level master read/write transaction.
 * This function is only called from i2c_dw_isr, and pumping i2c_msg
 * messages into the tx buffer.  Even if the size of i2c_msg data is
 * longer than the size of the tx buffer, it handles everything.
 */
static void
i2c_dw_xfer_msg(struct dw_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 intr_mask;
	int tx_limit, rx_limit;
	u32 addr = msgs[dev->msg_write_idx].addr;
	u32 buf_len = dev->tx_buf_len;
	u8 *buf = dev->tx_buf;
	int need_restart = 0;
	u32 status;

	intr_mask = DW_IC_INTR_MASTER_MASK;

	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {
		u32 flags = msgs[dev->msg_write_idx].flags;

		/*
		 * If target address has changed, we need to
		 * reprogram the target address in the I2C
		 * adapter when we are done with this transfer.
		 */
		if (msgs[dev->msg_write_idx].addr != addr) {
			TRACE_ERROR("%s: invalid target address\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (msgs[dev->msg_write_idx].len == 0) {
			TRACE_ERROR("%s: invalid message length\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (!(dev->status & STATUS_WRITE_IN_PROGRESS)) {
			/* new i2c_msg */
			buf = msgs[dev->msg_write_idx].buf;
			buf_len = msgs[dev->msg_write_idx].len;

			/* If both IC_EMPTYFIFO_HOLD_MASTER_EN and
			 * IC_RESTART_EN are set, we must manually
			 * set restart bit between messages.
			 */
			if ((dev->master_cfg & DW_IC_CON_RESTART_EN) &&
					(dev->msg_write_idx > 0))
				need_restart = 1;
		}

		status = dw_readl(dev, DW_IC_STATUS);
		tx_limit = dw_readl(dev, DW_IC_TXFLR);
		if (tx_limit == 0 && !(status & DW_IC_STATUS_TFNF))
			tx_limit = 0;
		else
			tx_limit = dev->tx_fifo_depth - tx_limit;
		rx_limit = dw_readl(dev, DW_IC_RXFLR);
		if (rx_limit == 0 && (status & DW_IC_STATUS_RFF))
			rx_limit = 0;
		else
			rx_limit = dev->rx_fifo_depth - rx_limit;

		while (buf_len > 0 && tx_limit > 0 && rx_limit > 0) {
			u32 cmd = 0;

			/*
			 * If IC_EMPTYFIFO_HOLD_MASTER_EN is set we must
			 * manually set the stop bit. However, it cannot be
			 * detected from the registers so we set it always
			 * when writing/reading the last byte.
			 */

			/*
			 * i2c-core always sets the buffer length of
			 * I2C_FUNC_SMBUS_BLOCK_DATA to 1. The length will
			 * be adjusted when receiving the first byte.
			 * Thus we can't stop the transaction here.
			 */
			if (dev->msg_write_idx == dev->msgs_num - 1 &&
			    buf_len == 1 && !(flags & I2C_M_RECV_LEN))
				cmd |= BIT(9);

			if (need_restart) {
				cmd |= BIT(10);
				need_restart = 0;
			}

			if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {

				/* Avoid rx buffer overrun */
				if (dev->rx_outstanding >= dev->rx_fifo_depth)
					break;

				dw_writel(dev, cmd | 0x100, DW_IC_DATA_CMD);
				rx_limit--;
				dev->rx_outstanding++;
			} else
				dw_writel(dev, cmd | *buf++, DW_IC_DATA_CMD);
			tx_limit--; buf_len--;
		}

		dev->tx_buf = buf;
		dev->tx_buf_len = buf_len;

		/*
		 * Because we don't know the buffer length in the
		 * I2C_FUNC_SMBUS_BLOCK_DATA case, we can't stop
		 * the transaction here.
		 */
		if (buf_len > 0 || flags & I2C_M_RECV_LEN) {
			/* more bytes to be written */
			dev->status |= STATUS_WRITE_IN_PROGRESS;
			break;
		} else
			dev->status &= ~STATUS_WRITE_IN_PROGRESS;
	}

	/*
	 * If i2c_msg index search is completed, we don't need TX_EMPTY
	 * interrupt any more.
	 */
	if (dev->msg_write_idx == dev->msgs_num)
		intr_mask &= ~DW_IC_INTR_TX_EMPTY;

	if (dev->msg_err)
		intr_mask = 0;

	dw_writel(dev, intr_mask,  DW_IC_INTR_MASK);
}

static u8
i2c_dw_recv_len(struct dw_i2c_dev *dev, u8 len)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 flags = msgs[dev->msg_read_idx].flags;

	/*
	 * Adjust the buffer length and mask the flag
	 * after receiving the first byte.
	 */
	len += (flags & I2C_CLIENT_PEC) ? 2 : 1;
	dev->tx_buf_len = len - min(len, dev->rx_outstanding);
	msgs[dev->msg_read_idx].len = len;
	msgs[dev->msg_read_idx].flags &= ~I2C_M_RECV_LEN;

	return len;
}


static void
i2c_dw_read(struct dw_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	int rx_valid;
	u32 status;

	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++) {
		u32 len;
		u8 *buf;

		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		if (!(dev->status & STATUS_READ_IN_PROGRESS)) {
			len = msgs[dev->msg_read_idx].len;
			buf = msgs[dev->msg_read_idx].buf;
		} else {
			len = dev->rx_buf_len;
			buf = dev->rx_buf;
		}

		status = dw_readl(dev, DW_IC_STATUS);
		rx_valid = dw_readl(dev, DW_IC_RXFLR);
		if (rx_valid == 0 && (status & DW_IC_STATUS_RFF))
			rx_valid = DW_IC_FIFO_DEPTH;

		for (; len > 0 && rx_valid > 0; len--, rx_valid--) {
			u32 flags = msgs[dev->msg_read_idx].flags;

			*buf = dw_readl(dev, DW_IC_DATA_CMD);
			/* Ensure length byte is a valid value */
			if (flags & I2C_M_RECV_LEN &&
				*buf <= I2C_SMBUS_BLOCK_MAX && *buf > 0) {
				len = i2c_dw_recv_len(dev, *buf);
			}
			buf++;
			dev->rx_outstanding--;
		}

		if (len > 0) {
			dev->status |= STATUS_READ_IN_PROGRESS;
			dev->rx_buf_len = len;
			dev->rx_buf = buf;
			return;
		} else
			dev->status &= ~STATUS_READ_IN_PROGRESS;
	}
}

/*
 * Prepare controller for a transaction and call i2c_dw_xfer_msg.
 */
static int
i2c_dw_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct dw_i2c_dev *dev = adap->dw_dev;
	int ret;

	xQueueReset(dev->cmd_complete);
	dev->msgs = msgs;
	dev->msgs_num = num;
	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;
	dev->rx_outstanding = 0;

	ret = i2c_dw_acquire_lock(dev);
	if (ret)
		goto done_nolock;

	ret = i2c_dw_wait_bus_not_busy(dev);
	if (ret < 0)
		goto done;

	/* Start the transfers */
	i2c_dw_xfer_init(dev);

	/* Wait for tx to complete */
	if (!xQueueReceive(dev->cmd_complete, NULL, adap->timeout)) {
		TRACE_ERROR("controller timed out\n");
		/* i2c_dw_init implicitly disables the adapter */
		i2c_dw_init_master(dev);
		ret = -ETIMEDOUT;
		goto done;
	}

	if (!dev->cmd_err && dev->status/* == STATUS_READ_IN_PROGRESS*/) {
		u32 stick = xTaskGetTickCount();
		while (dev->status) {
			if (xTaskGetTickCount() - stick > pdMS_TO_TICKS(500))
				break;
		}
	}

	/*
	 * We must disable the adapter before returning and signaling the end
	 * of the current transfer. Otherwise the hardware might continue
	 * generating interrupts which in turn causes a race condition with
	 * the following transfer.  Needs some more investigation if the
	 * additional interrupts are a hardware bug or this driver doesn't
	 * handle them correctly yet.
	 */
	__i2c_dw_disable_nowait(dev);

	if (dev->msg_err) {
		ret = dev->msg_err;
		goto done;
	}

	/* No error */
	if (!dev->cmd_err && !dev->status) {
		ret = num;
		goto done;
	}

	/* We have an error */
	if (dev->cmd_err == DW_IC_ERR_TX_ABRT) {
		ret = i2c_dw_handle_tx_abort(dev);
		goto done;
	}

	if (dev->status)
		TRACE_ERROR("transfer terminated early - interrupt latency too high?\n");

	ret = -EIO;

done:
	i2c_dw_release_lock(dev);

done_nolock:
	return ret;
}

static u32 i2c_dw_get_clk_rate_khz(struct dw_i2c_dev *dev)
{
	return ulClkGetRate(CLK_XTAL24M) / 1000;
}

static void i2c_dw_configure_master(struct dw_i2c_dev *dev)
{
	dev->functionality = I2C_FUNC_10BIT_ADDR | DW_IC_DEFAULT_FUNCTIONALITY;

	dev->master_cfg = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
			  DW_IC_CON_RESTART_EN;

	dev->mode = DW_IC_MASTER;

	switch (dev->clk_freq) {
	case 100000:
		dev->master_cfg |= DW_IC_CON_SPEED_STD;
		break;
	case 3400000:
		dev->master_cfg |= DW_IC_CON_SPEED_HIGH;
		break;
	default:
		dev->master_cfg |= DW_IC_CON_SPEED_FAST;
	}
}

#if defined(I2C0_SLAVE_MODE) || defined(I2C1_SLAVE_MODE)
static void i2c_dw_configure_slave(struct dw_i2c_dev *dev)
{
	dev->functionality = I2C_FUNC_SLAVE | DW_IC_DEFAULT_FUNCTIONALITY;

	dev->slave_cfg = DW_IC_CON_RX_FIFO_FULL_HLD_CTRL |
			 DW_IC_CON_RESTART_EN | DW_IC_CON_STOP_DET_IFADDRESSED;

	dev->mode = DW_IC_SLAVE;
}
#endif

static void dw_i2c_set_fifo_size(struct dw_i2c_dev *dev)
{
	u32 param, tx_fifo_depth, rx_fifo_depth;

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec.
	 */
	param = i2c_dw_read_comp_param(dev);
	tx_fifo_depth = ((param >> 16) & 0xff) + 1;
	rx_fifo_depth = ((param >> 8)  & 0xff) + 1;
	if (!dev->tx_fifo_depth) {
		dev->tx_fifo_depth = tx_fifo_depth;
		dev->rx_fifo_depth = rx_fifo_depth;
	} else if (tx_fifo_depth >= 2) {
		dev->tx_fifo_depth = min(dev->tx_fifo_depth,
				tx_fifo_depth);
		dev->rx_fifo_depth = min(dev->rx_fifo_depth,
				rx_fifo_depth);
	}
}

static u32 i2c_dw_read_clear_intrbits(struct dw_i2c_dev *dev)
{
	u32 stat;

	/*
	 * The IC_INTR_STAT register just indicates "enabled" interrupts.
	 * Ths unmasked raw version of interrupt status bits are available
	 * in the IC_RAW_INTR_STAT register.
	 *
	 * That is,
	 *   stat = dw_readl(IC_INTR_STAT);
	 * equals to,
	 *   stat = dw_readl(IC_RAW_INTR_STAT) & dw_readl(IC_INTR_MASK);
	 *
	 * The raw version might be useful for debugging purposes.
	 */
	stat = dw_readl(dev, DW_IC_INTR_STAT);

	/*
	 * Do not use the IC_CLR_INTR register to clear interrupts, or
	 * you'll miss some interrupts, triggered during the period from
	 * dw_readl(IC_INTR_STAT) to dw_readl(IC_CLR_INTR).
	 *
	 * Instead, use the separately-prepared IC_CLR_* registers.
	 */
	if (stat & DW_IC_INTR_RX_UNDER)
		dw_readl(dev, DW_IC_CLR_RX_UNDER);
	if (stat & DW_IC_INTR_RX_OVER)
		dw_readl(dev, DW_IC_CLR_RX_OVER);
	if (stat & DW_IC_INTR_TX_OVER)
		dw_readl(dev, DW_IC_CLR_TX_OVER);
	if (stat & DW_IC_INTR_RD_REQ)
		dw_readl(dev, DW_IC_CLR_RD_REQ);
	if (stat & DW_IC_INTR_TX_ABRT) {
		/*
		 * The IC_TX_ABRT_SOURCE register is cleared whenever
		 * the IC_CLR_TX_ABRT is read.  Preserve it beforehand.
		 */
		dev->abort_source = dw_readl(dev, DW_IC_TX_ABRT_SOURCE);
		dw_readl(dev, DW_IC_CLR_TX_ABRT);
	}
	if (stat & DW_IC_INTR_RX_DONE)
		dw_readl(dev, DW_IC_CLR_RX_DONE);
	if (stat & DW_IC_INTR_ACTIVITY)
		dw_readl(dev, DW_IC_CLR_ACTIVITY);
	if (stat & DW_IC_INTR_STOP_DET)
		dw_readl(dev, DW_IC_CLR_STOP_DET);
	if (stat & DW_IC_INTR_START_DET)
		dw_readl(dev, DW_IC_CLR_START_DET);
	if (stat & DW_IC_INTR_GEN_CALL)
		dw_readl(dev, DW_IC_CLR_GEN_CALL);

	return stat;
}

/*
 * Interrupt service routine. This gets called whenever an I2C master interrupt
 * occurs.
 */
static int i2c_dw_irq_handler_master(struct dw_i2c_dev *dev)
{
	u32 stat;

	stat = i2c_dw_read_clear_intrbits(dev);
	if (stat & DW_IC_INTR_TX_ABRT) {
		dev->cmd_err |= DW_IC_ERR_TX_ABRT;
		dev->status = STATUS_IDLE;

		/*
		 * Anytime TX_ABRT is set, the contents of the tx/rx
		 * buffers are flushed. Make sure to skip them.
		 */
		dw_writel(dev, 0, DW_IC_INTR_MASK);
		goto tx_aborted;
	}

	if (stat & DW_IC_INTR_RX_FULL)
		i2c_dw_read(dev);

	if (stat & DW_IC_INTR_TX_EMPTY)
		i2c_dw_xfer_msg(dev);

	/*
	 * No need to modify or disable the interrupt mask here.
	 * i2c_dw_xfer_msg() will take care of it according to
	 * the current transmit status.
	 */

tx_aborted:
	if ((stat & (DW_IC_INTR_TX_ABRT | DW_IC_INTR_STOP_DET)) || dev->msg_err)
		xQueueSendFromISR(dev->cmd_complete, NULL, 0);
	else if (dev->flags & ACCESS_INTR_MASK) {
		/* Workaround to trigger pending interrupt */
		stat = dw_readl(dev, DW_IC_INTR_MASK);
		i2c_dw_disable_int(dev);
		dw_writel(dev, stat, DW_IC_INTR_MASK);
	}

	return 0;
}


static void i2c_dw_isr(void *dev_id)
{
	struct dw_i2c_dev *dev = dev_id;
	u32 stat, enabled;

	enabled = dw_readl(dev, DW_IC_ENABLE);
	stat = dw_readl(dev, DW_IC_RAW_INTR_STAT);
	TRACE_DEBUG("enabled=%#x stat=%#x\n", enabled, stat);
	if (!enabled || !(stat & ~DW_IC_INTR_ACTIVITY))
		return;

	i2c_dw_irq_handler_master(dev);
}


static const struct i2c_algorithm i2c_dw_algo = {
	.master_xfer = i2c_dw_xfer,
};

static void i2c_dw_configure_fifo_slave(struct dw_i2c_dev *dev)
{
	/* Configure Tx/Rx FIFO threshold levels. */
	dw_writel(dev, 0, DW_IC_TX_TL);
	dw_writel(dev, 0, DW_IC_RX_TL);

	/* Configure the I2C slave. */
	dw_writel(dev, dev->slave_cfg, DW_IC_CON);
	dw_writel(dev, DW_IC_INTR_SLAVE_MASK, DW_IC_INTR_MASK);
}

/**
 * i2c_dw_init_slave() - Initialize the designware i2c slave hardware
 * @dev: device private data
 *
 * This function configures and enables the I2C in slave mode.
 * This function is called during I2C init function, and in case of timeout at
 * run time.
 */
static int i2c_dw_init_slave(struct dw_i2c_dev *dev)
{
	int ret;

	ret = i2c_dw_acquire_lock(dev);
	if (ret)
		return ret;

	/* Disable the adapter. */
	__i2c_dw_disable(dev);

	/* Write SDA hold time if supported */
	if (dev->sda_hold_time)
		dw_writel(dev, dev->sda_hold_time, DW_IC_SDA_HOLD);

	i2c_dw_configure_fifo_slave(dev);
	i2c_dw_release_lock(dev);

	return 0;
}

static int i2c_dw_reg_slave(struct i2c_adapter *slave)
{
	struct dw_i2c_dev *dev = slave->dw_dev;

	if (dev->slave)
		return -EBUSY;
	if (slave->flags & I2C_CLIENT_TEN)
		return -ENOTSUP;

	/*
	 * Set slave address in the IC_SAR register,
	 * the address to which the DW_apb_i2c responds.
	 */
	__i2c_dw_disable_nowait(dev);
	dw_writel(dev, slave->addr, DW_IC_SAR);
	dev->slave = slave;

	__i2c_dw_enable(dev);

	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;
	dev->rx_outstanding = 0;

	return 0;
}

static int i2c_dw_unreg_slave(struct i2c_adapter *slave)
{
	struct dw_i2c_dev *dev = slave->dw_dev;

	dev->disable_int(dev);
	dev->disable(dev);
	dev->slave = NULL;

	return 0;
}

static u32 i2c_dw_read_clear_intrbits_slave(struct dw_i2c_dev *dev)
{
	u32 stat;

	/*
	 * The IC_INTR_STAT register just indicates "enabled" interrupts.
	 * Ths unmasked raw version of interrupt status bits are available
	 * in the IC_RAW_INTR_STAT register.
	 *
	 * That is,
	 *   stat = dw_readl(IC_INTR_STAT);
	 * equals to,
	 *   stat = dw_readl(IC_RAW_INTR_STAT) & dw_readl(IC_INTR_MASK);
	 *
	 * The raw version might be useful for debugging purposes.
	 */
	stat = dw_readl(dev, DW_IC_INTR_STAT);

	/*
	 * Do not use the IC_CLR_INTR register to clear interrupts, or
	 * you'll miss some interrupts, triggered during the period from
	 * dw_readl(IC_INTR_STAT) to dw_readl(IC_CLR_INTR).
	 *
	 * Instead, use the separately-prepared IC_CLR_* registers.
	 */
	if (stat & DW_IC_INTR_TX_ABRT)
		dw_readl(dev, DW_IC_CLR_TX_ABRT);
	if (stat & DW_IC_INTR_RX_UNDER)
		dw_readl(dev, DW_IC_CLR_RX_UNDER);
	if (stat & DW_IC_INTR_RX_OVER)
		dw_readl(dev, DW_IC_CLR_RX_OVER);
	if (stat & DW_IC_INTR_TX_OVER)
		dw_readl(dev, DW_IC_CLR_TX_OVER);
	if (stat & DW_IC_INTR_RX_DONE)
		dw_readl(dev, DW_IC_CLR_RX_DONE);
	if (stat & DW_IC_INTR_ACTIVITY)
		dw_readl(dev, DW_IC_CLR_ACTIVITY);
	if (stat & DW_IC_INTR_STOP_DET)
		dw_readl(dev, DW_IC_CLR_STOP_DET);
	if (stat & DW_IC_INTR_START_DET)
		dw_readl(dev, DW_IC_CLR_START_DET);
	if (stat & DW_IC_INTR_GEN_CALL)
		dw_readl(dev, DW_IC_CLR_GEN_CALL);

	return stat;
}

/*
 * Interrupt service routine. This gets called whenever an I2C slave interrupt
 * occurs.
 */

static int i2c_dw_irq_handler_slave(struct dw_i2c_dev *dev)
{
	u32 raw_stat, stat, enabled;
	u8 val, slave_activity;

	stat = dw_readl(dev, DW_IC_INTR_STAT);
	enabled = dw_readl(dev, DW_IC_ENABLE);
	raw_stat = dw_readl(dev, DW_IC_RAW_INTR_STAT);
	slave_activity = ((dw_readl(dev, DW_IC_STATUS) &
		DW_IC_STATUS_SLAVE_ACTIVITY) >> 6);

	if (!enabled || !(raw_stat & ~DW_IC_INTR_ACTIVITY) || !dev->slave)
		return 0;

	TRACE_DEBUG("%#x STATUS SLAVE_ACTIVITY=%#x : RAW_INTR_STAT=%#x : INTR_STAT=%#x\n",
		enabled, slave_activity, raw_stat, stat);

	if ((stat & DW_IC_INTR_RX_FULL) && (stat & DW_IC_INTR_STOP_DET))
		i2c_slave_event(dev->slave, I2C_SLAVE_WRITE_REQUESTED, &val);

	if (stat & DW_IC_INTR_RD_REQ) {
		if (slave_activity) {
			if (stat & DW_IC_INTR_RX_FULL) {
				val = dw_readl(dev, DW_IC_DATA_CMD);

				if (!i2c_slave_event(dev->slave,
						     I2C_SLAVE_WRITE_RECEIVED,
						     &val)) {
					TRACE_DEBUG("Byte %X acked!", val);
				}
				dw_readl(dev, DW_IC_CLR_RD_REQ);
				stat = i2c_dw_read_clear_intrbits_slave(dev);
			} else {
				dw_readl(dev, DW_IC_CLR_RD_REQ);
				dw_readl(dev, DW_IC_CLR_RX_UNDER);
				stat = i2c_dw_read_clear_intrbits_slave(dev);
			}
			if (!i2c_slave_event(dev->slave,
					     I2C_SLAVE_READ_REQUESTED,
					     &val))
				dw_writel(dev, val, DW_IC_DATA_CMD);
		}
	}

	if (stat & DW_IC_INTR_RX_DONE) {
		if (!i2c_slave_event(dev->slave, I2C_SLAVE_READ_PROCESSED,
				     &val))
			dw_readl(dev, DW_IC_CLR_RX_DONE);

		i2c_slave_event(dev->slave, I2C_SLAVE_STOP, &val);
		stat = i2c_dw_read_clear_intrbits_slave(dev);
		return 1;
	}

	if (stat & DW_IC_INTR_RX_FULL) {
		val = dw_readl(dev, DW_IC_DATA_CMD);
		if (!i2c_slave_event(dev->slave, I2C_SLAVE_WRITE_RECEIVED,
				     &val))
			TRACE_DEBUG("Byte %X acked!", val);
	} else {
		i2c_slave_event(dev->slave, I2C_SLAVE_STOP, &val);
		stat = i2c_dw_read_clear_intrbits_slave(dev);
	}

	return 1;
}

static void i2c_dw_isr_slave(void *dev_id)
{
	struct dw_i2c_dev *dev = dev_id;
	int ret;

	i2c_dw_read_clear_intrbits_slave(dev);
	ret = i2c_dw_irq_handler_slave(dev);
	if (ret > 0)
		xQueueSendFromISR(dev->cmd_complete, NULL, 0);
}

static const struct i2c_algorithm i2c_dw_slave_algo = {
	.reg_slave = i2c_dw_reg_slave,
	.unreg_slave = i2c_dw_unreg_slave,
};

int i2c_dw_init(int id)
{
	struct i2c_adapter *adap;
	struct dw_i2c_dev *dev;
	int ret;

	dev = pvPortMalloc(sizeof(struct dw_i2c_dev));
	if (!dev) {
		TRACE_ERROR("[%s] pvPortMalloc failed\n", __func__);
		return -1;
	}
	memset(dev, 0, sizeof(struct dw_i2c_dev));

	dev->cmd_complete = xQueueCreate(1, 0);

	if(id == 0) {
		dev->base = REGS_IIC0_BASE;
		dev->irq = I2C0_IRQn;
		dev->clk_freq = 100000;
		sys_soft_reset(softreset_i2c);
#ifdef I2C0_SLAVE_MODE
		i2c_dw_configure_slave(dev);
#else
		i2c_dw_configure_master(dev);
#endif
	} else if(id == 1) {
		dev->base = REGS_IIC1_BASE;
		dev->irq = I2C1_IRQn;
		dev->clk_freq = 100000;
		sys_soft_reset(softreset_i2c1);
#ifdef I2C1_SLAVE_MODE
		i2c_dw_configure_slave(dev);
#else
		i2c_dw_configure_master(dev);
#endif
	} else {
		TRACE_ERROR("[%s] Invalid id:%d\n", __func__, id);
		ret = -1;
		goto exit;
	}

	dev->get_clk_rate_khz = i2c_dw_get_clk_rate_khz;

	dw_i2c_set_fifo_size(dev);

	adap = &dev->adapter;

	if (dev->mode == DW_IC_MASTER) {
		dev->init = i2c_dw_init_master;
		dev->disable = i2c_dw_disable;
		dev->disable_int = i2c_dw_disable_int;

		ret = i2c_dw_set_reg_access(dev);
		if (ret) {
			TRACE_ERROR("[%s] i2c_dw_set_reg_access failed, id:%d\n", __func__, id);
			goto exit;
		}

		ret = i2c_dw_set_timings_master(dev);
		if (ret) {
			TRACE_ERROR("[%s] i2c_dw_set_timings_master failed, id:%d\n", __func__, id);
			goto exit;
		}

		ret = dev->init(dev);
		if (ret) {
			TRACE_ERROR("[%s] dev->init failed, id:%d\n", __func__, id);
			goto exit;
		}

		adap->retries = 3;
		adap->algo = &i2c_dw_algo;

		i2c_dw_disable_int(dev);

		request_irq(dev->irq, 0, i2c_dw_isr, dev);
	} else {
		dev->init = i2c_dw_init_slave;
		dev->disable = i2c_dw_disable;
		dev->disable_int = i2c_dw_disable_int;

		ret = i2c_dw_set_reg_access(dev);
		if (ret) {
			TRACE_ERROR("[%s] i2c_dw_set_reg_access failed, id:%d\n", __func__, id);
			goto exit;
		}

		ret = i2c_dw_set_sda_hold(dev);
		if (ret) {
			TRACE_ERROR("[%s] i2c_dw_set_sda_hold failed, id:%d\n", __func__, id);
			goto exit;
		}

		ret = dev->init(dev);
		if (ret) {
			TRACE_ERROR("[%s] dev->init failed, id:%d\n", __func__, id);
			goto exit;
		}

		adap->retries = 3;
		adap->algo = &i2c_dw_slave_algo;
		adap->flags |= I2C_CLIENT_SLAVE;

		request_irq(dev->irq, 0, i2c_dw_isr_slave, dev);
	}

	adap->dw_dev = dev;
	snprintf(adap->name, sizeof(adap->name), "i2c%d", id);
	ret = i2c_add_adapter(adap);
	if(ret) {
		TRACE_ERROR("[%s] i2c_add_adapter failed, id:%d\n", __func__, id);
		free_irq(dev->irq);
		goto exit;
	}

	return 0;
exit:
	if(dev)
		vPortFree(dev);

	return ret;
}
#endif
