#ifndef __CARLINK_UTILS_H
#define __CARLINK_UTILS_H

void hex2str(const uint8_t *input, uint16_t input_len, char *output);
int string2hex(char *input, int input_len, char *output, int max_len);
void str2asiistr(const char *input, int input_len, char *output, int max_len);

typedef struct
{
	uint8_t *			buffer;
	//const uint8_t *	end;
	uint32_t			size;
	uint32_t			mask;
	uint32_t			read_offset;
	uint32_t			write_offset;
} ring_buffer_t;

int	ring_buffer_init( ring_buffer_t *ctx, int buffer_size);
void ring_buffer_free( ring_buffer_t *ctx );
uint32_t ring_buffer_write(ring_buffer_t *ctx, uint8_t *buffer, uint32_t len);
uint32_t ring_buffer_read(ring_buffer_t *ctx, uint8_t *buffer, uint32_t len);

/*#define ring_buffer_get_read_ptr( RING )			( &(RING)->buffer[ (RING)->read_offset  & (RING)->mask ] )
#define ring_buffer_get_write_ptr( RING )			( &(RING)->buffer[ (RING)->write_offset & (RING)->mask ] )
#define ring_buffer_read_advance( RING, COUNT )	do { (RING)->read_offset  += (COUNT); } while( 0 )
#define ring_buffer_write_advance( RING, COUNT )	do { (RING)->write_offset += (COUNT); } while( 0 )
#define ring_buffer_reset( RING )					do { (RING)->read_offset = (RING)->write_offset; } while( 0 )*/
#define ring_buffer_get_bytes_used( RING )			( (uint32_t)( (RING)->write_offset - (RING)->read_offset ) )
#define ring_buffer_get_bytes_free( RING )			( (RING)->size - ring_buffer_get_bytes_used( RING ) )

#endif
