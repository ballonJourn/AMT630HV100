#include "FreeRTOS.h"
#include "board.h"
#include "sfud.h"
#include "lfs.h"

static uint32_t lfs_startblk;
static lfs_t sufd_lfs;
static sfud_flash *lfs_sf;

static int lfs_sfud_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{
	block += lfs_startblk;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	if (sfud_read(lfs_sf, block * c->block_size + off, size, buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
#else
	if (emmc_read(block * c->block_size + off, size, buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
#endif
}

static int lfs_sfud_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
	block += lfs_startblk;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	if (sfud_write(lfs_sf, block * c->block_size + off, size, buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
#endif
}

static int lfs_sfud_erase(const struct lfs_config *c, lfs_block_t block)
{
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	return LFS_ERR_OK;
#endif
	block += lfs_startblk;
	if (sfud_erase(lfs_sf, block * c->block_size, c->block_size) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
}

static int lfs_sfud_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
}

static struct lfs_config sfud_cfg =
{
	.read = lfs_sfud_read,
	.prog = lfs_sfud_prog,
	.erase = lfs_sfud_erase,
	.sync = lfs_sfud_sync,
	.read_size = 256,
	.prog_size = 256,
	.block_size = 4096,
	.block_count = OTA_MEDIA_SIZE / 4096,
	.block_cycles = 500,
	.cache_size = 4096,
	.lookahead_size  = 128,
};

int lfs_sfud_init(void)
{
	int err = 0;

	lfs_sf = sfud_get_device(0);
	if (!lfs_sf)
		return -1;
	lfs_startblk = OTA_MEDIA_OFFSET / 4096;
	err = lfs_mount(&sufd_lfs, &sfud_cfg);
	if (err) {
		lfs_format(&sufd_lfs, &sfud_cfg);
		err = lfs_mount(&sufd_lfs, &sfud_cfg);
		if (err)
			printf("lfs mount fail. err=%d.\n", err);
		else
			printf("lfs mount ok.\n");
	} else {
		printf("lfs mount ok.\n");
	}

	return err;
}

#define TEST_FILE_SIZE	0x600000
#define TEST_READ_SIZE	16834
void lfs_demo(void)
{
	int err;
	lfs_file_t file;
	uint32_t size;
	uint32_t leftsize;
	uint32_t time;
	uint8_t *wbuf = NULL, *rbuf = NULL;
	uint8_t *buf;
	int i;

	err = lfs_sfud_init();
	if (err) {
		printf("lfs_sfud_init fail.\n");
		return;
	}

	wbuf = pvPortMalloc(TEST_FILE_SIZE);
	if (!wbuf)
		return;

	rbuf = pvPortMalloc(TEST_FILE_SIZE);
	if (!rbuf) {
		vPortFree(wbuf);
		return;
	}

	for (i = 0; i < TEST_FILE_SIZE; i++) {
		wbuf[i] = rand();
		rbuf[i] = 0;
	}


	err = lfs_file_open(&sufd_lfs, &file, "test.bin", LFS_O_RDONLY);
	if (!err) {
		lfs_file_close(&sufd_lfs, &file);
		lfs_remove(&sufd_lfs, "test.bin");
	}

	err = lfs_file_open(&sufd_lfs, &file, "test.bin", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
	if (!err) {
		buf = wbuf;
		leftsize = TEST_FILE_SIZE;
		while (leftsize > 0) {
			int wsize;
			size = leftsize > TEST_READ_SIZE ? TEST_READ_SIZE : leftsize;
			time = get_timer(0);
			wsize = lfs_file_write(&sufd_lfs, &file, buf, size);
			time = get_timer(time);
			printf("wtime %d us.\n", time);
			if (wsize != size)
				printf("lfs_file_write err=%d, leftsize=%d.\n", wsize, leftsize);
			if (wsize > 0) {
				leftsize -= wsize;
				buf += wsize;
			}
		}
	}
	lfs_file_close(&sufd_lfs, &file);

	err = lfs_file_open(&sufd_lfs, &file, "test.bin", LFS_O_RDONLY);
	if (!err) {
		buf = rbuf;
		leftsize = TEST_FILE_SIZE;
		while (leftsize > 0) {
			int rsize;
			size = leftsize > TEST_READ_SIZE ? TEST_READ_SIZE : leftsize;
			time = get_timer(0);
			rsize = lfs_file_read(&sufd_lfs, &file, buf, size);
			time = get_timer(time);
			printf("rtime %d us.\n", time);
			if (rsize != size)
				printf("lfs_file_read err=%d, leftsize=%d.\n", rsize, leftsize);
			if (rsize > 0) {
				leftsize -= rsize;
				buf += rsize;
			}
		}
	}
	lfs_file_close(&sufd_lfs, &file);

	for (i = 0; i < TEST_FILE_SIZE; i++) {
		if (wbuf[i] != rbuf[i]) {
			printf("lfs rw fail %d,0x%x,0x%x.\n", i, wbuf[i], rbuf[i]);
		}
	}

	vPortFree(wbuf);
	vPortFree(rbuf);
}
