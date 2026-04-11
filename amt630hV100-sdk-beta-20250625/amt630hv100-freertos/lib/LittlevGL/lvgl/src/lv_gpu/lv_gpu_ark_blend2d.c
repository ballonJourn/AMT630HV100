/*********************
 *      INCLUDES
 *********************/

#include "../lv_conf_internal.h"

#if LV_USE_GPU_ARK_BLEND2D

#include "lvgl/lvgl.h"
#include "lv_gpu_ark_blend2d.h"
#include "../lv_misc/lv_mem.h"
#include "../lv_misc/lv_log.h"

#include "blend2d.h"
#include "cp15/cp15.h"

/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_16_SWAP
    #error Color swap not implemented. Disable LV_COLOR_16_SWAP feature.
#endif

#if LV_COLOR_DEPTH==16
    #define BLEND2D_OUT_PIXEL_FORMAT BLEND2D_FORAMT_RGB565
#else
	#define BLEND2D_OUT_PIXEL_FORMAT BLEND2D_FORAMT_ARGB888
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_res_t lv_gpu_ark_blend2d_run(void);
static void lv_gpu_ark_invalidate_cache(uint32_t address, uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t pxSize);

static void lv_gpu_ark_clean_cache(uint32_t address, uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t pxSize);
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
lv_res_t lv_gpu_ark_blend2d_init(void)
{
    return LV_RES_OK;
}

/**
 * Disable BLEND2D device. Should be called during display deinit sequence.
 */
void lv_gpu_ark_blend2d_deinit(void)
{

}

/**
 * Fill area, with optional opacity.
 *
 * @param[in/out] dest_buf destination buffer
 * @param[in] dest_width width of destination buffer in pixels
 * @param[in] dest_height height of destination buffer in pixels
 * @param[in] fill_area area to fill
 * @param[in] color color
 * @param[in] opa transparency of the color
 * @retval LV_RES_OK Transfer complete
 * @retval LV_RES_INV Error occurred
 */
lv_res_t lv_gpu_ark_blend2d_fill(lv_color_t * dest_buf, lv_coord_t dest_width, lv_coord_t dest_height,
						const lv_area_t * fill_area, lv_color_t color, lv_opa_t opa)

{
	int32_t width = fill_area->x2 - fill_area->x1 + 1;
    int32_t height = fill_area->y2 - fill_area->y1 + 1;
    uint32_t start_address = (uint32_t)(dest_buf + dest_width * fill_area->y1 + fill_area->x1);
	uint8_t r, g, b;

#if LV_COLOR_DEPTH==16
	if (fill_area->x1 & 1 || width & 1)
		return LV_RES_INV;
#endif

    lv_gpu_ark_invalidate_cache(start_address, width, height, dest_width * sizeof(lv_color_t),
                                sizeof(lv_color_t));

#if LV_COLOR_DEPTH==16
	r = LV_COLOR_GET_R(color) << 3;
	g = LV_COLOR_GET_G(color) << 2;
	b = LV_COLOR_GET_B(color) << 3;
#else
	r = LV_COLOR_GET_R(color);
	g = LV_COLOR_GET_G(color);
	b = LV_COLOR_GET_B(color);
#endif

	blend2d_fill((uint32_t)dest_buf, fill_area->x1, fill_area->y1, width, height, dest_width, dest_height,
		r, g, b, BLEND2D_OUT_PIXEL_FORMAT, opa, 0);

    return lv_gpu_ark_blend2d_run(); /* Start BLEND2D task */
}

/***
 * BLock Image Transfer.
 * @param[in] blit Description of the transfer
 * @retval LV_RES_OK Transfer complete
 * @retval LV_RES_INV Error occurred
 */
lv_res_t lv_gpu_ark_blend2d_blit(lv_gpu_ark_blend2d_blit_info_t * blit)

{
	int width, height;
	uint32_t start_address = (uint32_t)(blit->dst + blit->dst_width * blit->dst_area.y1 + blit->dst_area.x1);

	width = blit->src_area.x2 - blit->src_area.x1;
	height = blit->src_area.y2 - blit->src_area.y1;
	if (blit->dst_area.x2 - blit->dst_area.x1 != width || blit->dst_area.y2 - blit->dst_area.y1 != height)
		return LV_RES_INV;
	
#if LV_COLOR_DEPTH==16
	if (blit->src_area.x1 & 1 || blit->dst_area.x1 & 1 || width & 1)
		return LV_RES_INV;
#endif

    lv_gpu_ark_invalidate_cache(start_address, width, height, blit->dst_width * sizeof(lv_color_t),
                                sizeof(lv_color_t));

	start_address = (uint32_t)(blit->src + blit->src_width * blit->src_area.y1 + blit->src_area.x1);
	lv_gpu_ark_clean_cache(start_address, width, height, blit->src_width * sizeof(lv_color_t),
                                sizeof(lv_color_t));

	blend2d_blit((uint32_t)blit->dst, blit->dst_width, blit->dst_height, blit->dst_area.x1, blit->dst_area.y1, BLEND2D_OUT_PIXEL_FORMAT,
		width, height, (uint32_t)blit->src, blit->src_width, blit->src_height, blit->src_area.x1, blit->src_area.y1,
		BLEND2D_OUT_PIXEL_FORMAT, blit->opa, blit->alpha_byte);

	return lv_gpu_ark_blend2d_run(); /* Start BLEND2D task */
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Start BLEND2D job and wait for results
 *
 * Function used internally to start BLEND2D task according current device
 * configuration.
 */
static lv_res_t lv_gpu_ark_blend2d_run(void)
{
	if (!blend2d_run())
		return LV_RES_OK;
	else
		return LV_RES_INV;
}

/**
 * @brief Invalidate cache for rectangular area of memory
 *
 * @param[in] address starting address of area
 * @param[in] width width of area in pixels
 * @param[in] height height of area in pixels
 * @param[in] stride stride in bytes
 * @param[in] pxSize pixel size in bytes
 */
static void lv_gpu_ark_invalidate_cache(uint32_t address, uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t pxSize)
{
    int y;

    for(y = 0; y < height; y++) {
        CP15_flush_dcache_for_dma(address, address + width * pxSize);
        address += stride;
    }
}

/**
 * @brief Clean cache for rectangular area of memory
 *
 * @param[in] address starting address of area
 * @param[in] width width of area in pixels
 * @param[in] height height of area in pixels
 * @param[in] stride stride in bytes
 * @param[in] pxSize pixel size in bytes
 */
static void lv_gpu_ark_clean_cache(uint32_t address, uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t pxSize)
{
    int y;

    for(y = 0; y < height; y++) {
        CP15_clean_dcache_for_dma(address, address + width * pxSize);
        address += stride;
    }
}										
#endif /* LV_USE_GPU && LV_USE_GPU_ARK_BLEND2D */
