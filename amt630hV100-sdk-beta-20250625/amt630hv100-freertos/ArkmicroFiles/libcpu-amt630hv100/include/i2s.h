/*
 * ark_i2s.h
 *
 */

#ifndef __I2S_H
#define __I2S_H

/*
 * I2S Controller Register and Bit Definitions
 */
#define I2S_SACR0			0x00  /* Global Control Register */
#define I2S_SACR1			0x04  /* Serial Audio I 2 S/MSB-Justified Control Register */
#define I2S_SASR0			0x0C  /* Serial Audio I 2 S/MSB-Justified Interface and FIFO Status Register */
#define I2S_SAIMR			0x14  /* Serial Audio Interrupt Mask Register */
#define I2S_SAICR			0x18  /* Serial Audio Interrupt Clear Register */
#define I2S_SADR			0x80  /* Serial Audio Data Register (TX and RX FIFO access Register). */

#define SACR0_RFIFIFIRSTBIT	(1 << 26)	/* rx fifo first bit */
#define SACR0_TFIFOFIRSTBIT	(1 << 25)	/* Tx fifo first bit */
#define SACR0_CHANLOCK		(1 << 24)	/* Channel lock(left first or right first) */
#define SACR0_SCBIT			(1 << 23)	/*  */
#define SACR0_BITS			(1 << 22)	/* I2S Bit Select(16/32 bits) */
#define SACR0_SYNCINV		(1 << 21)	/* SYNC Clock Invert */
#define SACR0_RFTH_MASK		(0x1F << 16)
#define SACR0_RFTH(x)		((x) << 16)	/* Rx FIFO Interrupt or DMA Trigger Threshold */
#define SACR0_TFTH_MASK		(0X1F << 8)
#define SACR0_TFTH(x)		((x) << 8)	/* Tx FIFO Interrupt or DMA Trigger Threshold */
#define SACR0_STRF			(1 << 7)	/* DAC output clk edge select */
#define SACR0_RDMAEN		(1 << 6)	/* RX DMA Enable */
#define SACR0_ENLBF			(1 << 5)	/* Enable Loopback */
#define SACR0_RST			(1 << 4)	/* FIFO, i2s Register Reset */
#define SACR0_TDMAEN		(1 << 3)	/* TX DMA Enable */
#define SACR0_BCKD			(1 << 2)	/* Bit Clock Direction */
#define SACR0_SYNCD			(1 << 1)	/* Word Select(sync) Clock Direction */
#define SACR0_ENB			(1 << 0)	/* Enable I2S Link */

#define SACR1_DRPL			(1 << 1)	/* Disable Replaying Function */
#define SACR1_DREC			(1 << 0)	/* Disable Recording Function */

#define SASR0_RFL(x) 		((x) << 16) /* Rx FIFO Level */
#define SASR0_TFL(x) 		((x) << 8) 	/* Tx FIFO Level */
#define SASR0_ROR			(1 << 6)	/* Rx FIFO Overrun */
#define SASR0_TUR			(1 << 5)	/* Tx FIFO Underrun */
#define SASR0_RFS			(1 << 4)	/* Rx FIFO Service Request */
#define SASR0_TFS			(1 << 3)	/* Tx FIFO Service Request */
#define SASR0_BSY			(1 << 2)	/* I2S Busy */
#define SASR0_RNE			(1 << 1)	/* Rx FIFO Not Empty */
#define SASR0_TNF			(1 << 0)	/* Tx FIFO Not Full */

#define SAICR_ROR			(1 << 6)	/* Clear Rx FIFO Overrun Interrupt */
#define SAICR_TUR			(1 << 5)	/* Clear Tx FIFO Underrun Interrupt */
#define SAICR_RFS			(1 << 4)	/* Clear Rx FIFO Service Interrupt */
#define SAICR_TFS			(1 << 3)	/* Clear Tx FIFO Service Interrupt */

#define SAIMR_ROR			(1 << 6)	/* Enable Rx FIFO Overrun Condition Interrupt */
#define SAIMR_TUR			(1 << 5)	/* Enable Tx FIFO Underrun Condition Interrupt */
#define SAIMR_RFS			(1 << 4)	/* Enable Rx FIFO Service Interrupt */
#define SAIMR_TFS			(1 << 3)	/* Enable Tx FIFO Service Interrupt */


struct ark_i2s_cfg {
	int master;
	int rates;
	int channels;
	int bits;
	int lfirst;	//0: right channel first; 1:left channel first;
};

struct ark_i2s_data {
	unsigned int base;
	unsigned int nco_reg;
	int id;
	int clkid;
	int full_duplex;
	struct ark_i2s_cfg cfg[2];
	struct dma_chan *dma_txch;
	struct dma_chan *dma_rxch;
	SemaphoreHandle_t mutex;
	void *extdata;
};

int ark_i2s_init(struct ark_i2s_data *i2s, int flags);
int ark_i2s_set_volume(struct ark_i2s_data *i2s,int lvol, int rvol);
int ark_i2s_set_rate(struct ark_i2s_data *i2s, int stream, unsigned int rate);
int ark_i2s_set_params(struct ark_i2s_data *i2s, int stream, int rates, int channels, int bits);
int ark_i2s_startup(struct ark_i2s_data *i2s, int stream);
void ark_i2s_stop(struct ark_i2s_data *i2s, int stream);
#endif
