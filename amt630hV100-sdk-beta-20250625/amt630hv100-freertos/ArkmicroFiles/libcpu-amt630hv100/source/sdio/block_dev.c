#include <string.h>

#include "FreeRTOS.h"
#include "trace.h"

#include "mmcsd_core.h"

#define SECTOR_SIZE                512

int32_t mmcsd_num_wr_blocks(struct mmcsd_card *card)
{
    int32_t err;
    uint32_t blocks;

    struct mmcsd_req req;
    struct mmcsd_cmd cmd;
    struct mmcsd_data data;
    uint32_t timeout_us;

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = APP_CMD;
    cmd.arg = card->rca << 16;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;

    err = mmcsd_send_cmd(card->host, &cmd, 0);
    if (err)
        return -1;
    if (!controller_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
        return -1;

    memset(&cmd, 0, sizeof(struct mmcsd_cmd));

    cmd.cmd_code = SD_APP_SEND_NUM_WR_BLKS;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    memset(&data, 0, sizeof(struct mmcsd_data));

    data.timeout_ns = card->tacc_ns * 100;
    data.timeout_clks = card->tacc_clks * 100;

    timeout_us = data.timeout_ns / 1000;
    timeout_us += data.timeout_clks * 1000 /
        (card->host->io_cfg.clock / 1000);

    if (timeout_us > 100000) 
    {
        data.timeout_ns = 100000000;
        data.timeout_clks = 0;
    }

    data.blksize = 4;
    data.blks = 1;
    data.flags = DATA_DIR_READ;
    data.buf = &blocks;

    memset(&req, 0, sizeof(struct mmcsd_req));

    req.cmd = &cmd;
    req.data = &data;

    mmcsd_send_request(card->host, &req);

    if (cmd.err || data.err)
        return -1;

    return blocks;
}

int mmcsd_req_blk(struct mmcsd_card *card,
                                 uint32_t           sector,
                                 void                 *buf,
                                 size_t             blks,
                                 uint8_t            dir)
{
    struct mmcsd_cmd  cmd, stop;
    struct mmcsd_data  data;
    struct mmcsd_req  req;
    struct mmcsd_host *host = card->host;
    uint32_t r_cmd, w_cmd;

    mmcsd_host_lock(host);
    memset(&req, 0, sizeof(struct mmcsd_req));
    memset(&cmd, 0, sizeof(struct mmcsd_cmd));
    memset(&stop, 0, sizeof(struct mmcsd_cmd));
    memset(&data, 0, sizeof(struct mmcsd_data));
    req.cmd = &cmd;
    req.data = &data;
    
    cmd.arg = sector;
    if (!(card->flags & CARD_FLAG_SDHC)) 
    {
        cmd.arg <<= 9;
    }
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    data.blksize = SECTOR_SIZE;
    data.blks  = blks;

    if (blks > 1) 
    {
        if (!controller_is_spi(card->host) || !dir)
        {
            req.stop = &stop;
            stop.cmd_code = STOP_TRANSMISSION;
            stop.arg = 0;
            stop.flags = RESP_SPI_R1B | RESP_R1B | CMD_AC;
        }
        r_cmd = READ_MULTIPLE_BLOCK;
        w_cmd = WRITE_MULTIPLE_BLOCK;
    }
    else
    {
        req.stop = NULL;
        r_cmd = READ_SINGLE_BLOCK;
        w_cmd = WRITE_BLOCK;
    }

    if (!dir) 
    {
        cmd.cmd_code = r_cmd;
        data.flags |= DATA_DIR_READ;
    }
    else
    {
        cmd.cmd_code = w_cmd;
        data.flags |= DATA_DIR_WRITE;
    }

    mmcsd_set_data_timeout(&data, card);
    data.buf = buf;
    mmcsd_send_request(host, &req);

    if (!controller_is_spi(card->host) && dir != 0) 
    {
        do 
        {
            int32_t err;

            cmd.cmd_code = SEND_STATUS;
            cmd.arg = card->rca << 16;
            cmd.flags = RESP_R1 | CMD_AC;
            err = mmcsd_send_cmd(card->host, &cmd, 5);
            if (err) 
            {
                TRACE_ERROR("error %d requesting status", err);
                break;
            }
            /*
             * Some cards mishandle the status bits,
             * so make sure to check both the busy
             * indication and the card state.
             */
         } while (!(cmd.resp[0] & R1_READY_FOR_DATA) ||
            (R1_CURRENT_STATE(cmd.resp[0]) == 7));
    }

    mmcsd_host_unlock(host);

    if (cmd.err || data.err || stop.err) 
    {
        TRACE_ERROR("mmcsd request blocks error");
        TRACE_ERROR("%d,%d,%d, 0x%08x,0x%08x",
                   cmd.err, data.err, stop.err, data.flags, sector);

        return -1;
    }

    return 0;
}

int32_t mmcsd_set_blksize(struct mmcsd_card *card)
{
    struct mmcsd_cmd cmd;
    int err;

    /* Block-addressed cards ignore MMC_SET_BLOCKLEN. */
    if (card->flags & CARD_FLAG_SDHC)
        return 0;

    mmcsd_host_lock(card->host);
    cmd.cmd_code = SET_BLOCKLEN;
    cmd.arg = 512;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;
    err = mmcsd_send_cmd(card->host, &cmd, 5);
    mmcsd_host_unlock(card->host);

    if (err) 
    {
        TRACE_ERROR("MMCSD: unable to set block size to %d: %d", cmd.arg, err);

        return -1;
    }

    return 0;
}
