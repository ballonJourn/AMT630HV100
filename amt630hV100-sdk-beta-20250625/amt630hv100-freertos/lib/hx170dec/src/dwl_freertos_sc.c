#include <string.h>
#include "FreeRTOS.h"

#include "basetype.h"
#include "dwl_freertos.h"
#include "dwl.h"
#include "hx170dec.h"

/*------------------------------------------------------------------------------
    Function name   : DWLInit
    Description     : Initialize a DWL instance

    Return type     : const void * - pointer to a DWL instance

    Argument        : void * param - not in use, application passes NULL
------------------------------------------------------------------------------*/
const void *DWLInit(DWLInitParam_t * param)
{
    hX170dwl_t *dec_dwl;

    dec_dwl = (hX170dwl_t *) pvPortMalloc(sizeof(hX170dwl_t));
	memset(dec_dwl, 0, sizeof(hX170dwl_t));

    DWL_DEBUG("INITIALIZE\n");

    if(dec_dwl == NULL)
    {
        DWL_DEBUG("failed to alloc hX170dwl_t struct\n");
        return NULL;
    }

    dec_dwl->clientType = param->clientType;

    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
    case DWL_CLIENT_TYPE_PP:
    {
        break;
    }
    default:
    {
        DWL_DEBUG("Unknown client type no. %d\n", dec_dwl->clientType);
        goto err;
    }
    }

    if(vdec_ioctl(HX170DEC_IOC_MC_CORES,  &dec_dwl->numCores) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOC_MC_CORES failed\n");
        goto err;
    }

    if(vdec_ioctl(HX170DEC_IOCGHWIOSIZE, &dec_dwl->regSize) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOCGHWIOSIZE failed\n");
        goto err;
    }

    DWL_DEBUG("SUCCESS\n");

    return dec_dwl;

err:

    DWL_DEBUG("FAILED\n");

    DWLRelease(dec_dwl);

    return NULL;
}

/*------------------------------------------------------------------------------
    Function name   : DWLRelease
    Description     : Release a DWl instance

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - instance to be released
------------------------------------------------------------------------------*/
i32 DWLRelease(const void *instance)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    configASSERT(dec_dwl != NULL);

    vPortFree(dec_dwl);

    DWL_DEBUG("SUCCESS\n");

    return (DWL_OK);
}


/*******************************************************************************
 Function name   : DWLSetShareRamDecoder
 Description     : Set share ram for decode use
 Return type     : void
 Argument        : hx280ewl_t*
*******************************************************************************/
void DWLSetShareRamDecoder(hX170dwl_t * dwl)
{
}

/* HW locking */

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHwPipe
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHwPipe(const void *instance/*, i32 *coreID*/)
{
    i32 ret;
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	i32 id = 0;
	i32 *coreID = &id;

    configASSERT(dec_dwl != NULL);
    configASSERT(dec_dwl->clientType != DWL_CLIENT_TYPE_PP);

    DWL_DEBUG("Start\n");

    /* reserve decoder */
    *coreID = vdec_ioctl(HX170DEC_IOCH_DEC_RESERVE, (void*)dec_dwl->clientType);

    if (*coreID != 0)
    {
        return DWL_ERROR;
    }

    /* reserve PP */
    ret = vdec_ioctl(HX170DEC_IOCQ_PP_RESERVE, NULL);

    /* for pipeline we expect same core for both dec and PP */
    if (ret != *coreID)
    {
        /* release the decoder */
        vdec_ioctl( HX170DEC_IOCT_DEC_RELEASE, coreID);
        return DWL_ERROR;
    }

    dec_dwl->bPPReserved = 1;

    DWL_DEBUG("Reserved DEC+PP core %d\n", *coreID);

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHw
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHw(const void *instance/*, i32 *coreID*/)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;
	i32 id = 0;
	i32 *coreID = &id;

    configASSERT(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    DWL_DEBUG(" %s\n", isPP ? "PP" : "DEC");

    if (isPP)
    {
        *coreID = vdec_ioctl(HX170DEC_IOCQ_PP_RESERVE, NULL);

        /* PP is single core so we expect a zero return value */
        if (*coreID != 0)
        {
            return DWL_ERROR;
        }
    }
    else
    {
        *coreID = vdec_ioctl(HX170DEC_IOCH_DEC_RESERVE,
                (void*)dec_dwl->clientType);
    }

    /* negative value signals an error */
    if (*coreID < 0)
    {
        DWL_DEBUG("ioctl HX170DEC_IOCS_%s_RESERVE failed\n",
                isPP ? "PP" : "DEC");
        return DWL_ERROR;
    }

#ifdef DEC_WITH_ENC
	DWLSetShareRamDecoder(dec_dwl);
#endif

    DWL_DEBUG("Reserved %s core %d\n", isPP ? "PP" : "DEC", *coreID);

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReleaseHw
    Description     :
    Return type     : void
    Argument        : const void *instance
------------------------------------------------------------------------------*/
void DWLReleaseHw(const void *instance/*, i32 coreID*/)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;
	i32 coreID = 0;

    configASSERT((u32)coreID < dec_dwl->numCores);
    configASSERT(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    if ((u32) coreID >= dec_dwl->numCores)
        return;

    DWL_DEBUG(" %s core %d\n", isPP ? "PP" : "DEC", coreID);

    if (isPP)
    {
        configASSERT(coreID == 0);

        vdec_ioctl(HX170DEC_IOCT_PP_RELEASE, (void*)coreID);
    }
    else
    {
        if (dec_dwl->bPPReserved)
        {
            /* decoder has reserved PP also => release it */
            DWL_DEBUG("DEC released PP core %d\n", coreID);

            dec_dwl->bPPReserved = 0;

            configASSERT(coreID == 0);

            vdec_ioctl(HX170DEC_IOCT_PP_RELEASE, (void*)coreID);
        }

        vdec_ioctl(HX170DEC_IOCT_DEC_RELEASE, (void*)coreID);
    }
}
