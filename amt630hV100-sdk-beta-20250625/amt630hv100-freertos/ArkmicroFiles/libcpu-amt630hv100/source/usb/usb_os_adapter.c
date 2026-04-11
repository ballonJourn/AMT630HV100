#include "usb_os_adapter.h"

void *kmem_cache_alloc(struct kmem_cache *obj, int flag)
{
	(void)flag;
	return pvPortMalloc(obj->sz);
}

void kmem_cache_free(struct kmem_cache *cachep, void *obj)
{
	(void)cachep;
	vPortFree(obj);
}

void kmem_cache_destroy(struct kmem_cache *cachep)
{
	free(cachep);
}

static void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	void* ptr = NULL;
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;
	ptr =  pvPortMalloc(n * size);

	if (flags & __GFP_ZERO) {
		memset(ptr, 0, n * size);
	}

	return ptr;
}

void *kcalloc(size_t n, size_t size, gfp_t flags)
{
	return kmalloc_array(n, size, flags | __GFP_ZERO);
}

void *kmalloc(size_t size, int flags)
{
	void *p;

	p = pvPortMalloc(size);
	if (flags & __GFP_ZERO)
		memset(p, 0, size);

	return p;
}

void *kzalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags | __GFP_ZERO);
}

//struct device;
void *devm_kzalloc(struct device *dev, size_t size, gfp_t flags)
{
	return kmalloc(size, flags | __GFP_ZERO);
}


void kfree(void* addr)
{
	vPortFree(addr);
}

struct kmem_cache *get_mem(int element_sz)
{
	struct kmem_cache *ret;

	ret = pvPortMalloc(sizeof(struct kmem_cache));
	ret->sz = element_sz;

	return ret;
}


static unsigned long _find_next_bit(const unsigned long *addr1,
		const unsigned long *addr2, unsigned long nbits,
		unsigned long start, unsigned long invert)
{
	unsigned long tmp;

	if (unlikely(start >= nbits))
		return nbits;

	tmp = addr1[start / BITS_PER_LONG];
	if (addr2)
		tmp &= addr2[start / BITS_PER_LONG];
	tmp ^= invert;

	/* Handle 1st word. */
	tmp &= BITMAP_FIRST_WORD_MASK(start);
	start = round_down(start, BITS_PER_LONG);

	while (!tmp) {
		start += BITS_PER_LONG;
		if (start >= nbits)
			return nbits;

		tmp = addr1[start / BITS_PER_LONG];
		if (addr2)
			tmp &= addr2[start / BITS_PER_LONG];
		tmp ^= invert;
	}

	return min(start + __ffs(tmp), nbits);
}
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
			    unsigned long offset)
{
	return _find_next_bit(addr, NULL, size, offset, 0UL);
}

unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size,
				 unsigned long offset)
{
	return _find_next_bit(addr, NULL, size, offset, ~0UL);
}

unsigned long bitmap_find_next_zero_area_off(unsigned long *map,
					     unsigned long size,
					     unsigned long start,
					     unsigned int nr,
					     unsigned long align_mask,
					     unsigned long align_offset)
{
	unsigned long index, end, i;
again:
	index = find_next_zero_bit(map, size, start);

	/* Align allocation */
	index = __ALIGN_MASK(index + align_offset, align_mask) - align_offset;

	end = index + nr;
	if (end > size)
		return end;
	i = find_next_bit(map, end, index);
	if (i < end) {
		start = i + 1;
		goto again;
	}
	return index;
}

unsigned long
bitmap_find_next_zero_area(unsigned long *map,
			   unsigned long size,
			   unsigned long start,
			   unsigned int nr,
			   unsigned long align_mask)
{
	return bitmap_find_next_zero_area_off(map, size, start, nr, align_mask, 0);
}

void bitmap_set(unsigned long *map, unsigned int start, int len)
{
	unsigned long *p = map + BIT_WORD(start);
	const unsigned int size = start + len;
	int bits_to_set = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned long mask_to_set = BITMAP_FIRST_WORD_MASK(start);

	while (len - bits_to_set >= 0) {
		*p |= mask_to_set;
		len -= bits_to_set;
		bits_to_set = BITS_PER_LONG;
		mask_to_set = ~0UL;
		p++;
	}
	if (len) {
		mask_to_set &= BITMAP_LAST_WORD_MASK(size);
		*p |= mask_to_set;
	}
}

void bitmap_clear(unsigned long *map, unsigned int start, int len)
{
	unsigned long *p = map + BIT_WORD(start);
	const unsigned int size = start + len;
	int bits_to_clear = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned long mask_to_clear = BITMAP_FIRST_WORD_MASK(start);

	while (len - bits_to_clear >= 0) {
		*p &= ~mask_to_clear;
		len -= bits_to_clear;
		bits_to_clear = BITS_PER_LONG;
		mask_to_clear = ~0UL;
		p++;
	}
	if (len) {
		mask_to_clear &= BITMAP_LAST_WORD_MASK(size);
		*p &= ~mask_to_clear;
	}
}

void writesl(u32 addr, const void *buffer, unsigned int count)
{
	if (count) {
		const u32 *buf = (u32 *)buffer;

		do {
			writel(*buf++, addr);
		} while (--count);
	}
}

void readsl(u32 addr, void *buffer, unsigned int count)
{
	if (count) {
		u32 *buf = (u32 *)buffer;

		do {
			u32 x = readl(addr);
			*buf++ = x;
		} while (--count);
	}
}

void iowrite32_rep(u32 addr, const void *buffer, unsigned int count)
{
	writesl(addr, buffer, count);
}

void ioread32_rep(u32 addr, void *buffer, unsigned int count)
{
	readsl(addr, buffer, count);
}

void put_unaligned_le16(u16 val, void *p)
{
    //   *((__le16 *)p) = cpu_to_le16(val);
    u8 *tmp = (u8*)p;
    tmp[0] = (val & 0xff);
    tmp[1] = ((val >> 8) & 0xff);
}

void put_unaligned_le32(u32 val, void *p)
{
	// *((__le32 *)p) = cpu_to_le32(val);
    u8 *tmp = (u8*)p;
    tmp[0] = (val & 0xff);
    tmp[1] = ((val >> 8) & 0xff);
    tmp[2] = ((val >> 16) & 0xff);
    tmp[3] = ((val >> 24) & 0xff);
}

int get_unaligned_le16(void *p)
{
	u8 *tmp = (u8*)p;

	return (tmp[1] << 8) | tmp[0];
}

int get_unaligned_le32(void *p)
{
	u8 *tmp = (u8*)p;

	return (tmp[3] << 24) | (tmp[2] << 16) | (tmp[1] << 8) | tmp[0];
}


void generic_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p  |= mask;
}

int is_zero_ether_addr(const u8 *addr)
{
	return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

int is_multicast_ether_addr(const u8 *addr)
{
	return 0x01 & addr[0];
}

int is_local_ether_addr(const u8 *addr)
{
	return 0x02 & addr[0];
}
#if 0
int is_broadcast_ether_addr(const u8 *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) == 0xff;
}
#endif
int is_unicast_ether_addr(const u8 *addr)
{
	return !is_multicast_ether_addr(addr);
}

int is_valid_ether_addr(const u8 *addr)
{
	return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}
#if 0
static inline void random_ether_addr(u8 *addr)
{
	get_random_bytes (addr, ETH_ALEN);
	addr [0] &= 0xfe;	/* clear multicast bit */
	addr [0] |= 0x02;	/* set local assignment bit (IEEE802) */
}
#endif
