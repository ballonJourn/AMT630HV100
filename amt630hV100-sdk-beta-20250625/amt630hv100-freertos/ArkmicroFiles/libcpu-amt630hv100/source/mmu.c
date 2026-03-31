/* ----------------------------------------------------------------------------
 *         SAM Software Package License 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2013, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */
 
/** \file */

/** 
 * \addtogroup mmu MMU Initialization
 *
 * \section Usage
 *
 * Translation Lookaside Buffers (TLBs) are an implementation technique that caches translations or
 * translation table entries. TLBs avoid the requirement for every memory access to perform a translation table
 * lookup. The ARM architecture does not specify the exact form of the TLB structures for any design. In a
 * similar way to the requirements for caches, the architecture only defines certain principles for TLBs:
 * 
 * The MMU supports memory accesses based on memory sections or pages:
 * Supersections Consist of 16MB blocks of memory. Support for Supersections is optional.
 * -# Sections Consist of 1MB blocks of memory.
 * -# Large pages Consist of 64KB blocks of memory.
 * -# Small pages Consist of 4KB blocks of memory.
 *
 * Access to a memory region is controlled by the access permission bits and the domain field in the TLB entry.
 * Memory region attributes
 * Each TLB entry has an associated set of memory region attributes. These control accesses to the caches,
 * how the write buffer is used, and if the memory region is Shareable and therefore must be kept coherent.
 *
 * Related files:\n
 * \ref mmu.c\n
 * \ref mmu.h \n
 */
 
/*------------------------------------------------------------------------------ */
/*         Headers                                                               */
/*------------------------------------------------------------------------------ */
#include <chip.h>

/*------------------------------------------------------------------------------ */
/*         Exported functions */
/*------------------------------------------------------------------------------ */

/**
 * \brief Initializes MMU.
 * \param pTB  Address of the translation table.
 */
void MMU_Initialize(uint32_t *pTB)
{
    unsigned int index;
    unsigned int addr;

    /* Reset table entries */
    for (index = 0; index < 4096; index++)
        pTB[index] = 0;

	/* interrupt vector address (after remap) 0x0000_0000 */
    pTB[0x000] = (0x200 << 20)| // Physical Address
                 //   ( 1 << 12)| // TEX[0]
                    ( 3 << 10)| // Access in supervisor mode (AP)
                   ( 0xF << 5)| // Domain 0xF
                    ( 1 <<  4)| // (XN)
                    ( 0 <<  3)| // C bit : cachable => YES
					( 0 <<  2)| // B bit : write-back => YES  
                    ( 2 <<  0); // Set as 1 Mbyte section

    /* SRAM address (after remap) 0x0030_0000 */
    pTB[0x003] = (0x003 << 20)| // Physical Address
                   // ( 1 << 12)| // TEX[0]
                    ( 3 << 10)| // Access in supervisor mode (AP)
                  ( 0xF <<  5)| // Domain 0xF
                    ( 1 <<  4)| // (XN)
                    ( 0 <<  3)| // C bit : cachable => YES
                    ( 0 <<  2)| // B bit : write-back => YES
                    ( 2 <<  0); // Set as 1 Mbyte section

	/* DDRAM address (after remap) 0x2000_0000 */
	for(addr = 0x200; addr < 0x240; addr++)
		pTB[addr] = (addr << 20)| // Physical Address
		          ( 1 << 18)|   // 16MB Supersection
		          ( 3 << 10)|   // Access in supervisor mode (AP)
		          ( 1 << 12)|   // TEX[0]
		          ( 0 <<  5)|   // Domain 0x0, Supersection only support domain 0
		          ( 0 <<  4)|   // (XN)
		          ( 1 <<  3)|   // C bit : cachable => YES
		          ( 1 <<  2)|   // B bit : write-back => YES
		          ( 2 <<  0);   // Set as 1 Mbyte section

	/* DDRAM non-cache address (after remap) 0x3000_0000 */
	for(addr = 0x300; addr < 0x340; addr++)
		pTB[addr] = ((addr - 0x100) << 20)| // Physical Address
		          ( 1 << 18)|   // 16MB Supersection
		          ( 3 << 10)|   // Access in supervisor mode (AP)
		          ( 1 << 12)|   // TEX[0]
		          ( 0 <<  5)|   // Domain 0x0, Supersection only support domain 0
		          ( 0 <<  4)|   // (XN)
		          ( 0 <<  3)|   // C bit : cachable => YES
		          ( 0 <<  2)|   // B bit : write-back => YES
		          ( 2 <<  0);   // Set as 1 Mbyte section				  

	// periph address 0x60000000 ~ 0x80000000
	for(addr = 0x600; addr < 0x800; addr++)
		pTB[addr] = (addr << 20)| // Physical Address
		          ( 3 << 10)| // Access in supervisor mode (AP)
		        ( 0xF <<  5)| // Domain 0xF
		          ( 1 <<  4)| // (XN)
		          ( 0 <<  3)| // C bit : cachable => NO
		          ( 0 <<  2)| // B bit : write-back => NO
		          ( 2 <<  0); // Set as 1 Mbyte section

    CP15_WriteTTB((unsigned int)pTB);
    /* Program the domain access register */
    CP15_WriteDomainAccessControl(0xC0000003); // only domain 0 & 15: access are not checked
}


void dma_inv_range(UINT32 ulStart, UINT32 ulEnd)
{
	CP15_invalidate_dcache_for_dma (ulStart, ulEnd);
}

void dma_clean_range(UINT32 ulStart, UINT32 ulEnd)
{
	CP15_clean_dcache_for_dma (ulStart, ulEnd);
}

// flush��clean and invalidate
void dma_flush_range(UINT32 ulStart, UINT32 ulEnd)
{
	CP15_flush_dcache_for_dma (ulStart, ulEnd);
}
