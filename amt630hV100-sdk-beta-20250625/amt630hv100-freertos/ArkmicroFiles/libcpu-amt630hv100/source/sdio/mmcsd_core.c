#include "FreeRTOS.h"
#include "os_adapt.h"
#include "trace.h"
#include "errno.h"
#include "ff_sddisk.h"

#include "mmcsd_core.h"
#include "sd.h"
#include "mmc.h"
#include "sdio.h"

#define MMCSD_STACK_SIZE 2048
#define MMCSD_THREAD_PREORITY  0x16

#define	SDMMC_MOUNT_PATH	"/sd"

struct mmcsd_card *sdmmc_cardinfo = NULL;
FF_Disk_t *sdmmc_disk;

static TaskHandle_t mmcsd_detect_thread;
static QueueHandle_t  mmcsd_detect_mb;
static QueueHandle_t mmcsd_hotpluge_mb;
static QueueHandle_t mmcsd_sdio_ready_mb;

static QueueHandle_t mmcsd_mmc_ready_mb;

void mmcsd_host_lock(struct mmcsd_host *host)
{
    xSemaphoreTake(host->bus_lock, portMAX_DELAY);
}

void mmcsd_host_unlock(struct mmcsd_host *host)
{
    xSemaphoreGive(host->bus_lock);
}

void mmcsd_req_complete(struct mmcsd_host *host)
{
    xSemaphoreGive(host->sem_ack);
}

void mmcsd_send_request(struct mmcsd_host *host, struct mmcsd_req *req)
{
    do {
        req->cmd->retries--;
        req->cmd->err = 0;
        req->cmd->mrq = req;
        if (req->data)
        {   
            req->cmd->data = req->data;
            req->data->err = 0;
            req->data->mrq = req;
            if (req->stop)
            {
                req->data->stop = req->stop;
                req->stop->err = 0;
                req->stop->mrq = req;
            }       
        }
        host->ops->request(host, req);

        xSemaphoreTake(host->sem_ack, portMAX_DELAY);
          
    } while(req->cmd->err && (req->cmd->retries > 0));


}

int32_t mmcsd_send_cmd(struct mmcsd_host *host,
                          struct mmcsd_cmd  *cmd,
                          int                   retries)
{
    struct mmcsd_req req;

    memset(&req, 0, sizeof(struct mmcsd_req));
    memset(cmd->resp, 0, sizeof(cmd->resp));
    cmd->retries = retries;

    req.cmd = cmd;
    cmd->data = NULL;

    mmcsd_send_request(host, &req);

    return cmd->err;
}

int32_t mmcsd_go_idle(struct mmcsd_host *host)
{
    int32_t err;
    struct mmcsd_cmd cmd;

    if (!controller_is_spi(host))
    {
        mmcsd_set_chip_select(host, MMCSD_CS_HIGH);
        mmcsd_delay_ms(1);
    }

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_NONE | CMD_BC;

    err = mmcsd_send_cmd(host, &cmd, 0);

    mmcsd_delay_ms(1);

    if (!controller_is_spi(host)) 
    {
        mmcsd_set_chip_select(host, MMCSD_CS_IGNORE);
        mmcsd_delay_ms(1);
    }

    return err;
}

int32_t mmcsd_spi_read_ocr(struct mmcsd_host *host,
                              int32_t            high_capacity,
                              uint32_t          *ocr)
{
    struct mmcsd_cmd cmd;
    int32_t err;

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = SPI_READ_OCR;
    cmd.arg = high_capacity ? (1 << 30) : 0;
    cmd.flags = RESP_SPI_R3;

    err = mmcsd_send_cmd(host, &cmd, 0);

    *ocr = cmd.resp[1];

    return err;
}

int32_t mmcsd_all_get_cid(struct mmcsd_host *host, uint32_t *cid)
{
    int32_t err;
    struct mmcsd_cmd cmd;

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = ALL_SEND_CID;
    cmd.arg = 0;
    cmd.flags = RESP_R2 | CMD_BCR;

    err = mmcsd_send_cmd(host, &cmd, 3);
    if (err)
        return err;

    memcpy(cid, cmd.resp, sizeof(uint32_t) * 4);

    return 0;
}

int32_t mmcsd_get_cid(struct mmcsd_host *host, uint32_t *cid)
{
    int32_t err, i;
    struct mmcsd_req req;
    struct mmcsd_cmd cmd;
    struct mmcsd_data data;
    uint32_t *buf = NULL;

    if (!controller_is_spi(host)) 
    {
        if (!host->card)
            return -1;
        memset(&cmd, 0, sizeof(struct mmcsd_cmd));

        cmd.cmd_code = SEND_CID;
        cmd.arg = host->card->rca << 16;
        cmd.flags = RESP_R2 | CMD_AC;
        err = mmcsd_send_cmd(host, &cmd, 3);
        if (err)
            return err;

        memcpy(cid, cmd.resp, sizeof(uint32_t) * 4);

        return 0;
    }

    buf = (uint32_t *)pvPortMalloc(16);
    if (!buf) 
    {
        TRACE_ERROR("allocate memory failed!");

        return -ENOMEM;
    }

    memset(&req, 0, sizeof(struct mmcsd_req));
    memset(&cmd, 0, sizeof(struct mmcsd_cmd));
    memset(&data, 0, sizeof(struct mmcsd_data));

    req.cmd = &cmd;
    req.data = &data;

    cmd.cmd_code = SEND_CID;
    cmd.arg = 0;

    /* NOTE HACK:  the RESP_SPI_R1 is always correct here, but we
     * rely on callers to never use this with "native" calls for reading
     * CSD or CID.  Native versions of those commands use the R2 type,
     * not R1 plus a data block.
     */
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    data.blksize = 16;
    data.blks = 1;
    data.flags = DATA_DIR_READ;
    data.buf = buf;
    /*
     * The spec states that CSR and CID accesses have a timeout
     * of 64 clock cycles.
     */
    data.timeout_ns = 0;
    data.timeout_clks = 64;

    mmcsd_send_request(host, &req);

    if (cmd.err || data.err)
    {
        vPortFree(buf);

        return -1;
    }

    for (i = 0;i < 4;i++)
        cid[i] = buf[i];
    vPortFree(buf);

    return 0;
}

int32_t mmcsd_get_csd(struct mmcsd_card *card, uint32_t *csd)
{
    int32_t err, i;
    struct mmcsd_req req;
    struct mmcsd_cmd cmd;
    struct mmcsd_data data;
    uint32_t *buf = NULL;

    if (!controller_is_spi(card->host))
    {
        memset(&cmd, 0, sizeof(struct mmcsd_cmd));

        cmd.cmd_code = SEND_CSD;
        cmd.arg = card->rca << 16;
        cmd.flags = RESP_R2 | CMD_AC;
        err = mmcsd_send_cmd(card->host, &cmd, 3);
        if (err)
            return err;

        memcpy(csd, cmd.resp, sizeof(uint32_t) * 4);

        return 0;
    }

    buf = (uint32_t*)pvPortMalloc(16);
    if (!buf) 
    {
        TRACE_ERROR("allocate memory failed!");

        return -ENOMEM;
    }

    memset(&req, 0, sizeof(struct mmcsd_req));
    memset(&cmd, 0, sizeof(struct mmcsd_cmd));
    memset(&data, 0, sizeof(struct mmcsd_data));

    req.cmd = &cmd;
    req.data = &data;

    cmd.cmd_code = SEND_CSD;
    cmd.arg = 0;

    /* NOTE HACK:  the RESP_SPI_R1 is always correct here, but we
     * rely on callers to never use this with "native" calls for reading
     * CSD or CID.  Native versions of those commands use the R2 type,
     * not R1 plus a data block.
     */
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    data.blksize = 16;
    data.blks = 1;
    data.flags = DATA_DIR_READ;
    data.buf = buf;

    /*
     * The spec states that CSR and CID accesses have a timeout
     * of 64 clock cycles.
     */
    data.timeout_ns = 0;
    data.timeout_clks = 64;

    mmcsd_send_request(card->host, &req);

    if (cmd.err || data.err)
    {
        vPortFree(buf);

        return -1;
    }

    for (i = 0;i < 4;i++)
        csd[i] = buf[i];
    vPortFree(buf);

    return 0;
}

static int32_t _mmcsd_select_card(struct mmcsd_host *host,
                                     struct mmcsd_card *card)
{
    int32_t err;
    struct mmcsd_cmd cmd;

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = SELECT_CARD;

    if (card) 
    {
        cmd.arg = card->rca << 16;
        cmd.flags = RESP_R1 | CMD_AC;
    } 
    else 
    {
        cmd.arg = 0;
        cmd.flags = RESP_NONE | CMD_AC;
    }

    err = mmcsd_send_cmd(host, &cmd, 3);
    if (err)
        return err;

    return 0;
}

int32_t mmcsd_select_card(struct mmcsd_card *card)
{
    return _mmcsd_select_card(card->host, card);
}

int32_t mmcsd_deselect_cards(struct mmcsd_card *card)
{
    return _mmcsd_select_card(card->host, NULL);
}

int32_t mmcsd_spi_use_crc(struct mmcsd_host *host, int32_t use_crc)
{
    struct mmcsd_cmd cmd;
    int32_t err;

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = SPI_CRC_ON_OFF;
    cmd.flags = RESP_SPI_R1;
    cmd.arg = use_crc;

    err = mmcsd_send_cmd(host, &cmd, 0);
    if (!err)
        host->spi_use_crc = use_crc;

    return err;
}

static inline void mmcsd_set_iocfg(struct mmcsd_host *host)
{
    struct mmcsd_io_cfg *io_cfg = &host->io_cfg;

    mmcsd_dbg("clock %uHz busmode %u powermode %u cs %u Vdd %u "
        "width %u \n",
         io_cfg->clock, io_cfg->bus_mode,
         io_cfg->power_mode, io_cfg->chip_select, io_cfg->vdd,
         io_cfg->bus_width);

    host->ops->set_iocfg(host, io_cfg);
}

/*
 * Control chip select pin on a host.
 */
void mmcsd_set_chip_select(struct mmcsd_host *host, int32_t mode)
{
    host->io_cfg.chip_select = mode;
    mmcsd_set_iocfg(host);
}

/*
 * Sets the host clock to the highest possible frequency that
 * is below "hz".
 */
void mmcsd_set_clock(struct mmcsd_host *host, uint32_t clk)
{
    if (clk < host->freq_min)
    {
        TRACE_WARNING("clock too low!");
    }

    host->io_cfg.clock = clk;
    mmcsd_set_iocfg(host);
}

/*
 * Change the bus mode (open drain/push-pull) of a host.
 */
void mmcsd_set_bus_mode(struct mmcsd_host *host, uint32_t mode)
{
    host->io_cfg.bus_mode = mode;
    mmcsd_set_iocfg(host);
}

/*
 * Change data bus width of a host.
 */
void mmcsd_set_bus_width(struct mmcsd_host *host, uint32_t width)
{
    host->io_cfg.bus_width = width;
    mmcsd_set_iocfg(host);
}

void mmcsd_set_data_timeout(struct mmcsd_data       *data,
                            const struct mmcsd_card *card)
{
    uint32_t mult;

    if (card->card_type == CARD_TYPE_SDIO) 
    {
        data->timeout_ns = 1000000000;  /* SDIO card 1s */
        data->timeout_clks = 0;

        return;
    }

    /*
     * SD cards use a 100 multiplier rather than 10
     */
    mult = (card->card_type == CARD_TYPE_SD) ? 100 : 10;

    /*
     * Scale up the multiplier (and therefore the timeout) by
     * the r2w factor for writes.
     */
    if (data->flags & DATA_DIR_WRITE)
        mult <<= card->csd.r2w_factor;

    data->timeout_ns = card->tacc_ns * mult;
    data->timeout_clks = card->tacc_clks * mult;

    /*
     * SD cards also have an upper limit on the timeout.
     */
    if (card->card_type == CARD_TYPE_SD) 
    {
        uint32_t timeout_us, limit_us;

        timeout_us = data->timeout_ns / 1000;
        timeout_us += data->timeout_clks * 1000 /
            (card->host->io_cfg.clock / 1000);

        if (data->flags & DATA_DIR_WRITE)
            /*
             * The limit is really 250 ms, but that is
             * insufficient for some crappy cards.
             */
            limit_us = 300000;
        else
            limit_us = 100000;

        /*
         * SDHC cards always use these fixed values.
         */
        if (timeout_us > limit_us || card->flags & CARD_FLAG_SDHC) 
        {
            data->timeout_ns = limit_us * 1000; /* SDHC card fixed 250ms */
            data->timeout_clks = 0;
        }
    }

    if (controller_is_spi(card->host)) 
    {
        if (data->flags & DATA_DIR_WRITE) 
        {
            if (data->timeout_ns < 1000000000)
                data->timeout_ns = 1000000000;  /* 1s */
        } 
        else 
        {
            if (data->timeout_ns < 100000000)
                data->timeout_ns =  100000000;  /* 100ms */
        }
    }
}

/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
uint32_t mmcsd_select_voltage(struct mmcsd_host *host, uint32_t ocr)
{
    int bit;

    ocr &= host->valid_ocr;

    bit = ffs(ocr);
    if (bit) 
    {
        bit -= 1;

        ocr &= 3 << bit;

        host->io_cfg.vdd = bit;
        mmcsd_set_iocfg(host);
    } 
    else 
    {
        TRACE_WARNING("host doesn't support card's voltages!");
        ocr = 0;
    }

    return ocr;
}

static void mmcsd_power_up(struct mmcsd_host *host)
{
    int bit = fls(host->valid_ocr) - 1;

    host->io_cfg.vdd = bit;
    if (controller_is_spi(host))
    {
        host->io_cfg.chip_select = MMCSD_CS_HIGH;
        host->io_cfg.bus_mode = MMCSD_BUSMODE_PUSHPULL;
    } 
    else
    {
        host->io_cfg.chip_select = MMCSD_CS_IGNORE;
        host->io_cfg.bus_mode = MMCSD_BUSMODE_OPENDRAIN;
    }
    host->io_cfg.power_mode = MMCSD_POWER_UP;
    host->io_cfg.bus_width = MMCSD_BUS_WIDTH_1;
    mmcsd_set_iocfg(host);

    /*
     * This delay should be sufficient to allow the power supply
     * to reach the minimum voltage.
     */
    mmcsd_delay_ms(10);

    host->io_cfg.clock = host->freq_min;
    host->io_cfg.power_mode = MMCSD_POWER_ON;
    mmcsd_set_iocfg(host);

    /*
     * This delay must be at least 74 clock sizes, or 1 ms, or the
     * time required to reach a stable voltage.
     */
    mmcsd_delay_ms(10);
}

static void mmcsd_power_off(struct mmcsd_host *host)
{
    host->io_cfg.clock = 0;
    host->io_cfg.vdd = 0;
    if (!controller_is_spi(host)) 
    {
        host->io_cfg.bus_mode = MMCSD_BUSMODE_OPENDRAIN;
        host->io_cfg.chip_select = MMCSD_CS_IGNORE;
    }
    host->io_cfg.power_mode = MMCSD_POWER_OFF;
    host->io_cfg.bus_width = MMCSD_BUS_WIDTH_1;
    mmcsd_set_iocfg(host);
}

int mmcsd_wait_cd_changed(uint32_t timeout)
{
    struct mmcsd_host *host;
    if (xQueueReceive(mmcsd_hotpluge_mb, &host, timeout) == pdPASS)
    {
        if(host->card == NULL)
        {
            return MMCSD_HOST_UNPLUGED;
        }
        else
        {
            return MMCSD_HOST_PLUGED;
        }
    }
    return -1;
}

int mmcsd_wait_sdio_ready(int32_t timeout)
{
    struct mmcsd_host *host;
    if (xQueueReceive(mmcsd_sdio_ready_mb, &host, timeout) == pdPASS)
    	return MMCSD_HOST_PLUGED;

    return -1;
}

int mmcsd_wait_mmc_ready(uint32_t timeout)
{
    struct mmcsd_host *host;
    if (xQueueReceive(mmcsd_mmc_ready_mb, &host, timeout) == pdPASS)
    	return MMCSD_HOST_PLUGED;

    return -1;
}


void mmcsd_change(struct mmcsd_host *host)
{
    xQueueSend(mmcsd_detect_mb, &host, 0);
}

void mmcsd_change_from_isr(struct mmcsd_host *host)
{
    xQueueSendFromISR(mmcsd_detect_mb, &host, 0);
}

void mmcsd_detect(void *param)
{
    struct mmcsd_host *host;
    uint32_t  ocr;
    int32_t  err;

    while (1) 
    {
        if (xQueueReceive(mmcsd_detect_mb, &host, portMAX_DELAY) == pdPASS)
        {
            if (host->card == NULL)
            {
                mmcsd_host_lock(host);
                mmcsd_power_up(host);
                mmcsd_go_idle(host);

                mmcsd_send_if_cond(host, host->valid_ocr);

#if DEVICE_TYPE_SELECT != EMMC_FLASH
                err = sdio_io_send_op_cond(host, 0, &ocr);
                if (!err)
                {
                    if (init_sdio(host, ocr))
                        mmcsd_power_off(host);
					else
						xQueueSend(mmcsd_sdio_ready_mb, &host, 0);
                    mmcsd_host_unlock(host);
                    continue;
                }

                /*
                 * detect SD card
                 */
                err = mmcsd_send_app_op_cond(host, 0, &ocr);
                if (!err) 
                {
                    if (init_sd(host, ocr)) {
                        mmcsd_power_off(host);
                    } else {
						sdmmc_cardinfo = host->card;
	                    mmcsd_host_unlock(host);
						sdmmc_disk = FF_SDDiskInit(SDMMC_MOUNT_PATH);
						xQueueSend(mmcsd_hotpluge_mb, &host, 0);
                    }
                    continue;
                }
#else                
                /*
                 * detect mmc card
                 */
                err = mmc_send_op_cond(host, 0, &ocr);
                if (!err) 
                {               
                    if (init_mmc(host, ocr)) {
                        mmcsd_power_off(host);
                    } else {
						sdmmc_cardinfo = host->card;
	                    mmcsd_host_unlock(host);
						sdmmc_disk = FF_SDDiskInit(SDMMC_MOUNT_PATH);
						xQueueSend(mmcsd_mmc_ready_mb, &host, 0);
                    }
                    continue;			
                }
#endif
                mmcsd_host_unlock(host);
            }
            else
            {
            	/* card removed */
            	mmcsd_host_lock(host);
            	if (host->card->sdio_function_num != 0)
            	{
            		TRACE_WARNING("unsupport sdio card plug out!\r\n");
            	}
            	else
            	{
            		if (host->card->card_type == CARD_TYPE_SD || host->card->card_type == CARD_TYPE_MMC) {
						FF_SDDiskDelete(sdmmc_disk);
						sdmmc_cardinfo = NULL;
					}
            		vPortFree(host->card);

            		host->card = NULL;
            	}
            	mmcsd_host_unlock(host);
            	xQueueSend(mmcsd_hotpluge_mb, &host, 0);
            }
        }
    }
}

struct mmcsd_host *mmcsd_alloc_host(void)
{
    struct mmcsd_host *host;

    host = pvPortMalloc(sizeof(struct mmcsd_host));
    if (!host) 
    {
        TRACE_ERROR("alloc host failed");

        return NULL;
    }

    memset(host, 0, sizeof(struct mmcsd_host));

    host->max_seg_size = 65535;
    host->max_dma_segs = 1;
    host->max_blk_size = 512;
    host->max_blk_count = 4096;

    host->bus_lock = xSemaphoreCreateMutex();
    host->sem_ack= xSemaphoreCreateBinary();

    return host;
}

void mmcsd_free_host(struct mmcsd_host *host)
{
    vSemaphoreDelete(host->bus_lock);
    vSemaphoreDelete(host->sem_ack);
    vPortFree(host);
}

int mmcsd_core_init(void)
{
    /* initialize detect SD cart thread */
    /* initialize mailbox and create detect SD card thread */
    mmcsd_detect_mb = xQueueCreate(4, sizeof(uint32_t));
    configASSERT(mmcsd_detect_mb);

	mmcsd_hotpluge_mb = xQueueCreate(4, sizeof(uint32_t));
	configASSERT(mmcsd_hotpluge_mb);

	mmcsd_sdio_ready_mb = xQueueCreate(4, sizeof(uint32_t));
	configASSERT(mmcsd_sdio_ready_mb);

	mmcsd_mmc_ready_mb = xQueueCreate(4, sizeof(uint32_t));
	configASSERT(mmcsd_mmc_ready_mb);
	
	xTaskCreate(mmcsd_detect, "mmcsd_detect", MMCSD_STACK_SIZE, 
		NULL, MMCSD_THREAD_PREORITY, &mmcsd_detect_thread);
#if DEVICE_TYPE_SELECT != EMMC_FLASH
    sdio_init();
#endif
	return 0;
}

struct mmcsd_card *mmcsd_get_sdmmc_card_info(void)
{
	return sdmmc_cardinfo;
}
