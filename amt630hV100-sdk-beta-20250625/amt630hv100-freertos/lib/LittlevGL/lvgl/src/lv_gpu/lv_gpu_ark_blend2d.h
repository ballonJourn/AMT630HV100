#ifndef LV_SRC_LV_GPU_LV_GPU_ARK_BLEND2D_H_
#define LV_SRC_LV_GPU_LV_GPU_ARK_BLEND2D_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../lv_misc/lv_area.h"
#include "../lv_misc/lv_color.h"

/*********************
 *      DEFINES
 *********************/

#ifndef LV_GPU_ARK_BLEND2D_BLIT_SIZE_LIMIT
/** Minimum area (in pixels) for image copy with 100% opacity to be handled by BLEND2D */
#define LV_GPU_ARK_BLEND2D_BLIT_SIZE_LIMIT 1024
#endif

#ifndef LV_GPU_ARK_BLEND2D_BLIT_OPA_SIZE_LIMIT
/** Minimum area (in pixels) for image copy with transparency to be handled by BLEND2D */
#define LV_GPU_ARK_BLEND2D_BLIT_OPA_SIZE_LIMIT 512
#endif

#ifndef LV_GPU_ARK_BLEND2D_FILL_SIZE_LIMIT
/** Minimum area (in pixels) to be filled by BLEND2D with 100% opacity */
#define LV_GPU_ARK_BLEND2D_FILL_SIZE_LIMIT 1024
#endif

#ifndef LV_GPU_ARK_BLEND2D_FILL_OPA_SIZE_LIMIT
/** Minimum area (in pixels) to be filled by BLEND2D with transparency */
#define LV_GPU_ARK_BLEND2D_FILL_OPA_SIZE_LIMIT 512
#endif

/**********************
 *      TYPEDEFS
 **********************/
/**
 * BLock Image Transfer descriptor structure
 */
typedef struct {

    const lv_color_t * src;  /**< Source buffer pointer (must be aligned on 32 bytes) */
    lv_area_t src_area;      /**< Area to be copied from source */
    lv_coord_t src_width;    /**< Source buffer width */
    lv_coord_t src_height;   /**< Source buffer height */
    uint32_t src_stride;     /**< Source buffer stride in bytes (must be aligned on 16 px) */

    const lv_color_t * dst;  /**< Destination buffer pointer (must be aligned on 32 bytes) */
    lv_area_t dst_area;      /**< Target area in destination buffer (must be the same as src_area) */
    lv_coord_t dst_width;    /**< Destination buffer width */
    lv_coord_t dst_height;   /**< Destination buffer height */
    uint32_t dst_stride;     /**< Destination buffer stride in bytes (must be aligned on 16 px) */

    lv_opa_t opa;            /**< Opacity - alpha mix (0 = source not copied, 255 = 100% opaque) */
	bool alpha_byte;         /**< true: extra alpha byte is inserted for every pixel */	
} lv_gpu_ark_blend2d_blit_info_t;


/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Reset and initialize BLEND2D device. This function should be called as a part
 * of display init sequence.
 *
 * @return LV_RES_OK: BLEND2D init ok; LV_RES_INV: init error. See error log for more information.
 */
lv_res_t lv_gpu_ark_blend2d_init(void);

/**
 * Disable BLEND2D device. Should be called during display deinit sequence.
 */
void lv_gpu_ark_blend2d_deinit(void);

/**
 * Fill area, with optional opacity.
 *
 * @param[in/out] dest_buf destination buffer
 * @param[in] dest_width width of destination buffer in pixels
 * @param[in] dest_height height of destination buffer in pixels
 * @param[in] fill_area area to fill
 * @param[in] color color
 * @param[in] opa transparency of the color
 */
lv_res_t lv_gpu_ark_blend2d_fill(lv_color_t * dest_buf, lv_coord_t dest_width, lv_coord_t dest_height,
						const lv_area_t * fill_area, lv_color_t color, lv_opa_t opa);

/***
 * BLock Image Transfer.
 * @param[in] blit Description of the transfer
 * @retval LV_RES_OK Transfer complete
 * @retval LV_RES_INV Error occurred (\see LV_GPU_NXP_VG_LITE_LOG_ERRORS)
 */
lv_res_t lv_gpu_ark_blend2d_blit(lv_gpu_ark_blend2d_blit_info_t * blit);



/**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_SRC_LV_GPU_LV_GPU_ARK_BLEND2D_H_ */
