/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : Decode NAL unit header
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: h264hwd_nal_unit.c,v $
--  $Date: 2008/05/28 11:08:48 $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

     1. Include headers
     2. External compiler flags
     3. Module defines
     4. Local function prototypes
     5. Functions
          h264bsdDecodeNalUnit

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "h264hwd_nal_unit.h"
#include "h264hwd_util.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function name: h264bsdDecodeNalUnit

        Functional description:
            Decode NAL unit header information

        Inputs:
            pStrmData       pointer to stream data structure

        Outputs:
            pNalUnit        NAL unit header information is stored here

        Returns:
            HANTRO_OK       success
            HANTRO_NOK      invalid NAL unit header information

------------------------------------------------------------------------------*/

u32 h264bsdDecodeNalUnit(strmData_t *pStrmData, nalUnit_t *pNalUnit)
{

/* Variables */

    u32 tmp;

/* Code */

    ASSERT(pStrmData);
    ASSERT(pNalUnit);
    ASSERT(pStrmData->bitPosInWord == 0);

    /* forbidden_zero_bit (not checked to be zero, errors ignored) */
    tmp = h264bsdGetBits(pStrmData, 1);
    /* Assuming that NAL unit starts from byte boundary ­> don't have to check
     * following 7 bits for END_OF_STREAM */
    if (tmp == END_OF_STREAM)
        return(HANTRO_NOK);

    tmp = h264bsdGetBits(pStrmData, 2);
    pNalUnit->nalRefIdc = tmp;

    tmp = h264bsdGetBits(pStrmData, 5);
    pNalUnit->nalUnitType = (nalUnitType_e)tmp;

    DEBUG_PRINT(("NAL TYPE %d\n", tmp));

    /* data partitioning NAL units not supported */
    if ( (tmp == NAL_CODED_SLICE_DP_A) ||
         (tmp == NAL_CODED_SLICE_DP_B) ||
         (tmp == NAL_CODED_SLICE_DP_C) )
    {
        ERROR_PRINT(("DP slices not allowed!!!"));
        return(HANTRO_NOK);
    }

    /* nal_ref_idc shall not be zero for these nal_unit_types */
    if ( ( (tmp == NAL_SEQ_PARAM_SET) || (tmp == NAL_PIC_PARAM_SET) ||
           (tmp == NAL_CODED_SLICE_IDR) ) && (pNalUnit->nalRefIdc == 0) )
    {
        ERROR_PRINT(("nal_ref_idc shall not be zero!!!"));
        return(HANTRO_NOK);
    }
    /* nal_ref_idc shall be zero for these nal_unit_types */
    else if ( ( (tmp == NAL_SEI) || (tmp == NAL_ACCESS_UNIT_DELIMITER) ||
                (tmp == NAL_END_OF_SEQUENCE) || (tmp == NAL_END_OF_STREAM) ||
                (tmp == NAL_FILLER_DATA) ) && (pNalUnit->nalRefIdc != 0) )
    {
        ERROR_PRINT(("nal_ref_idc shall be zero!!!"));
        return(HANTRO_NOK);
    }

    return(HANTRO_OK);

}

