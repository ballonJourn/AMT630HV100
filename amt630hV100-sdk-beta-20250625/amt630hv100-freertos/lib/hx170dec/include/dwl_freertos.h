#include "basetype.h"
#include "dwl.h"

//#define _DWL_DEBUG

#ifdef _DWL_DEBUG
#define DWL_DEBUG(fmt, args...) printf(__FILE__ ":%d: " fmt, __LINE__ , ## args)
#else
#define DWL_DEBUG(fmt, args...) /* not debugging: nothing */
#endif

#ifndef HX170DEC_IO_BASE
#define HX170DEC_IO_BASE   0xC0000000U
#endif

#define HX170PP_REG_START       0xF0
#define HX170DEC_REG_START      0x4

#define HX170PP_SYNTH_CFG       100
#define HX170DEC_SYNTH_CFG       50
#define HX170DEC_SYNTH_CFG_2     54

#define HX170PP_FUSE_CFG         99
#define HX170DEC_FUSE_CFG        57

#define DWL_DECODER_INT ((DWLReadReg(dec_dwl, HX170DEC_REG_START) >> 12) & 0xFFU)
#define DWL_PP_INT      ((DWLReadReg(dec_dwl, HX170PP_REG_START) >> 12) & 0xFFU)

#define DEC_IRQ_ABORT          (1 << 11)
#define DEC_IRQ_RDY            (1 << 12)
#define DEC_IRQ_BUS            (1 << 13)
#define DEC_IRQ_BUFFER         (1 << 14)
#define DEC_IRQ_ASO            (1 << 15)
#define DEC_IRQ_ERROR          (1 << 16)
#define DEC_IRQ_SLICE          (1 << 17)
#define DEC_IRQ_TIMEOUT        (1 << 18)

#define PP_IRQ_RDY             (1 << 12)
#define PP_IRQ_BUS             (1 << 13)

#define DWL_STREAM_ERROR_BIT    0x010000    /* 16th bit */
#define DWL_HW_TIMEOUT_BIT      0x040000    /* 18th bit */
#define DWL_HW_ENABLE_BIT       0x000001    /* 0th bit */
#define DWL_HW_PIC_RDY_BIT      0x001000    /* 12th bit */

#ifdef _DWL_HW_PERFORMANCE
/* signal that decoder/pp is enabled */
void DwlDecoderEnable(void);
void DwlPpEnable(void);
#endif
/* Function prototypes */

/* wrapper information */
typedef struct hX170dwl
{
    u32 clientType;
	u32 numCores;
	volatile u32 *pRegBase;  /* IO mem base */
    u32 regSize;             /* IO mem size */
    u32 freeLinMem;          /* Start address of free linear memory */
    u32 freeRefFrmMem;       /* Start address of free reference frame memory */
    int semid;
    int sigio_needed;
	u32 bPPReserved;
} hX170dwl_t;

i32 DWLWaitPpHwReady(const void *instance, u32 timeout);
i32 DWLWaitDecHwReady(const void *instance, u32 timeout);
void DWLSoftResetAsic(void);
