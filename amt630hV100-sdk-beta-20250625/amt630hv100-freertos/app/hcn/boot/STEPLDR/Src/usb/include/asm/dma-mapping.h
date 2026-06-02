/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2007
 * Stelian Pop <stelian@popies.net>
 * Lead Tech Design <www.leadtechdesign.com>
 */
#ifndef __ASM_ARM_DMA_MAPPING_H
#define __ASM_ARM_DMA_MAPPING_H

#include <linux/dma-direction.h>

#define	dma_mapping_error(x, y)	0
struct device;

static inline void *dma_alloc_coherent(struct device *dev, size_t len, dma_addr_t *handle, gfp_t flag)
{
	(void)flag;
	//*handle = (unsigned long)memalign(ARCH_DMA_MINALIGN, len);
	*handle = (unsigned long)malloc(len + ARCH_DMA_MINALIGN);
	return (void *)*handle;
}

static inline void dma_free_coherent(struct device *dev, size_t size, void *addr, dma_addr_t handle)
{
	free(addr);
}

static inline void *dmam_alloc_coherent(struct device *dev, size_t len, dma_addr_t *handle, gfp_t flag)
{
	(void)flag;
	(void)dev;
	*handle = (dma_addr_t)malloc(len + ARCH_DMA_MINALIGN);
	return (void *)*handle;
}

static inline void dmam_free_coherent(struct device *dev, size_t len, void *vaddr, dma_addr_t handle)
{
	free(vaddr);
}



static inline unsigned long dma_map_single(volatile void *vaddr, size_t len,
					   enum dma_data_direction dir)
{
	return (unsigned long)vaddr;
}

static inline void dma_unmap_single(volatile void *vaddr, size_t len,
				    unsigned long paddr)
{
}

static inline void dma_sync_single_for_device(struct device *dev,
	dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_cpu(struct device *dev,
	dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
}


#endif /* __ASM_ARM_DMA_MAPPING_H */
