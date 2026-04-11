#ifndef _DMA_H
#define _DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#define DMA_INT_TC		(1 << 0)
#define DMA_INT_ERR		(1 << 1)

enum DMA_HW_HS_MAP{
	SPI0_RX = 0,
	SPI0_TX,
	SPI1_RX,
	SPI1_TX,
	I2C0_RX,
	I2C0_TX,
	I2C1_RX,
	I2C1_TX,
	UART0_RX,
	UART0_TX,
	UART1_RX,
	UART1_TX,
	UART2_RX,
	UART2_TX,
	UART3_RX,
	UART3_TX,
	I2S_RX,
	I2S_TX,
	I2S1_RX,
	I2S1_TX,
	SDMMC0_RTX,
};

enum dma_transfer_direction {
	DMA_MEM_TO_MEM,
	DMA_MEM_TO_DEV,
	DMA_DEV_TO_MEM,
	DMA_DEV_TO_DEV,
	DMA_TRANS_NONE,
};

enum dma_buswidth {
	DMA_BUSWIDTH_1_BYTE = 0,
	DMA_BUSWIDTH_2_BYTES = 1,
	DMA_BUSWIDTH_4_BYTES = 2,
};

struct dma_config {
	enum dma_transfer_direction direction;
	int src_id;
	int dst_id;
	unsigned int src_addr;
	unsigned int dst_addr;
	enum dma_buswidth src_addr_width;
	enum dma_buswidth dst_addr_width;
	u32 src_maxburst;
	u32 dst_maxburst;
	u32 transfer_size;
	int blkint_en;
	int dst_ahbmaster_sel;
	int src_ahbmaster_sel;
};

struct dma_lli {
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned int next_lli;
	unsigned int control;
};

struct dma_chan {
	int chan_id;
	int in_use;
	void (*irq_callback)(void *param, unsigned int mask);
	void *callback_param;
	struct dma_lli *lli;
};

struct dma_chan *dma_request_channel(int favorite_ch);
void dma_release_channel(struct dma_chan *chan);
int dma_config_channel(struct dma_chan *chan, struct dma_config *config);
int dma_register_complete_callback(struct dma_chan *chan,
											void (*callback)(void *param, unsigned int mask),
											void *callback_param);
int dma_start_channel(struct dma_chan *chan);
int dma_stop_channel(struct dma_chan *chan);
int dma_init(void);
int dma_m2mcpy(unsigned int dst_addr, unsigned int src_addr, int size);



#ifdef __cplusplus
}
#endif

#endif
