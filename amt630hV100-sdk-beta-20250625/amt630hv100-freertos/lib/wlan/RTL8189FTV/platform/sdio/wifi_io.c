//#include "include/common.h"
#include "FreeRTOS.h"
#include "board.h"
#ifdef SDMMC_SUPPORT
#include "sdio.h"
#include "wifi_io.h"


/* test wifi driver */
#define ADDR_MASK 0x10000
#define LOCAL_ADDR_MASK 0x00000

int sdio_bus_probe(void)
{
}
int sdio_bus_remove(void)
{
	return 0;
}


void sdio_claim_host(struct sdio_func*func)
{
	mmcsd_host_lock(func->card->host);
}
void sdio_release_host(struct sdio_func *func)
{
	mmcsd_host_unlock(func->card->host);
}

int32_t sdio_claim_irq(struct sdio_func *func,
                           void(*handler)(struct sdio_func *))
{
	return sdio_attach_irq(func, handler);
}

int32_t sdio_release_irq(struct sdio_func *func)
{
	return sdio_detach_irq(func);
}

unsigned char sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	unsigned char ret;
	int32_t err = -1;
    ret =  sdio_io_readb(func, addr, (int32_t *)&err);
	//printf("%s addr:%08x\r\n", __func__, addr);
	if (err_ret)
		*err_ret = (int)err;
	return ret;
}

void sdio_writeb(struct sdio_func *func, unsigned char b, unsigned int addr, int *err_ret)
{
	int err = 0;//printf("%s addr:%08x\r\n", __func__, addr);
    err= sdio_io_writeb(func, addr, b);
	if (NULL != err_ret)
		*err_ret = err;
}

uint16_t sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{//printf("%s addr:%08x\r\n", __func__, addr);
	return sdio_io_readw(func, addr, err_ret);
}

void sdio_writew(struct sdio_func *func, uint16_t v, unsigned int addr, int *err_ret)
{
	int err = 0;//printf("%s addr:%08x\r\n", __func__, addr);
    err= sdio_io_writew(func, v, addr);
	if (NULL != err_ret)
		*err_ret = err;
}

uint32_t sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{//printf("%s addr:%08x\r\n", __func__, addr);
	return sdio_io_readl(func, addr, err_ret);
}

void sdio_writel(struct sdio_func *func, uint32_t v, unsigned int addr, int *err_ret)
{
	int err = 0;//printf("%s addr:%08x\r\n", __func__, addr);
    err= sdio_io_writel(func, v, addr);
	if (NULL != err_ret)
		*err_ret = err;
}
#include "usb_os_adapter.h"
int sdio_memcpy_fromio(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{//printf("%s addr:%08x\r\n", __func__, addr);
	
	return sdio_io_rw_extended_block(func, 0, addr, 1, (uint8_t *)dst, count);
}

int sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{//printf("%s   addr:%08x\r\n", __func__, addr);
	//ALLOC_CACHE_ALIGN_BUFFER(char, dma_buf, 2048);
	//memcpy(dma_buf, src, count);
	return sdio_io_rw_extended_block(func, 1, addr, 1, (uint8_t *)src, count);
}

int wifi_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);

	err = sdio_memcpy_fromio(func, pdata, (unsigned int)addr, (int)cnt);
	if (err) {
		//dbg_host("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);
	}

	sdio_release_host(func);

	return err;
}

int wifi_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;
	u32 size;//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);

	size = cnt;
	err = sdio_memcpy_toio(func, addr, pdata, size);
	if (err) {
		//dbg_host("%s: FAIL(%d)! ADDR=%#x Size=%d(%d)\n", __func__, err, addr, cnt, size);
	}

	sdio_release_host(func);

	return err;
}

u8 wifi_readb(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);
	ret = sdio_readb(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err) {
		//dbg_host("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);
	}

	return ret;
}

u16 wifi_readw(struct sdio_func *func, u32 addr)
{
	int err;
	u16 v;//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);
	v = sdio_readw(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err) {
		//dbg_host("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);
	}

	return  v;
}

u32 wifi_readl(struct sdio_func *func, u32 addr)
{
	int err;
	u32 v;//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);
	v = sdio_readl(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	return  v;
}

void wifi_writeb(struct sdio_func *func, u32 addr, u8 val)
{
	int err;
	//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);
	sdio_writeb(func, val, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err) {
		//dbg_host("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr, val);
	}
}

void wifi_writew(struct sdio_func *func, u32 addr, u16 v)
{
	int err;
	//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);
	sdio_writew(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err) {
		//dbg_host("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr, v);
	}
}

void wifi_writel(struct sdio_func *func, u32 addr, u32 v)
{
	int err;
	//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_claim_host(func);
	sdio_writel(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);
}

u8 wifi_readb_local(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;//printf("%s addr:%08x\r\n", __func__, addr);

	ret = sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);

	return ret;
}

void wifi_writeb_local(struct sdio_func *func, u32 addr, u8 val)
{
	int err;//printf("%s addr:%08x\r\n", __func__, addr);

	sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
}
extern int rtw_fake_driver_probe(struct sdio_func *func);
void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func)
{printf("%s:%d\r\n", __func__, __LINE__);
	rtw_fake_driver_probe(func);//todo1
}

extern int sdio_bus_probe(void);
extern int sdio_bus_remove(void);
SDIO_BUS_OPS rtw_sdio_bus_ops = {
	sdio_bus_probe,
	sdio_bus_remove,
	sdio_enable_func,
	sdio_disable_func,
	NULL,
	NULL,
	sdio_claim_irq,
	sdio_release_irq,
	sdio_claim_host,
	sdio_release_host,
	sdio_readb,
	sdio_readw,
	sdio_readl,
	sdio_writeb,
	sdio_writew,
	sdio_writel,
	sdio_memcpy_fromio,
	sdio_memcpy_toio
};
#endif

