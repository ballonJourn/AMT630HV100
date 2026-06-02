#include <stdio.h>
#include <stdlib.h>

#define CRC32_POLY 0x04C11DB7L
 
unsigned int get_sum_poly(unsigned char top_byte)
{
    /// sum all the polys at various offsets 
    unsigned int sum_poly = top_byte << 24;
	int i;

    for (i = 0; i < 8; ++i) {
        /// check the top bit
        if( ( sum_poly >> 31 ) != 0 )
            /// TODO : understand why '<<' first
            sum_poly = ( sum_poly << 1 ) ^ CRC32_POLY;
        else
            sum_poly <<= 1;
    }
 
    return sum_poly;
}
 
void create_table(unsigned int *table)
{
    for( int i = 0; i < 256; ++ i )
    {
        table[i] = get_sum_poly( (unsigned char) i );
    }
}

static const unsigned int *crc32_table = NULL;

unsigned int
xcrc32(const unsigned char *buf, int len, unsigned int init)
{
	unsigned int crc = init;
	static unsigned int table[256];

	if (!crc32_table) {
		create_table(table);
		crc32_table = &table[0];
	}
	
	while (len--)
	{
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}
