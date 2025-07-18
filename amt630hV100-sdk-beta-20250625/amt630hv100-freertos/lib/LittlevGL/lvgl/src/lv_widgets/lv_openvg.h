/**
 * @file lv_openvg.h
 *
 */

#ifndef LV_OPENVG_H
#define LV_OPENVG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"

#if LV_USE_OPENVG != 0

/*Testing of dependencies*/
#if LV_USE_IMG == 0
#error "lv_openvg: lv_img is required. Enable it in lv_conf.h (LV_USE_IMG 1)"
#endif

#include "../lv_core/lv_obj.h"
#include "lv_img.h"
#include "../lv_draw/lv_draw_img.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
/*Data of image button*/
typedef struct {
    const void * img_src_bg;   /*background image*/
	void *param;
    lv_img_cf_t act_cf; /*Color format of the currently active image*/
} lv_openvg_ext_t;

/*Parts of the image button*/
enum {
    LV_OPENVG_PART_MAIN = LV_IMG_PART_MAIN,
};
typedef uint8_t lv_openvg_part_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a image button objects
 * @param par pointer to an object, it will be the parent of the new image button
 * @param copy pointer to a image button object, if not NULL then the new object will be copied from
 * it
 * @return pointer to the created image button
 */
lv_obj_t * lv_openvg_create(lv_obj_t * par, const lv_obj_t * copy);

/*======================
 * Add/remove functions
 *=====================*/

/*=====================
 * Setter functions
 *====================*/

/**
 * Set images for a background of the openvg
 * @param openvg pointer to an openvg object
 * @param src pointer to an image source (a C array or path to a file)
 */
void lv_openvg_set_bg_src(lv_obj_t * openvg, const void * src);

/**
 * Set param for the openvg
 * @param openvg pointer to an openvg object
 * @param param
 */
void lv_openvg_set_param(lv_obj_t * openvg, void *param);

/**
 * Get the background image
 * @param openvg pointer to an openvg objects
 * @return pointer to an image source (a C array or path to a file)
 */
const void * lv_openvg_get_bg_src(lv_obj_t * openvg);

/**
 * Get the openvg param
 * @param openvg pointer to an openvg object
 * @return param
 */
void * lv_openvg_get_param(lv_obj_t * openvg);


/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_OPENVG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_OPENVG_H*/
