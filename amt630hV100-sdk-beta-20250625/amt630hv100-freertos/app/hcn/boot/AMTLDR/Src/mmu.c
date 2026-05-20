#include "mmu.h"

//#pragma data_alignment=16384
//__no_init unsigned int mmu_tlb_table[4096];

//#define _MMUTT_STARTADDRESS		((unsigned int)mmu_tlb_table)
#define _MMUTT_STARTADDRESS		((unsigned int)0x30c000)


/**
 * \brief Initializes MMU.
 * \param pTB  Address of the translation table.
 */
void MMU_Initialize(unsigned int *pTB)
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
                    ( 1 << 18)|   // 16MB Supersection
                    ( 3 << 10)| // Access in supervisor mode (AP)
                    ( 1 << 12)| // TEX[0]
                    ( 0 <<  5)| // Domain 0x0, Supersection only support domain 0
                    ( 0 <<  4)| // (XN)
                    ( 1 <<  3)| // C bit : cachable => YES
                    ( 1 <<  2)| // B bit : write-back => YES
                    ( 2 <<  0); // Set as 1 Mbyte section

	/* DDRAM address (after remap) 0x2000_0000 */
	for(addr = 0x200; addr < 0x220; addr++)
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
	for(addr = 0x300; addr < 0x320; addr++)
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

void MMU_Init(void)
{
	if(CP15_IsIcacheEnabled())
		CP15_DisableIcache();
	if(CP15_IsDcacheEnabled())
		CP15_DisableDcache();

	if(CP15_IsMMUEnabled())
		CP15_DisableMMU();
		
	MMU_Initialize((unsigned int*)_MMUTT_STARTADDRESS);
	
	CP15_EnableMMU(); 
	CP15_EnableIcache();
	CP15_EnableDcache(); 
}
