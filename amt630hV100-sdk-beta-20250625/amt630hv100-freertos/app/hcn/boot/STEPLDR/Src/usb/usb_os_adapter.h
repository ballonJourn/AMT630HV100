#ifndef _USB_OS_ADAPTER_H
#define _USB_OS_ADAPTER_H

#include "os_adapt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define pr_err(...)         TRACE_ERROR(__VA_ARGS__)
#define WARN_ON_ONCE(condition) WARN_ON(condition)

#define USB_UNUSED(x) ((void) x)

typedef INT8 __s8;
typedef UINT8 __u8;
typedef INT16 __s16;
typedef UINT16 __u16;
typedef INT32 __s32;
typedef UINT32 __u32;
typedef long long __s64;
typedef unsigned long long __u64;

typedef __u16  __le16;
typedef __u16  __be16;
//typedef __u32  __le32;
typedef __u32  __be32;
typedef __u64  __le64;
typedef __u64  __be64;

typedef unsigned long ulong;
typedef __u32 dev_t;


#define __cpu_to_le16(x) (x)
#define cpu_to_le16(x)  (x)
#define __le16_to_cpu    le16_to_cpu
#define get_unaligned(x) (*x)


void iowrite32_rep(u32 addr, const void *buffer, unsigned int count);
void ioread32_rep(u32 addr, void *buffer, unsigned int count);
unsigned long bitmap_find_next_zero_area(unsigned long *map,
			   unsigned long size,
			   unsigned long start,
			   unsigned int nr,
			   unsigned long align_mask);
void bitmap_set(unsigned long *map, unsigned int start, int len);
void bitmap_clear(unsigned long *map, unsigned int start, int len);

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(ListItem_t *item, List_t *list)
{
	if (!listIS_CONTAINED_WITHIN(NULL, item))
		uxListRemove(item);
	vListInsertEnd(list, item);
}

static inline void list_del_init(ListItem_t *item)
{
	if (!listIS_CONTAINED_WITHIN(NULL, item))//maybe item has removed from list
		uxListRemove(item);
	//vListInitialiseItem(item);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(ListItem_t *item, List_t *list)
{
	vListInsertEnd(list, item);
}

static inline void INIT_LIST_HEAD(List_t *list)
{
	vListInitialise(list);
}

static inline void INIT_LIST_ITEM(ListItem_t *item)
{
	vListInitialiseItem(item);
}

#define list_for_each_entry_safe(pxListItem, nListItem, pvOwner, list)	\
		for (pxListItem = listGET_HEAD_ENTRY(list), \
			 nListItem = listGET_NEXT(pxListItem), \
			 pvOwner = listGET_LIST_ITEM_OWNER(pxListItem); \
			 pxListItem != listGET_END_MARKER(list);	\
			 pxListItem = nListItem, \
			 nListItem = listGET_NEXT(pxListItem), \
			 pvOwner = listGET_LIST_ITEM_OWNER(pxListItem))

#define list_for_each_safe(pxListItem, nListItem, list) \
	for (pxListItem = listGET_HEAD_ENTRY(list), \
			 nListItem = listGET_NEXT(pxListItem); \
			 pxListItem != listGET_END_MARKER(list);	\
			 pxListItem = nListItem, \
			 nListItem = listGET_NEXT(pxListItem))

#define list_del(pxListItem)   uxListRemove(pxListItem)
#define list_empty(pxList)	listLIST_IS_EMPTY(pxList)
#define list_item_empty(pxListItem)  ((pxListItem)->pxContainer == NULL)

#define __ARG_PLACEHOLDER_1 0,
#define config_enabled(cfg) _config_enabled(cfg)
#define _config_enabled(value) __config_enabled(__ARG_PLACEHOLDER_##value)
#define __config_enabled(arg1_or_junk) ___config_enabled(arg1_or_junk 1, 0)
#define ___config_enabled(__ignored, val, ...) val

/*
 * IS_ENABLED(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y' or 'm',
 * 0 otherwise.
 *
 */
#define IS_ENABLED(option) \
	(config_enabled(option) || config_enabled(option##_MODULE))
struct timer_list
{
	int a;
};

struct unused {
  int a;
};
typedef struct unused unused_t;


#define task_pid_nr(x)			0
#define set_freezable(...)		do { } while (0)
#define try_to_freeze(...)		0
#define set_current_state(...)		do { } while (0)
#define kthread_should_stop(...)	0
#define schedule()			do { } while (0)

#define setup_timer(timer, func, data) do {} while (0)
#define del_timer_sync(timer) do {} while (0)
#define schedule_work(work) do {} while (0)
#define INIT_WORK(work, fun) do {} while (0)
#define local_irq_save(flag) do {(void)flag;} while (0)
#define local_irq_restore(flag) do {(void)flag;} while (0)

//#define local_irq_save(flag) do {portENTER_CRITICAL(); (void)flag;} while(0)
//#define local_irq_restore(flag) do {portEXIT_CRITICAL(); (void)flag;} while(0)


struct work_struct {
int a;
};

struct kmem_cache { 
	int sz; 
};

typedef int	wait_queue_head_t;


typedef struct {
	volatile unsigned int slock;
} spinlock_t;

static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	//TODO
	unsigned volatile long y;
	switch(size){
	case 1:
		y = (*(char *)ptr) & 0x000000ff;
		*((char *)ptr) = (char)x;
		break;
	case 2:
		y = (*(short *)ptr) & 0x0000ffff;
		*((short *)ptr) = (short)x;
		break;
	default: // 4
		y = (*(unsigned long *)ptr) & 0xffffffff;
		*((unsigned long *)ptr) = x;
		break;
	}
    return y;
}

#define ARCH_SPIN_LOCK_UNLOCKED  1
#define arch_spin_is_locked(x)	(*(volatile signed char *)(&(x)->slock) <= 0)
#define arch_spin_unlock_wait(x) do { barrier(); } while(spin_is_locked(x))
#define xchg(ptr,v) ((unsigned int)__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
#define __xg(x) ((volatile long *)(x))

static inline void _raw_spin_unlock(spinlock_t *lock)
{
	xchg(&lock->slock, 1);
}

static inline int _raw_spin_trylock(spinlock_t *lock)
{
	return xchg(&lock->slock, 0) != 0 ? 1 : 0;
}

static inline void _raw_spin_lock(spinlock_t *lock)
{
	volatile int was_locked;
	do {
		was_locked = xchg(&lock->slock, 0) == 0 ? 1 : 0;
	} while(was_locked);
}

#define SPINLOCK_MAGIC	0xdead4ead
#define SPIN_LOCK_UNLOCKED ARCH_SPIN_LOCK_UNLOCKED
#define spin_lock_init(x)	do { (x)->slock = SPIN_LOCK_UNLOCKED; } while(0)
#define spin_is_locked(x)	arch_spin_is_locked(x)
#define spin_unlock_wait(x)	arch_spin_unlock_wait(x)
#define _spin_trylock(lock)     ({_raw_spin_trylock(lock) ? \
                                1 : ({ 0;});})
#define _spin_lock(lock)        \
do {                            \
        _raw_spin_lock(lock);   \
} while(0)

#define _spin_unlock(lock)      \
do {                            \
        _raw_spin_unlock(lock); \
} while (0)

#define spin_lock(lock)       _spin_lock(lock)
#define spin_unlock(lock)       _spin_unlock(lock)
#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED


#define spin_lock_irqsave(lock, flags)      \
do {                                        \
	flags = __get_interrupt_state(); __disable_irq(); \
} while (0)

#define spin_unlock_irqrestore(lock, flags) \
do {                                        \
	__set_interrupt_state(flags);			\
} while (0)

#define assert_spin_locked(lock) do {} while (0)

#define  irqreturn_t int
#define IRQ_NONE 0
#define IRQ_HANDLED 1
#define IRQ_WAKE_THREAD 2
#define GFP_ATOMIC ((gfp_t) 0)
#define GFP_KERNEL ((gfp_t) 0)
#define GFP_NOFS ((gfp_t) 0)
#define GFP_USER ((gfp_t) 0)
#define __GFP_NOWARN ((gfp_t) 0)
#define __GFP_ZERO	((gfp_t)0x8000u)
#define UINT_MAX   (~0U)

void *kmem_cache_alloc(struct kmem_cache *obj, int flag);
void kmem_cache_free(struct kmem_cache *cachep, void *obj);
void kmem_cache_destroy(struct kmem_cache *cachep);

void *kcalloc(size_t n, size_t size, gfp_t flags);
void *kmalloc(size_t size, int flags);
void *kzalloc(size_t size, gfp_t flags);
void kfree(void* addr);
struct device;
void *devm_kzalloc(struct device *dev, size_t size, gfp_t flags);
struct kmem_cache *get_mem(int element_sz);
#define kmem_cache_create(a, sz, c, d, e)	get_mem(sz)



#define min_t(type, x, y) (x < y ? x: y)
#define max_t(type, x, y) (x > y ? x: y)
#define msleep mdelay

#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))
#define __round_mask(x, y) ((unsigned long)((y)-1))
#define round_down(x, y) ((x) & ~(__round_mask((x), (y))))
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))


//#define min(x,y) ((x) < (y) ? x : y)
#define max(x,y) ((x) > (y) ? x : y)


#define min3(x, y, z) min(min(x, y), z)
#define max3(x, y, z) max(max(x, y), z)

#define ROUND(a,b)      (((a) + (b) - 1) & ~((b) - 1))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define ALIGN(x,a)      __ALIGN_MASK((x),(uintptr_t)(a)-1)
//#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))


//typedef unsigned long       uintptr_t;

#define PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)		\
	char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)  \
		      + (align - 1)];					\
									\
	type *name = (type *)ALIGN((uintptr_t)__##name, align)
#define ALLOC_ALIGN_BUFFER(type, name, size, align)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)			\
	ALLOC_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)

#define be32_to_cpu(x) ((uint32_t)(              \
    (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |        \
    (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |        \
    (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |        \
    (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define LOG2(x) (((x & 0xaaaaaaaa) ? 1 : 0) + ((x & 0xcccccccc) ? 2 : 0) + \
         ((x & 0xf0f0f0f0) ? 4 : 0) + ((x & 0xff00ff00) ? 8 : 0) + \
         ((x & 0xffff0000) ? 16 : 0))


#ifdef __cplusplus
}
#endif

#endif
