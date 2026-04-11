#ifndef _SPI_H
#define _SPI_H

#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPI mode flags */
#define SPI_CPHA	BIT(0)			/* clock phase */
#define SPI_CPOL	BIT(1)			/* clock polarity */
#define SPI_MODE_0	(0|0)			/* (original MicroWire) */
#define SPI_MODE_1	(0|SPI_CPHA)
#define SPI_MODE_2	(SPI_CPOL|0)
#define SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
#define SPI_CS_HIGH	BIT(2)			/* CS active high */
#define SPI_LSB_FIRST	BIT(3)			/* per-word bits-on-wire */
#define SPI_3WIRE	BIT(4)			/* SI/SO signals shared */
#define SPI_LOOP	BIT(5)			/* loopback mode */
#define SPI_SLAVE	BIT(6)			/* slave mode */
#define SPI_PREAMBLE	BIT(7)			/* Skip preamble bytes */
#define SPI_TX_BYTE	BIT(8)			/* transmit with 1 wire byte */
#define SPI_TX_DUAL	BIT(9)			/* transmit with 2 wires */
#define SPI_TX_QUAD	BIT(10)			/* transmit with 4 wires */
#define SPI_RX_SLOW	BIT(11)			/* receive with 1 wire slow */
#define SPI_RX_DUAL	BIT(12)			/* receive with 2 wires */
#define SPI_RX_QUAD	BIT(13)			/* receive with 4 wires */
#define SPI_READY	BIT(14)			/* Slave pulls low to pause */
#define SPI_NO_CS	BIT(15)			/* No chipselect */

#define SPI_DEFAULT_WORDLEN	8

/**
 * SPI message structure
 */
struct spi_message
{
    const void *send_buf;
    void *recv_buf;
    size_t length;
    struct spi_message *next;

    unsigned cs_take    : 1;
    unsigned cs_release : 1;
};

struct qspi_message
{
    struct spi_message message;

    /* instruction stage */
    struct
    {
        uint8_t content;
        uint8_t qspi_lines;
    } instruction;

    /* address and alternate_bytes stage */
    struct
    {
        uint32_t content;
        uint8_t size;
        uint8_t qspi_lines;
    } address, alternate_bytes;

    /* dummy_cycles stage */
    uint32_t dummy_cycles;

    /* number of lines in qspi data stage, the other configuration items are in parent */
    uint8_t qspi_data_lines;
};

/**
 * SPI configuration structure
 */
struct spi_configuration
{
    uint32_t mode;
    uint32_t data_width;
    uint32_t max_hz;
	uint32_t qspi_max_hz;
	uint32_t reserved;
};

/**
 * struct spi_slave - Representation of a SPI slave
 */
struct spi_slave {
	unsigned int bus;
	unsigned int cs;
	unsigned int mode;
	unsigned int wordlen;
	int (*xfer)(struct spi_slave *slave, struct spi_message *message);
	int (*qspi_read)(struct spi_slave *slave, struct qspi_message *qspi_message);
	int (*configure)(struct spi_slave *slave, struct spi_configuration *configuration);
	SemaphoreHandle_t xMutex;
	SemaphoreHandle_t xSfudMutex;
	int open_count;
	char name[16];
};

int ecspi_init(void);
int dwspi_init(void);
void spi_init(void);
int spi_add_slave(struct spi_slave *slave);
struct spi_slave *spi_open(const char *spidev);
void spi_close(struct spi_slave *slave);
int spi_send_then_recv(struct spi_slave *slave, const void *send_buf, 
								size_t send_length, void *recv_buf, 
								size_t recv_length);
int spi_transfer(struct spi_slave *slave, const void	 *send_buf,
						  void *recv_buf, size_t length);
int spi_configure(struct spi_slave *slave, struct spi_configuration *cfg);
int spi_recv(struct spi_slave *slave, void *recv_buf, size_t length);
int spi_send(struct spi_slave *slave, const void *send_buf, size_t length);

#ifdef __cplusplus
}
#endif

#endif
