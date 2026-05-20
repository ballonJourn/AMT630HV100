#ifndef _MMU_
#define _MMU_
#include "cp15.h"
/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/
extern void MMU_Init(void);
extern void dma_flush_range(unsigned int ulStart, unsigned int ulEnd);
extern void dma_inv_range (unsigned int ulStart, unsigned int ulEnd);
extern void dma_clean_range(unsigned int ulStart, unsigned int ulEnd);
extern unsigned int vaddr_to_page_addr (unsigned int addr);
extern unsigned int page_addr_to_vaddr (unsigned int addr);



#endif /* #ifndef _MMU_ */

