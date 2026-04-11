/**
 * @file lv_openvg.c
 *
 */

#ifndef LVGL_VG_GPU

/*********************
 *      INCLUDES
 *********************/

#include "../lv_misc/lv_debug.h"
#include "../lv_themes/lv_theme.h"
#include "lv_openvg.h"
#include "lv_label.h"

#include "vg_driver.h"

#if LV_USE_OPENVG != 0

/*********************
 *      DEFINES
 *********************/
#define LV_OBJX_NAME "lv_openvg"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_design_res_t lv_openvg_design(lv_obj_t * openvg, const lv_area_t * clip_area, lv_design_mode_t mode);
static lv_res_t lv_openvg_signal(lv_obj_t * openvg, lv_signal_t sign, void * param);
static void refr_img(lv_obj_t * openvg);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_signal_cb_t ancestor_signal;
static lv_design_cb_t ancestor_design;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Create a openvg object
 * @param par pointer to an object, it will be the parent of the new openvg
 * @param copy pointer to a openvg object, if not NULL then the new object will be copied from
 * it
 * @return pointer to the created openvg
 */
lv_obj_t * lv_openvg_create(lv_obj_t * par, const lv_obj_t * copy)
{
    LV_LOG_TRACE("openvg create started");

    /*Create the ancestor of openvg*/
    lv_obj_t * openvg = lv_obj_create(par, copy);
    LV_ASSERT_MEM(openvg);
    if(openvg == NULL) return NULL;

    /*Allocate the openvg type specific extended data*/
    lv_openvg_ext_t * ext = lv_obj_allocate_ext_attr(openvg, sizeof(lv_openvg_ext_t));
    LV_ASSERT_MEM(ext);
    if(ext == NULL) {
        lv_obj_del(openvg);
        return NULL;
    }

    if(ancestor_signal == NULL) ancestor_signal = lv_obj_get_signal_cb(openvg);
    if(ancestor_design == NULL) ancestor_design = lv_obj_get_design_cb(openvg);

    /*Initialize the allocated 'ext' */
    ext->img_src_bg = NULL;
	ext->param = NULL;
    ext->act_cf = LV_IMG_CF_UNKNOWN;

    /*The signal and design functions are not copied so set them here*/
    lv_obj_set_signal_cb(openvg, lv_openvg_signal);
    lv_obj_set_design_cb(openvg, lv_openvg_design);

    /*Init the new openvg*/
    if(copy == NULL) {
        lv_theme_apply(openvg, LV_THEME_OPENVG);
    }
    /*Copy an existing openvg*/
    else {
        lv_openvg_ext_t * copy_ext = lv_obj_get_ext_attr(copy);
		ext->img_src_bg = copy_ext->img_src_bg;
        /*Refresh the style with new signal function*/
        lv_obj_refresh_style(openvg, LV_OBJ_PART_ALL, LV_STYLE_PROP_ALL);
    }

    LV_LOG_INFO("openvg created");

    return openvg;
}

/*=====================
 * Setter functions
 *====================*/

/**
 * Set images for a background of the openvg
 * @param openvg pointer to an openvg object
 * @param src pointer to an image source (a C array or path to a file)
 */
void lv_openvg_set_bg_src(lv_obj_t * openvg, const void * src)
{
    LV_ASSERT_OBJ(openvg, LV_OBJX_NAME);

    lv_openvg_ext_t * ext = lv_obj_get_ext_attr(openvg);

    ext->img_src_bg = src;

    refr_img(openvg);
}

/**
 * Set param for the openvg
 * @param openvg pointer to an openvg object
 * @param param
 */
void lv_openvg_set_param(lv_obj_t * openvg, void *param)
{
    LV_ASSERT_OBJ(openvg, LV_OBJX_NAME);

    lv_openvg_ext_t * ext = lv_obj_get_ext_attr(openvg);

    ext->param = param;

    refr_img(openvg);
}


/*=====================
 * Getter functions
 *====================*/

/**
 * Get the background image
 * @param openvg pointer to an openvg object
 * @return pointer to an image source (a C array or path to a file)
 */
const void * lv_openvg_get_bg_src(lv_obj_t * openvg)
{
    LV_ASSERT_OBJ(openvg, LV_OBJX_NAME);

    lv_openvg_ext_t * ext = lv_obj_get_ext_attr(openvg);

    return ext->img_src_bg;
}

/**
 * Get the openvg param
 * @param openvg pointer to an openvg object
 * @return param
 */
void *lv_openvg_get_param(lv_obj_t * openvg)
{
    LV_ASSERT_OBJ(openvg, LV_OBJX_NAME);

    lv_openvg_ext_t * ext = lv_obj_get_ext_attr(openvg);

    return ext->param;
}


/*=====================
 * Other functions
 *====================*/

/*
 * New object specific "other" functions come here
 */

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Handle the drawing related tasks of the openvgs
 * @param openvg pointer to an object
 * @param clip_area the object will be drawn only in this area
 * @param mode LV_DESIGN_COVER_CHK: only check if the object fully covers the 'mask_p' area
 *                                  (return 'true' if yes)
 *             LV_DESIGN_DRAW: draw the object (always return 'true')
 *             LV_DESIGN_DRAW_POST: drawing after every children are drawn
 * @param return an element of `lv_design_res_t`
 */
static lv_design_res_t lv_openvg_design(lv_obj_t * openvg, const lv_area_t * clip_area, lv_design_mode_t mode)
{
    /*Return false if the object is not covers the mask_p area*/
    if(mode == LV_DESIGN_COVER_CHK) {
        lv_openvg_ext_t * ext = lv_obj_get_ext_attr(openvg);
        lv_design_res_t cover = LV_DESIGN_RES_NOT_COVER;
        if(ext->act_cf == LV_IMG_CF_TRUE_COLOR || ext->act_cf == LV_IMG_CF_RAW) {
            cover = _lv_area_is_in(clip_area, &openvg->coords, 0) ? LV_DESIGN_RES_COVER : LV_DESIGN_RES_NOT_COVER;
        }

        return cover;
    }
    /*Draw the object*/
    else if(mode == LV_DESIGN_DRAW_MAIN) {
        /*Draw background image*/
        lv_openvg_ext_t * ext    = lv_obj_get_ext_attr(openvg);
		//uint32_t t1 = xTaskGetTickCount();
#if 0		
        const void * src = ext->img_src_bg;
        if(lv_img_src_get_type(src) == LV_IMG_SRC_SYMBOL) {
            lv_draw_label_dsc_t label_dsc;
            lv_draw_label_dsc_init(&label_dsc);
            lv_obj_init_draw_label_dsc(openvg, LV_OPENVG_PART_MAIN, &label_dsc);
            lv_draw_label(&openvg->coords, clip_area, &label_dsc, src, NULL);
        }
        else {
            lv_draw_img_dsc_t img_dsc;
            lv_draw_img_dsc_init(&img_dsc);
            lv_obj_init_draw_img_dsc(openvg, LV_OPENVG_PART_MAIN, &img_dsc);
            lv_draw_img(&openvg->coords, clip_area, src, &img_dsc);
        }
#endif
		if (ext->param) {
			xm_vg_draw_prepare(ext->param);
			xm_vg_draw_start();
		}
		//uint32_t t2 = xTaskGetTickCount();
		//printf("vg tick %d.\n", t2 - t1);
    }

    return LV_DESIGN_RES_OK;
}

/**
 * Signal function of the openvg
 * @param openvg pointer to a openvg object
 * @param sign a signal type from lv_signal_t enum
 * @param param pointer to a signal specific variable
 * @return LV_RES_OK: the object is not deleted in the function; LV_RES_INV: the object is deleted
 */
static lv_res_t lv_openvg_signal(lv_obj_t * openvg, lv_signal_t sign, void * param)
{
    lv_res_t res;

    /* Include the ancient signal function */
    res = ancestor_signal(openvg, sign, param);
    if(res != LV_RES_OK) return res;
    if(sign == LV_SIGNAL_GET_TYPE) return lv_obj_handle_get_type_signal(param, LV_OBJX_NAME);

    if(sign == LV_SIGNAL_STYLE_CHG) {
        refr_img(openvg);
    }
    else if(sign == LV_SIGNAL_REFR_EXT_DRAW_PAD) {
        /*Handle the padding of the background*/
        lv_style_int_t left = lv_obj_get_style_pad_left(openvg, LV_OPENVG_PART_MAIN);
        lv_style_int_t right = lv_obj_get_style_pad_right(openvg, LV_OPENVG_PART_MAIN);
        lv_style_int_t top = lv_obj_get_style_pad_top(openvg, LV_OPENVG_PART_MAIN);
        lv_style_int_t bottom = lv_obj_get_style_pad_bottom(openvg, LV_OPENVG_PART_MAIN);

        openvg->ext_draw_pad = LV_MATH_MAX(openvg->ext_draw_pad, left);
        openvg->ext_draw_pad = LV_MATH_MAX(openvg->ext_draw_pad, right);
        openvg->ext_draw_pad = LV_MATH_MAX(openvg->ext_draw_pad, top);
        openvg->ext_draw_pad = LV_MATH_MAX(openvg->ext_draw_pad, bottom);
    }
    else if(sign == LV_SIGNAL_PRESSED || sign == LV_SIGNAL_RELEASED || sign == LV_SIGNAL_PRESS_LOST) {
        refr_img(openvg);
    }
    else if(sign == LV_SIGNAL_CLEANUP) {
        /*Nothing to cleanup. (No dynamically allocated memory in 'ext')*/
    }

    return res;
}

static void refr_img(lv_obj_t * openvg)
{
    lv_openvg_ext_t * ext = lv_obj_get_ext_attr(openvg);
    lv_img_header_t header;

    const void * src = ext->img_src_bg;
    if(src == NULL) {
		lv_obj_invalidate(openvg);
		return;
    }

    lv_res_t info_res = LV_RES_OK;
    if(lv_img_src_get_type(src) == LV_IMG_SRC_SYMBOL) {
        const lv_font_t * font = lv_obj_get_style_text_font(openvg, LV_OPENVG_PART_MAIN);
        header.h = lv_font_get_line_height(font);
        header.w = _lv_txt_get_width(src, (uint16_t)strlen(src), font, 0, LV_TXT_FLAG_NONE);
        header.always_zero = 0;
        header.cf = LV_IMG_CF_ALPHA_1BIT;
    }
    else {
        info_res = lv_img_decoder_get_info(src, &header);
    }

    if(info_res == LV_RES_OK) {
        ext->act_cf = header.cf;
        lv_obj_set_size(openvg, header.w, header.h);
    }
    else {
        ext->act_cf = LV_IMG_CF_UNKNOWN;
    }

    lv_obj_invalidate(openvg);
}

#endif

#endif
