#ifndef _OS_ADAPT_H
#define _OS_ADAPT_H

#include <string.h>
#include "FreeRTOS.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif
  
#define __INLINE	inline

#define PAGE_SIZE			4096
#define ARCH_DMA_MINALIGN	32

#define USEC_PER_MSEC	1000
#define MSEC_PER_SEC	1000

#define BUG()	printf("bug on %s %d.\n", __func__, __LINE__);
#define BUG_ON(condition) if (condition) BUG()
#define WARN_ON(condition) if (condition) BUG()

#define barrier()
#define wmb()
#define EXPORT_SYMBOL(x)

#define dev_dbg(dev, ...)	TRACE_DEBUG(__VA_ARGS__)
#define dev_vdbg	dev_dbg
#define dev_info(dev, ...)	TRACE_INFO(__VA_ARGS__)
#define dev_warn(dev, ...)	TRACE_WARNING(__VA_ARGS__)
#define dev_err(dev, ...)	TRACE_ERROR(__VA_ARGS__)

#define __iomem		volatile

#define unlikely(x)		(x)
#define likely(x)		(x)

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_UP_ULL(ll, d)		(((unsigned long long)(ll) + (d) - 1) / (d))


#define BITS_PER_BYTE		8
#define BITS_PER_LONG 		32

#define BIT(nr)			(1UL << (nr))
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define min(x,y) ((x)<(y)?(x):(y))

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. */
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define VOID	void
#define BOOL	int
#define TRUE	1
#define FALSE	0

#define FAR
#define NEAR

typedef unsigned long long	u64;
typedef	uint32_t UINT32;
typedef	uint16_t UINT16;
typedef	uint8_t UINT8;
typedef int32_t	INT32;
typedef int16_t INT16;
typedef int8_t INT8;
typedef char CHAR;
typedef	uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
//typedef s32	ssize_t;
typedef u32  __le32;
typedef UINT32	AARCHPTR;
typedef u32 dma_addr_t;
typedef u32 phys_addr_t;
typedef phys_addr_t resource_size_t;
typedef unsigned gfp_t;

#define le16_to_cpu(x)	(x)
#define le32_to_cpu(x)	(x)
#define cpu_to_le32(x)	(x)

#define ___constant_swab32(x) ((u32)(				\
	(((u32)(x) & (u32)0x000000ffUL) << 24) |		\
	(((u32)(x) & (u32)0x0000ff00UL) <<  8) |		\
	(((u32)(x) & (u32)0x00ff0000UL) >>  8) |		\
	(((u32)(x) & (u32)0xff000000UL) >> 24)))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

typedef struct {
	int counter;
} atomic_t;

typedef struct refcount_struct {
	atomic_t refs;
} refcount_t;

#define REFCOUNT_INIT(n)	{ .refs = ATOMIC_INIT(n), }

struct kref {
	refcount_t refcount;
};

#define KREF_INIT(n)	{ .refcount = REFCOUNT_INIT(n), }

#define small_const_nbits(nbits) \
	(nbits && (nbits) <= BITS_PER_LONG)


#define reg8_read(addr)       *((volatile uint8_t *)(addr))
#define reg16_read(addr)      *((volatile uint16_t *)(addr))
#define reg32_read(addr)      *((volatile uint32_t *)(addr))

#define reg8_write(addr,val)  *((volatile uint8_t *)(addr)) = (val)
#define reg16_write(addr,val) *((volatile uint16_t *)(addr)) = (val)
#define reg32_write(addr,val) *((volatile uint32_t *)(addr)) = (val)

#define mem8_read(addr)       *((volatile uint8_t *)(addr))
#define mem16_read(addr)      *((volatile uint16_t *)(addr))
#define mem32_read(addr)      *((volatile uint32_t *)(addr))

#define mem8_write(addr,val)  *((volatile uint8_t *)(addr)) = (val)
#define mem16_write(addr,val) *((volatile uint16_t *)(addr)) = (val)
#define mem32_write(addr,val) *((volatile uint32_t *)(addr)) = (val)

#define readb(a)        reg8_read(a)
#define readw(a)        reg16_read(a)
#define readl(a)        reg32_read(a)

#define writeb(v, a)    reg8_write(a, v)
#define writew(v, a)    reg16_write(a, v)
#define writel(v, a)    reg32_write(a, v)

static __INLINE void memset_s(void *dest, size_t destMax, int c, size_t count)
{
	memset(dest, c, destMax); 
}

static __INLINE void bitmap_zero(unsigned long *dst, unsigned int nbits)
{
	if (small_const_nbits(nbits))
		*dst = 0UL;
	else {
		unsigned int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
		memset(dst, 0, len);
	}
}

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static __INLINE int fls(u32 x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

/*
 * __fls() returns the bit position of the last bit set, where the
 * LSB is 0 and MSB is 31.  Zero input is undefined.
 */
static __INLINE unsigned long __fls(unsigned long x)
{
	return fls(x) - 1;
}

/*
 * ffs() returns zero if the input was zero, otherwise returns the bit
 * position of the first set bit, where the LSB is 1 and MSB is 32.
 */
static __INLINE int ffs(int x)
{
	return fls(x & -x);
}


/*
 * __ffs() returns the bit position of the first bit set, where the
 * LSB is 0 and MSB is 31.  Zero input is undefined.
 */
static __INLINE unsigned long __ffs(unsigned long x)
{
	return ffs(x) - 1;
}


#define MAX_ERRNO	4095
#define IS_ERR_VALUE(x) (x) >= (unsigned long)-MAX_ERRNO

static __INLINE void *ERR_PTR(long error)
{
	return (void *) error;
}

static __INLINE long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static __INLINE long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static __INLINE int IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

static __INLINE void set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p	|= mask;
}

static __INLINE void clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p &= ~mask;
}

/**
 * test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static __INLINE int test_bit(int nr, const volatile unsigned long *addr)
{
	return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It may be reordered on other architectures than x86.
 * It also implies a memory barrier.
 */
static __INLINE int test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;

	old = *p;
	*p = old | mask;

	return (old & mask) != 0;
}

static __INLINE int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;

	old = *p;
	*p = old & ~mask;

	return (old & mask) != 0;
}


/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static __INLINE void list_splice_init(List_t *list, List_t *head)
{
	ListItem_t *pxIndex = head->pxIndex;
	ListItem_t *first = list->pxIndex->pxNext;
	ListItem_t *last = list->pxIndex->pxPrevious;
	
	if (!listLIST_IS_EMPTY(list)) {
		first->pxPrevious = pxIndex->pxPrevious;
		last->pxNext = pxIndex;	
		pxIndex->pxPrevious->pxNext = first;
		pxIndex->pxPrevious = last;
		head->uxNumberOfItems += list->uxNumberOfItems;
		vListInitialise(list);
	}
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static __INLINE void list_move(ListItem_t *item, List_t *list, ListItem_t *pos)
{
	void *pvOwner = item->pvOwner;
	
	uxListRemove(item);
	item->pvOwner = pvOwner;
	item->pxNext = pos->pxNext;
	item->pxNext->pxPrevious = item;
	item->pxPrevious = pos;
	pos->pxNext = item;


	/* Remember which list the item is in.  This allows fast removal of the
	item later. */
	item->pxContainer = list;

	list->uxNumberOfItems++;

}

#define list_for_each_entry(pxListItem, pvOwner, list)	\
	for (pxListItem = listGET_HEAD_ENTRY(list),	\
		 pvOwner = listGET_LIST_ITEM_OWNER(pxListItem);	\
	     pxListItem != listGET_END_MARKER(list); 	\
	     pxListItem = listGET_NEXT(pxListItem), \
	     pvOwner = listGET_LIST_ITEM_OWNER(pxListItem))

#define list_entry(pxListItem)	listGET_LIST_ITEM_OWNER(pxListItem)

#define list_first_entry(pxList) listGET_LIST_ITEM_OWNER(listGET_HEAD_ENTRY(pxList))


#define SG_MITER_ATOMIC		(1 << 0)	 /* use kmap_atomic */
#define SG_MITER_TO_SG		(1 << 1)	/* flush back to phys on unmap */
#define SG_MITER_FROM_SG	(1 << 2)	/* nop */

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
