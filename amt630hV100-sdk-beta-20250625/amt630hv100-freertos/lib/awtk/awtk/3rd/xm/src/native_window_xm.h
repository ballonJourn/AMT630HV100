/**
 * File:   native_window_xm.h
 * Author: ShenZhen ExceedSpace
 * Brief:  native window xm
 *
 * Copyright (c) 2019 - 2021  ShenZhen ExceedSpace Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2021-07-01 ZhuoYongHong created
 *
 */

#ifndef TK_NATIVE_WINDOW_XM_H
#define TK_NATIVE_WINDOW_XM_H

#include "base/native_window.h"

BEGIN_C_DECLS

ret_t native_window_xm_deinit(void);
ret_t native_window_xm_init(bool_t shared, uint32_t w, uint32_t h);

END_C_DECLS

#endif /*TK_NATIVE_WINDOW_XM_H*/
