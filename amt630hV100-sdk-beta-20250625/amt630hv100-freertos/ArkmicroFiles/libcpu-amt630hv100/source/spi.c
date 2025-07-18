#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "errno.h"

#define MAX_SPI_DEVICE_NUM		2

static struct spi_slave *spi_devs[MAX_SPI_DEVICE_NUM] = {NULL};
static int spi_devices_count = 0;

int spi_add_slave(struct spi_slave *slave)
{
	if (spi_devices_count >= MAX_SPI_DEVICE_NUM)
		return -1;

	spi_devs[spi_devices_count++] = slave;

	return 0;
}

struct spi_slave *spi_open(const char *spidev)
{
	struct spi_slave *slave;
	int i;

	for (i = 0; i < spi_devices_count; i++) {
		slave = spi_devs[i];
		if (!strcmp(slave->name, spidev)) {
			slave->open_count++;
			if (slave->open_count == 1)
				slave->xMutex = xSemaphoreCreateMutex();
			return slave;
		}
	}

	return NULL;
}

void spi_close(struct spi_slave *slave)
{
 	if (slave && --slave->open_count == 0)
		vSemaphoreDelete(slave->xMutex);
}

int spi_send_then_recv(struct spi_slave *slave, const void *send_buf, 
								size_t send_length, void *recv_buf, 
								size_t recv_length)
{
    int result;
    struct spi_message message = {0};

    /* send data */
    message.send_buf   = send_buf;
    message.recv_buf   = NULL;
    message.length     = send_length;
    message.cs_take    = 1;
    message.cs_release = 0;
    message.next       = NULL;

    result = slave->xfer(slave, &message);
    if (result < 0)
    {
        result = -EIO;
        goto __exit;
    }

    /* recv data */
    message.send_buf   = NULL;
    message.recv_buf   = recv_buf;
    message.length     = recv_length;
    message.cs_take    = 0;
    message.cs_release = 1;
    message.next       = NULL;

    result = slave->xfer(slave, &message);
    if (result < 0)
    {
        result = -EIO;
        goto __exit;
    }

    result = ENOERR;

__exit:
    return result;
}

int spi_transfer(struct spi_slave *slave, const void *send_buf,
                          void *recv_buf, size_t length)
{
    int result;
    struct spi_message message = {0};

    configASSERT(slave != NULL);

    xSemaphoreTake(slave->xMutex, portMAX_DELAY);

    /* initial message */
    message.send_buf   = send_buf;
    message.recv_buf   = recv_buf;
    message.length     = length;
    message.cs_take    = 1;
    message.cs_release = 1;
    message.next       = NULL;

    /* transfer message */
    result = slave->xfer(slave, &message);
    if (result < 0)
    {
        result = -EIO;
        goto __exit;
    }

__exit:
    xSemaphoreGive(slave->xMutex);

    return result;
}


int spi_configure(struct spi_slave *slave, struct spi_configuration *cfg)
{
	int ret;
	
	configASSERT(slave && cfg);
	
	xSemaphoreTake(slave->xMutex, portMAX_DELAY);

	ret = slave->configure(slave, cfg);

	xSemaphoreGive(slave->xMutex);

	return ret;

}

int spi_recv(struct spi_slave *slave, void *recv_buf, size_t length)
{
    return spi_transfer(slave, NULL, recv_buf, length);
}

int spi_send(struct spi_slave *slave, const void *send_buf, size_t length)
{
    return spi_transfer(slave, send_buf, NULL, length);
}

void spi_init(void)
{
#ifdef SPI1_SUPPORT
	ecspi_init();
#endif
	dwspi_init();
}
