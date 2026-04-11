#include "FreeRTOS.h"
#include "chip.h"

#include "basetype.h"
#include "dwl_defs.h"
#include "dwl_freertos.h"
#include "dwl.h"
#include "hx170dec.h"

#define DWL_USE_DEC_IRQ


#define DWL_PJPEG_E         22  /* 1 bit */
#define DWL_REF_BUFF_E      20  /* 1 bit */

#define DWL_JPEG_EXT_E          31  /* 1 bit */
#define DWL_REF_BUFF_ILACE_E    30  /* 1 bit */
#define DWL_MPEG4_CUSTOM_E      29  /* 1 bit */
#define DWL_REF_BUFF_DOUBLE_E   28  /* 1 bit */

#define DWL_MVC_E           20  /* 2 bits */

#define DWL_DEC_TILED_L     17  /* 2 bits */
#define DWL_DEC_PIC_W_EXT   14  /* 2 bits */
#define DWL_EC_E            12  /* 2 bits */
#define DWL_STRIDE_E        11  /* 1 bit */
#define DWL_FIELD_DPB_E     10  /* 1 bit */

#define DWL_CFG_E           24  /* 4 bits */
#define DWL_PP_IN_TILED_L   14  /* 2 bits */

#define DWL_SORENSONSPARK_E 11  /* 1 bit */

#define DWL_H264_FUSE_E          31 /* 1 bit */
#define DWL_MPEG4_FUSE_E         30 /* 1 bit */
#define DWL_MPEG2_FUSE_E         29 /* 1 bit */
#define DWL_SORENSONSPARK_FUSE_E 28 /* 1 bit */
#define DWL_JPEG_FUSE_E          27 /* 1 bit */
#define DWL_VP6_FUSE_E           26 /* 1 bit */
#define DWL_VC1_FUSE_E           25 /* 1 bit */
#define DWL_PJPEG_FUSE_E         24 /* 1 bit */
#define DWL_CUSTOM_MPEG4_FUSE_E  23 /* 1 bit */
#define DWL_RV_FUSE_E            22 /* 1 bit */
#define DWL_VP7_FUSE_E           21 /* 1 bit */
#define DWL_VP8_FUSE_E           20 /* 1 bit */
#define DWL_AVS_FUSE_E           19 /* 1 bit */
#define DWL_MVC_FUSE_E           18 /* 1 bit */

#define DWL_DEC_MAX_1920_FUSE_E  15 /* 1 bit */
#define DWL_DEC_MAX_1280_FUSE_E  14 /* 1 bit */
#define DWL_DEC_MAX_720_FUSE_E   13 /* 1 bit */
#define DWL_DEC_MAX_352_FUSE_E   12 /* 1 bit */
#define DWL_REF_BUFF_FUSE_E       7 /* 1 bit */


#define DWL_PP_FUSE_E				31  /* 1 bit */
#define DWL_PP_DEINTERLACE_FUSE_E   30  /* 1 bit */
#define DWL_PP_ALPHA_BLEND_FUSE_E   29  /* 1 bit */
#define DWL_PP_MAX_4096_FUSE_E		16  /* 1 bit */
#define DWL_PP_MAX_1920_FUSE_E		15  /* 1 bit */
#define DWL_PP_MAX_1280_FUSE_E		14  /* 1 bit */
#define DWL_PP_MAX_720_FUSE_E		13  /* 1 bit */
#define DWL_PP_MAX_352_FUSE_E		12  /* 1 bit */

#define IS_PIPELINE_ENABLED(val)    ((val) & 0x02)

/* shadow HW registers */
u32 dwlShadowRegs[MAX_ASIC_CORES][128];

#ifdef _DWL_DEBUG
static void PrintIrqType(u32 isPP, /*u32 coreID,*/ u32 status)
{
	u32 coreID = 0;

    if(isPP)
    {
        printf("PP[%d] IRQ %s\n", coreID,
                status & PP_IRQ_RDY ? "READY" : "BUS ERROR");
    }
    else
    {
        if(status & DEC_IRQ_ABORT)
            printf("DEC[%d] IRQ ABORT\n", coreID);
        else if (status & DEC_IRQ_RDY)
            printf("DEC[%d] IRQ READY\n", coreID);
        else if (status & DEC_IRQ_BUS)
            printf("DEC[%d] IRQ BUS ERROR\n", coreID);
        else if (status & DEC_IRQ_BUFFER)
            printf("DEC[%d] IRQ BUFFER\n", coreID);
        else if (status & DEC_IRQ_ASO)
            printf("DEC[%d] IRQ ASO\n", coreID);
        else if (status & DEC_IRQ_ERROR)
            printf("DEC[%d] IRQ STREAM ERROR\n", coreID);
        else if (status & DEC_IRQ_SLICE)
            printf("DEC[%d] IRQ SLICE\n", coreID);
        else if (status & DEC_IRQ_TIMEOUT)
            printf("DEC[%d] IRQ TIMEOUT\n", coreID);
        else
            printf("DEC[%d] IRQ UNKNOWN 0x%08x\n", coreID, status);
    }
}
#endif
/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicCoreCount
    Description     : Return the number of hardware cores available
------------------------------------------------------------------------------*/
u32 DWLReadAsicCoreCount(void)
{
    unsigned int cores = 0;

    /* ask module for cores */
    if (vdec_ioctl(HX170DEC_IOC_MC_CORES, &cores) == -1)
    {
        DWL_DEBUG("ioctl failed\n");
        cores = 0;
    }

    return (u32)cores;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicID
    Description     : Read the HW ID. Does not need a DWL instance to run

    Return type     : u32 - the HW ID
------------------------------------------------------------------------------*/
u32 DWLReadAsicID()
{
    u32 id = ~0;
    unsigned long base;

    DWL_DEBUG("\n");

    /* ask module for base */
    if(vdec_ioctl(HX170DEC_IOCGHWOFFSET, &base) == -1)
    {
        DWL_DEBUG("ioctl failed\n");
        return id;
    }
    id = *(volatile unsigned int*)base;

    return id;
}

static void ReadCoreFuse(const u32 *io, DWLHwFuseStatus_t *pHwFuseSts)
{
    u32 configReg, fuseReg, fuseRegPp;

    /* Decoder configuration */
    configReg = io[HX170DEC_SYNTH_CFG];

    /* Decoder fuse configuration */
    fuseReg = io[HX170DEC_FUSE_CFG];

    pHwFuseSts->h264SupportFuse = (fuseReg >> DWL_H264_FUSE_E) & 0x01U;
    pHwFuseSts->mpeg4SupportFuse = (fuseReg >> DWL_MPEG4_FUSE_E) & 0x01U;
    pHwFuseSts->mpeg2SupportFuse = (fuseReg >> DWL_MPEG2_FUSE_E) & 0x01U;
    pHwFuseSts->sorensonSparkSupportFuse =
            (fuseReg >> DWL_SORENSONSPARK_FUSE_E) & 0x01U;
    pHwFuseSts->jpegSupportFuse = (fuseReg >> DWL_JPEG_FUSE_E) & 0x01U;
    pHwFuseSts->vp6SupportFuse = (fuseReg >> DWL_VP6_FUSE_E) & 0x01U;
    pHwFuseSts->vc1SupportFuse = (fuseReg >> DWL_VC1_FUSE_E) & 0x01U;
    pHwFuseSts->jpegProgSupportFuse = (fuseReg >> DWL_PJPEG_FUSE_E) & 0x01U;
    pHwFuseSts->rvSupportFuse = (fuseReg >> DWL_RV_FUSE_E) & 0x01U;
    pHwFuseSts->avsSupportFuse = (fuseReg >> DWL_AVS_FUSE_E) & 0x01U;
    pHwFuseSts->vp7SupportFuse = (fuseReg >> DWL_VP7_FUSE_E) & 0x01U;
    pHwFuseSts->vp8SupportFuse = (fuseReg >> DWL_VP8_FUSE_E) & 0x01U;
    pHwFuseSts->customMpeg4SupportFuse = (fuseReg >> DWL_CUSTOM_MPEG4_FUSE_E) & 0x01U;
    pHwFuseSts->mvcSupportFuse = (fuseReg >> DWL_MVC_FUSE_E) & 0x01U;

    /* check max. decoder output width */
    if(fuseReg & 0x10000U)
        pHwFuseSts->maxDecPicWidthFuse = 4096;
    else if(fuseReg & 0x8000U)
        pHwFuseSts->maxDecPicWidthFuse = 1920;
    else if(fuseReg & 0x4000U)
        pHwFuseSts->maxDecPicWidthFuse = 1280;
    else if(fuseReg & 0x2000U)
        pHwFuseSts->maxDecPicWidthFuse = 720;
    else if(fuseReg & 0x1000U)
        pHwFuseSts->maxDecPicWidthFuse = 352;

    pHwFuseSts->refBufSupportFuse = (fuseReg >> DWL_REF_BUFF_FUSE_E) & 0x01U;

    /* Pp configuration */
    configReg = io[HX170PP_SYNTH_CFG];

    if((configReg >> DWL_PP_E) & 0x01U)
    {
        /* Pp fuse configuration */
        fuseRegPp = io[HX170PP_FUSE_CFG];

        if((fuseRegPp >> DWL_PP_FUSE_E) & 0x01U)
        {
            pHwFuseSts->ppSupportFuse = 1;

            /* check max. pp output width */
            if(fuseRegPp & 0x10000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 4096;
            else if(fuseRegPp & 0x8000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 1920;
            else if(fuseRegPp & 0x4000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 1280;
            else if(fuseRegPp & 0x2000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 720;
            else if(fuseRegPp & 0x1000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 352;

            pHwFuseSts->ppConfigFuse = fuseRegPp;
        }
        else
        {
            pHwFuseSts->ppSupportFuse = 0;
            pHwFuseSts->maxPpOutPicWidthFuse = 0;
            pHwFuseSts->ppConfigFuse = 0;
        }
    }
}

static void ReadCoreConfig(const u32 *io, DWLHwConfig_t *pHwCfg)
{
    u32 configReg;
    const u32 asicID = io[0];

    /* Decoder configuration */
    configReg = io[HX170DEC_SYNTH_CFG];

    pHwCfg->h264Support = (configReg >> DWL_H264_E) & 0x3U;
    /* check jpeg */
    pHwCfg->jpegSupport = (configReg >> DWL_JPEG_E) & 0x01U;
    if(pHwCfg->jpegSupport && ((configReg >> DWL_PJPEG_E) & 0x01U))
        pHwCfg->jpegSupport = JPEG_PROGRESSIVE;
    pHwCfg->mpeg4Support = (configReg >> DWL_MPEG4_E) & 0x3U;
    pHwCfg->vc1Support = (configReg >> DWL_VC1_E) & 0x3U;
    pHwCfg->mpeg2Support = (configReg >> DWL_MPEG2_E) & 0x01U;
    pHwCfg->sorensonSparkSupport = (configReg >> DWL_SORENSONSPARK_E) & 0x01U;
#ifndef DWL_REFBUFFER_DISABLE
    pHwCfg->refBufSupport = (configReg >> DWL_REF_BUFF_E) & 0x01U;
#else
    pHwCfg->refBufSupport = 0;
#endif
    pHwCfg->vp6Support = (configReg >> DWL_VP6_E) & 0x01U;
#ifdef DEC_X170_APF_DISABLE
    if(DEC_X170_APF_DISABLE)
    {
        pHwCfg->tiledModeSupport = 0;
    }
#endif /* DEC_X170_APF_DISABLE */

    pHwCfg->maxDecPicWidth = configReg & 0x07FFU;

    /* 2nd Config register */
    configReg = io[HX170DEC_SYNTH_CFG_2];
    if(pHwCfg->refBufSupport)
    {
        if((configReg >> DWL_REF_BUFF_ILACE_E) & 0x01U)
            pHwCfg->refBufSupport |= 2;
        if((configReg >> DWL_REF_BUFF_DOUBLE_E) & 0x01U)
            pHwCfg->refBufSupport |= 4;
    }

    pHwCfg->customMpeg4Support = (configReg >> DWL_MPEG4_CUSTOM_E) & 0x01U;
    pHwCfg->vp7Support = (configReg >> DWL_VP7_E) & 0x01U;
    pHwCfg->vp8Support = (configReg >> DWL_VP8_E) & 0x01U;
    pHwCfg->avsSupport = (configReg >> DWL_AVS_E) & 0x01U;

    /* JPEG extensions */
    if(((asicID >> 16) >= 0x8190U) || ((asicID >> 16) == 0x6731U))
        pHwCfg->jpegESupport = (configReg >> DWL_JPEG_EXT_E) & 0x01U;
    else
        pHwCfg->jpegESupport = JPEG_EXT_NOT_SUPPORTED;

    if(((asicID >> 16) >= 0x9170U) || ((asicID >> 16) == 0x6731U))
        pHwCfg->rvSupport = (configReg >> DWL_RV_E) & 0x03U;
    else
        pHwCfg->rvSupport = RV_NOT_SUPPORTED;

    pHwCfg->mvcSupport = (configReg >> DWL_MVC_E) & 0x03U;
    pHwCfg->webpSupport = (configReg >> DWL_WEBP_E) & 0x01U;
    pHwCfg->tiledModeSupport = (configReg >> DWL_DEC_TILED_L) & 0x03U;
    pHwCfg->maxDecPicWidth += (( configReg >> DWL_DEC_PIC_W_EXT) & 0x03U ) << 11;

    pHwCfg->ecSupport = (configReg >> DWL_EC_E) & 0x03U;
    pHwCfg->strideSupport = (configReg >> DWL_STRIDE_E) & 0x01U;
    pHwCfg->fieldDpbSupport = (configReg >> DWL_FIELD_DPB_E) & 0x01U;

    if(pHwCfg->refBufSupport && ((asicID >> 16) == 0x6731U))
    {
        pHwCfg->refBufSupport |= 8; /* enable HW support for offset */
    }

    /* Pp configuration */
    configReg = io[HX170PP_SYNTH_CFG];

    if((configReg >> DWL_PP_E) & 0x01U)
    {
        pHwCfg->ppSupport = 1;
        /* Theoretical max range 0...8191; actual 48...4096 */
        pHwCfg->maxPpOutPicWidth = configReg & 0x1FFFU;
        /*pHwCfg->ppConfig = (configReg >> DWL_CFG_E) & 0x0FU; */
        pHwCfg->ppConfig = configReg;
    }
    else
    {
        pHwCfg->ppSupport = 0;
        pHwCfg->maxPpOutPicWidth = 0;
        pHwCfg->ppConfig = 0;
    }

    /* check the HW version */
    if(((asicID >> 16) >= 0x8190U) || ((asicID >> 16) == 0x6731U))
    {
        u32 deInterlace;
        u32 alphaBlend;
        u32 deInterlaceFuse;
        u32 alphaBlendFuse;
        DWLHwFuseStatus_t hwFuseSts;

        /* check fuse status */
        ReadCoreFuse(io, &hwFuseSts);

        /* Maximum decoding width supported by the HW */
        if(pHwCfg->maxDecPicWidth > hwFuseSts.maxDecPicWidthFuse)
            pHwCfg->maxDecPicWidth = hwFuseSts.maxDecPicWidthFuse;
        /* Maximum output width of Post-Processor */
        if(pHwCfg->maxPpOutPicWidth > hwFuseSts.maxPpOutPicWidthFuse)
            pHwCfg->maxPpOutPicWidth = hwFuseSts.maxPpOutPicWidthFuse;
        /* h264 */
        if(!hwFuseSts.h264SupportFuse)
            pHwCfg->h264Support = H264_NOT_SUPPORTED;
        /* mpeg-4 */
        if(!hwFuseSts.mpeg4SupportFuse)
            pHwCfg->mpeg4Support = MPEG4_NOT_SUPPORTED;
        /* custom mpeg-4 */
        if(!hwFuseSts.customMpeg4SupportFuse)
            pHwCfg->customMpeg4Support = MPEG4_CUSTOM_NOT_SUPPORTED;
        /* jpeg (baseline && progressive) */
        if(!hwFuseSts.jpegSupportFuse)
            pHwCfg->jpegSupport = JPEG_NOT_SUPPORTED;
        if((pHwCfg->jpegSupport == JPEG_PROGRESSIVE) &&
                !hwFuseSts.jpegProgSupportFuse)
            pHwCfg->jpegSupport = JPEG_BASELINE;
        /* mpeg-2 */
        if(!hwFuseSts.mpeg2SupportFuse)
            pHwCfg->mpeg2Support = MPEG2_NOT_SUPPORTED;
        /* vc-1 */
        if(!hwFuseSts.vc1SupportFuse)
            pHwCfg->vc1Support = VC1_NOT_SUPPORTED;
        /* vp6 */
        if(!hwFuseSts.vp6SupportFuse)
            pHwCfg->vp6Support = VP6_NOT_SUPPORTED;
        /* vp7 */
        if(!hwFuseSts.vp7SupportFuse)
            pHwCfg->vp7Support = VP7_NOT_SUPPORTED;
        /* vp8 */
        if(!hwFuseSts.vp8SupportFuse)
            pHwCfg->vp8Support = VP8_NOT_SUPPORTED;
        /* webp */
        if(!hwFuseSts.vp8SupportFuse)
            pHwCfg->webpSupport = WEBP_NOT_SUPPORTED;
        /* pp */
        if(!hwFuseSts.ppSupportFuse)
            pHwCfg->ppSupport = PP_NOT_SUPPORTED;
        /* check the pp config vs fuse status */
        if((pHwCfg->ppConfig & 0xFC000000) &&
                ((hwFuseSts.ppConfigFuse & 0xF0000000) >> 5))
        {
            /* config */
            deInterlace = ((pHwCfg->ppConfig & PP_DEINTERLACING) >> 25);
            alphaBlend = ((pHwCfg->ppConfig & PP_ALPHA_BLENDING) >> 24);
            /* fuse */
            deInterlaceFuse =
                    (((hwFuseSts.ppConfigFuse >> 5) & PP_DEINTERLACING) >> 25);
            alphaBlendFuse =
                    (((hwFuseSts.ppConfigFuse >> 5) & PP_ALPHA_BLENDING) >> 24);

            /* check if */
            if(deInterlace && !deInterlaceFuse)
                pHwCfg->ppConfig &= 0xFD000000;
            if(alphaBlend && !alphaBlendFuse)
                pHwCfg->ppConfig &= 0xFE000000;
        }
        /* sorenson */
        if(!hwFuseSts.sorensonSparkSupportFuse)
            pHwCfg->sorensonSparkSupport = SORENSON_SPARK_NOT_SUPPORTED;
        /* ref. picture buffer */
        if(!hwFuseSts.refBufSupportFuse)
            pHwCfg->refBufSupport = REF_BUF_NOT_SUPPORTED;

        /* rv */
        if(!hwFuseSts.rvSupportFuse)
            pHwCfg->rvSupport = RV_NOT_SUPPORTED;
        /* avs */
        if(!hwFuseSts.avsSupportFuse)
            pHwCfg->avsSupport = AVS_NOT_SUPPORTED;
        /* mvc */
        if(!hwFuseSts.mvcSupportFuse)
            pHwCfg->mvcSupport = MVC_NOT_SUPPORTED;
    }
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicConfig
    Description     : Read HW configuration. Does not need a DWL instance to run

    Return type     : DWLHwConfig_t - structure with HW configuration
------------------------------------------------------------------------------*/
void DWLReadAsicConfig(DWLHwConfig_t *pHwCfg)
{
    unsigned long base;

    DWL_DEBUG("\n");

    /* ask module for base */
    if(vdec_ioctl(HX170DEC_IOCGHWOFFSET, &base) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOCGHWOFFSET failed\n");
        return;
    }

    /* Decoder configuration */
    memset(pHwCfg, 0, sizeof(*pHwCfg));

    ReadCoreConfig((const u32 *)base, pHwCfg);
}

void DWLReadMCAsicConfig(DWLHwConfig_t pHwCfg[MAX_ASIC_CORES])
{
    unsigned int nCores, i;
    unsigned long mcRegBase[MAX_ASIC_CORES];

    DWL_DEBUG("\n");

    if(vdec_ioctl(HX170DEC_IOC_MC_CORES,  &nCores) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOC_MC_CORES failed\n");
        return;
    }

    configASSERT(nCores <= MAX_ASIC_CORES);

    if(vdec_ioctl(HX170DEC_IOC_MC_OFFSETS, mcRegBase) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOC_MC_OFFSETS failed\n");
        return;
    }

    /* Decoder configuration */
    memset(pHwCfg, 0, MAX_ASIC_CORES * sizeof(*pHwCfg));

    for (i = 0; i < nCores; i++)
    {
        ReadCoreConfig((const u32 *)mcRegBase[i], pHwCfg + i);
    }
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicFuseStatus
    Description     : Read HW fuse configuration. Does not need a DWL instance to run

    Returns     : DWLHwFuseStatus_t * pHwFuseSts - structure with HW fuse configuration
------------------------------------------------------------------------------*/
void DWLReadAsicFuseStatus(DWLHwFuseStatus_t * pHwFuseSts)
{
    unsigned long base;
    //unsigned int regSize;

    DWL_DEBUG("\n");

    memset(pHwFuseSts, 0, sizeof(*pHwFuseSts));

    /* ask module for base */
    if(vdec_ioctl(HX170DEC_IOCGHWOFFSET, &base) == -1)
    {
        DWL_DEBUG("ioctl failed\n");
        return;
    }

    /* Decoder fuse configuration */
    ReadCoreFuse((const u32*)base, pHwFuseSts);
}

/*------------------------------------------------------------------------------
    Function name   : DWLMallocRefFrm
    Description     : Allocate a frame buffer (contiguous linear RAM memory)

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - DWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : void *info - place where the allocated memory buffer
                        parameters are returned
------------------------------------------------------------------------------*/
i32 DWLMallocRefFrm(const void *instance, u32 size, DWLLinearMem_t * info)
{

#ifdef MEMORY_USAGE_TRACE
    printf("DWLMallocRefFrm\t%8d bytes\n", size);
#endif

    return DWLMallocLinear(instance, size, info);

}

/*------------------------------------------------------------------------------
    Function name   : DWLFreeRefFrm
    Description     : Release a frame buffer previously allocated with
                        DWLMallocRefFrm.

    Return type     : void

    Argument        : const void * instance - DWL instance
    Argument        : void *info - frame buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeRefFrm(const void *instance, DWLLinearMem_t * info)
{
    DWLFreeLinear(instance, info);
}

/*------------------------------------------------------------------------------
    Function name   : DWLMallocLinear
    Description     : Allocate a contiguous, linear RAM  memory buffer

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - DWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : void *info - place where the allocated memory buffer
                        parameters are returned
------------------------------------------------------------------------------*/
i32 DWLMallocLinear(const void *instance, u32 size, DWLLinearMem_t * info)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    configASSERT(dec_dwl != NULL);
    configASSERT(info != NULL);

#ifdef MEMORY_USAGE_TRACE
    printf("DWLMallocLinear\t%8d bytes \n", size);
#endif

    info->size = size;
    info->virtualAddress = (u32 *)pvPortMalloc(size);
	info->busAddress = (u32)info->virtualAddress;
	info->virtualAddress = (u32 *)PHY_TO_UNCACHED_VIRT(info->virtualAddress);//info->virtualAddress + PHY_TO_VIR/4;

	CP15_invalidate_dcache_for_dma(info->busAddress, info->busAddress + size);

#ifdef MEMORY_USAGE_TRACE
    printf("DWLMallocLinear 0x%08x virtualAddress: 0x%08x\n",
           info->busAddress, (unsigned) info->virtualAddress);
#endif

    if(info->virtualAddress == NULL)
        return DWL_ERROR;

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLFreeLinear
    Description     : Release a linera memory buffer, previously allocated with
                        DWLMallocLinear.

    Return type     : void

    Argument        : const void * instance - DWL instance
    Argument        : void *info - linear buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeLinear(const void *instance, DWLLinearMem_t * info)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    configASSERT(dec_dwl != NULL);
    configASSERT(info != NULL);

    if(info->virtualAddress != NULL)
       vPortFree((u32*)UNCACHED_VIRT_TO_PHT(info->virtualAddress));
     
	
}

/*------------------------------------------------------------------------------
    Function name   : DWLWriteReg
    Description     : Write a value to a hardware IO register

    Return type     : void

    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be written
    Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/

void DWLWriteReg(const void *instance, /*i32 coreID,*/ u32 offset, u32 value)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	i32 coreID = 0;

#ifndef DWL_DISABLE_REG_PRINTS
    DWL_DEBUG("core[%d] swreg[%d] at offset 0x%02X = %08X\n",
              coreID, offset/4, offset, value);
#endif

    configASSERT((dec_dwl->clientType != DWL_CLIENT_TYPE_PP &&
            offset < HX170PP_REG_START) ||
            (dec_dwl->clientType == DWL_CLIENT_TYPE_PP &&
                    offset >= HX170PP_REG_START));

    configASSERT(dec_dwl != NULL);
    configASSERT(offset < dec_dwl->regSize);
    configASSERT(coreID < (i32)dec_dwl->numCores);

    offset = offset / 4;

    dwlShadowRegs[coreID][offset] = value;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadReg
    Description     : Read the value of a hardware IO register

    Return type     : u32 - the value stored in the register

    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be read
------------------------------------------------------------------------------*/
u32 DWLReadReg(const void *instance, /*i32 coreID,*/ u32 offset)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    u32 val;
	i32 coreID = 0;

    configASSERT(dec_dwl != NULL);

    configASSERT((dec_dwl->clientType != DWL_CLIENT_TYPE_PP &&
            offset < HX170PP_REG_START) ||
            (dec_dwl->clientType == DWL_CLIENT_TYPE_PP &&
                    offset >= HX170PP_REG_START) || (offset == 0) ||
                    (offset == HX170PP_SYNTH_CFG));

    configASSERT(offset < dec_dwl->regSize);
    configASSERT(coreID < (i32)dec_dwl->numCores);

    offset = offset / 4;

    val = dwlShadowRegs[coreID][offset];

#ifndef DWL_DISABLE_REG_PRINTS
    DWL_DEBUG("core[%d] swreg[%d] at offset 0x%02X = %08X\n",
              coreID, offset, offset * 4, val);
#endif

    return val;
}

/*------------------------------------------------------------------------------
    Function name   : DWLEnableHW
    Description     : Enable hw by writing to register
    Return type     : void
    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be written
    Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLEnableHW(const void *instance, /*i32 coreID,*/ u32 offset, u32 value)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    struct core_desc core;
    int isPP, ioctl_req;
	i32 coreID = 0;

    configASSERT(dec_dwl);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;
    ioctl_req = isPP ? HX170DEC_IOCS_PP_PUSH_REG : HX170DEC_IOCS_DEC_PUSH_REG;
	//printf("isPP:%d\n", isPP);
    DWLWriteReg(dec_dwl, /*coreID,*/ offset, value);

    DWL_DEBUG("%s %d enabled by previous DWLWriteReg\n",
              isPP ? "PP" : "DEC", coreID);

    core.id = coreID;
    core.regs = dwlShadowRegs[coreID];
    core.regs += isPP ? 60 : 0;
    core.size = isPP ? 41 * 4 : 60 * 4;
#if 0	//add for test.
	int i;
	for(i=core.size/4-1; i>=0; i--) {
		printf("*(volatile unsigned int*)(0x71200000 + %d) = 0x%x;\n", 
			i * 4, dwlShadowRegs[coreID][i]);
	}
#endif
    if(vdec_ioctl(ioctl_req, &core))
    {
        DWL_DEBUG("ioctl HX170DEC_IOCS_*_PUSH_REG failed\n");
        configASSERT(0);
    }
}

/*------------------------------------------------------------------------------
    Function name   : DWLDisableHW
    Description     : Disable hw by writing to register
    Return type     : void
    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be written
    Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLDisableHW(const void *instance, /*i32 coreID,*/ u32 offset, u32 value)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    struct core_desc core;
    int isPP, ioctl_req;
	i32 coreID = 0;

    configASSERT(dec_dwl);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;
    ioctl_req = isPP ? HX170DEC_IOCS_PP_PUSH_REG : HX170DEC_IOCS_DEC_PUSH_REG;

    DWLWriteReg(dec_dwl, /*coreID,*/ offset, value);

    DWL_DEBUG("%s %d disabled by previous DWLWriteReg\n",
              isPP ? "PP" : "DEC", coreID);

    core.id = coreID;
    core.regs = dwlShadowRegs[coreID];
    core.regs += isPP ? 60 : 0;
    core.size = isPP ? 41 * 4 : 60 * 4;

    if (vdec_ioctl(ioctl_req, &core))
    {
        DWL_DEBUG("ioctl HX170DEC_IOCS_*_PUSH_REG failed\n");
        configASSERT(0);
    }
}

/*------------------------------------------------------------------------------
    Function name   : DWLWaitHwReady
    Description     : Wait until hardware has stopped running.
                      Used for synchronizing software runs with the hardware.
                      The wait could succed, timeout, or fail with an error.

    Return type     : i32 - one of the values DWL_HW_WAIT_OK
                                              DWL_HW_WAIT_TIMEOUT
                                              DWL_HW_WAIT_ERROR

    Argument        : const void * instance - DWL instance
------------------------------------------------------------------------------*/
i32 DWLWaitHwReady(const void *instance, /*i32 coreID,*/ u32 timeout)
{
    const hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    struct core_desc core;
    int isPP, ioctl_req;
    i32 ret = DWL_HW_WAIT_OK;
	i32 coreID = 0;

    UNUSED(timeout);

#ifndef DWL_USE_DEC_IRQ
    int max_wait_time = 10000; /* 10s in ms */
#endif

    configASSERT(dec_dwl);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    DWL_DEBUG("%s %d\n", isPP ? "PP" : "DEC", coreID);

    core.id = coreID;
    core.regs = dwlShadowRegs[coreID];
    core.regs += isPP ? 60 : 0;
    core.size = isPP ? 41 * 4 : 60 * 4;

#ifdef DWL_USE_DEC_IRQ
    if(isPP)
    {
        ioctl_req = HX170DEC_IOCX_PP_WAIT;

        if (vdec_ioctl(ioctl_req, &core))
        {
            DWL_DEBUG("ioctl HX170DEC_IOCG_*_WAIT failed\n");
            ret = DWL_HW_WAIT_ERROR;
        }
    }
    else
    {
        ioctl_req = HX170DEC_IOCX_DEC_WAIT;

        if (vdec_ioctl(ioctl_req, &core))
        {
            DWL_DEBUG("ioctl HX170DEC_IOCG_*_WAIT failed\n");
            ret = DWL_HW_WAIT_ERROR;
        }
    }
#else /* Polling */

    ret = DWL_HW_WAIT_TIMEOUT;

    ioctl_req = isPP ? HX170DEC_IOCS_PP_PULL_REG : HX170DEC_IOCS_DEC_PULL_REG;

    do
    {
        u32 irq_stats;
        const unsigned int usec = 1000; /* 1 ms polling interval */

        if (vdec_ioctl(ioctl_req, &core))
        {
            DWL_DEBUG("ioctl HX170DEC_IOCS_*_PULL_REG failed\n");
            ret = DWL_HW_WAIT_ERROR;
            break;
        }

        irq_stats = isPP ? dwlShadowRegs[coreID][HX170_IRQ_STAT_PP] :
                           dwlShadowRegs[coreID][HX170_IRQ_STAT_DEC];

        irq_stats = (irq_stats >> 11) & 0xFF;

        if(irq_stats != 0)
        {
            ret = DWL_HW_WAIT_OK;
            break;
        }

        usleep(usec);

        max_wait_time--;
    }
    while (max_wait_time > 0);

#endif

#ifdef _DWL_DEBUG
    {
        u32 irq_stats = isPP ? dwlShadowRegs[coreID][HX170_IRQ_STAT_PP] :
                               dwlShadowRegs[coreID][HX170_IRQ_STAT_DEC];

        PrintIrqType(isPP, coreID, irq_stats);
    }
#endif

    DWL_DEBUG("%s %d done\n", isPP ? "PP" : "DEC", coreID);

    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : DWLmalloc
    Description     : Allocate a memory block. Same functionality as
                      the ANSI C malloc()

    Return type     : void pointer to the allocated space, or NULL if there
                      is insufficient memory available

    Argument        : u32 n - Bytes to allocate
------------------------------------------------------------------------------*/
void *DWLmalloc(u32 n)
{
#ifdef MEMORY_USAGE_TRACE
    printf("DWLmalloc\t%8d bytes\n", n);
#endif
    return pvPortMalloc((size_t) n);
}

/*------------------------------------------------------------------------------
    Function name   : DWLfree
    Description     : Deallocates or frees a memory block. Same functionality as
                      the ANSI C free()

    Return type     : void

    Argument        : void *p - Previously allocated memory block to be freed
------------------------------------------------------------------------------*/
void DWLfree(void *p)
{
    if(p != NULL)
        vPortFree(p);
}

/*------------------------------------------------------------------------------
    Function name   : DWLcalloc
    Description     : Allocates an array in memory with elements initialized
                      to 0. Same functionality as the ANSI C calloc()

    Return type     : void pointer to the allocated space, or NULL if there
                      is insufficient memory available

}
    Argument        : u32 n - Number of elements
    Argument        : u32 s - Length in bytes of each element.
------------------------------------------------------------------------------*/
void *DWLcalloc(u32 n, u32 s)
{
#ifdef MEMORY_USAGE_TRACE
    printf("DWLcalloc\t%8d bytes\n", n * s);
#endif
    void *p = pvPortMalloc(n * s);
	if (p) memset(p, 0, n * s);
	return p;
}

/*------------------------------------------------------------------------------
    Function name   : DWLmemcpy
    Description     : Copies characters between buffers. Same functionality as
                      the ANSI C memcpy()

    Return type     : The value of destination d

    Argument        : void *d - Destination buffer
    Argument        : const void *s - Buffer to copy from
    Argument        : u32 n - Number of bytes to copy
------------------------------------------------------------------------------*/
void *DWLmemcpy(void *d, const void *s, u32 n)
{
    return memcpy(d, s, (size_t) n);
}

/*------------------------------------------------------------------------------
    Function name   : DWLmemset
    Description     : Sets buffers to a specified character. Same functionality
                      as the ANSI C memset()

    Return type     : The value of destination d

    Argument        : void *d - Pointer to destination
    Argument        : i32 c - Character to set
    Argument        : u32 n - Number of characters
------------------------------------------------------------------------------*/
void *DWLmemset(void *d, i32 c, u32 n)
{
    return memset(d, (int) c, (size_t) n);
}

/*------------------------------------------------------------------------------
    Function name   : DWLSoftResetAsic
    Description     : Soft reset hx170dec

    Return type     : none
------------------------------------------------------------------------------*/
void DWLSoftResetAsic(void)
{
    sys_soft_reset(softreset_jpeg);
}
