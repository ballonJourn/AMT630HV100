#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "carlink_utils.h"
#include <FreeRTOS.h>

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

void hex2str(const uint8_t *input, uint16_t input_len, char *output)
{
    char *hexEncode = "0123456789ABCDEF";
    int i = 0, j = 0;

    for (i = 0; i < input_len; i++) {
        output[j++] = hexEncode[(input[i] >> 4) & 0xf];
        output[j++] = hexEncode[(input[i]) & 0xf];
    }
}
#if 1
#define isDigit(c)             (((c) <= '9' && (c) >= '0') ? (1) : (0))

static uint8_t hex2dec(char hex)
{
	if (isDigit(hex)) {
		return (hex - '0');
	}
	if (hex >= 'a' && hex <= 'f') {
		return (hex - 'a' + 10);
	}
	if (hex >= 'A' && hex <= 'F') {
		return (hex - 'A' + 10);
	}

	return 0;
}

int string2hex(char *input, int input_len, char *output, int max_len)
{
	int             i = 0;
	uint8_t         ch0, ch1;

	if (input_len % 2 != 0) {
		return -1;
	}

	while (i < input_len / 2 && i < max_len) {
		ch0 = hex2dec((char)input[2 * i]);
		ch1 = hex2dec((char)input[2 * i + 1]);
		output[i] = (ch0 << 4 | ch1);
		i++;
	}
	return i;
}
#endif

void str2asiistr(const char *input, int input_len, char *output, int max_len)
{
    int             i = 0;

    if (max_len < 2 *input_len)
        return;

    while (i < input_len) {
        sprintf(output, "%02x", input[i]);
        output += 2;
        i++;
    }
}

bool is_power_of_2(uint32_t n)
{
	return (n != 0 && ((n & (n -1)) == 0));
}

// buffer_size musb be pow of 2
int	ring_buffer_init( ring_buffer_t *ctx, int buffer_size)
{
	void *base = NULL;
	if (ctx == NULL || !is_power_of_2(buffer_size))
		return -1;
	base = pvPortMalloc(buffer_size);

	ctx->buffer		= (uint8_t *)base;
	//ctx->end		= (uint8_t *)base + buffer_size;
	ctx->size		= (uint32_t) buffer_size;
	ctx->mask		= (uint32_t)( buffer_size - 1 );
	ctx->read_offset	= 0;
	ctx->write_offset	= 0;
	return 0;
}

void ring_buffer_free( ring_buffer_t *ctx )
{
	if (ctx == NULL || NULL == ctx->buffer)
		return;
	vPortFree(ctx->buffer);
}

uint32_t ring_buffer_write(ring_buffer_t *ctx, uint8_t *buffer, uint32_t len)
{
	uint32_t l;

	len = min(len, ctx->size - ctx->write_offset + ctx->read_offset);

	l = min(len, ctx->size - (ctx->write_offset & (ctx->mask)));
	memcpy(ctx->buffer + (ctx->write_offset & (ctx->mask)), buffer, l);
	memcpy(ctx->buffer, buffer + l, len - l);
	ctx->write_offset += len;
	return len;
}


uint32_t ring_buffer_read(ring_buffer_t *ctx, uint8_t *buffer, uint32_t len)
{
	uint32_t l;

	len = min(len, ctx->write_offset - ctx->read_offset);

	l = min(len, ctx->size - (ctx->read_offset & (ctx->mask)));
	memcpy(buffer, ctx->buffer + (ctx->read_offset & (ctx->mask)), l);
	memcpy(buffer + l, ctx->buffer, len - l);
	ctx->read_offset += len;
	return len;
}

