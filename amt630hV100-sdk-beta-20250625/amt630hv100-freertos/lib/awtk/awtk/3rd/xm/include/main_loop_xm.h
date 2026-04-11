/**
 * File:   main_loop_xm.h
 * Author: ZhuoYongHong
 * Brief:  XM implemented main_loop interface
 *
 * Copyright (c) 2021 - 2025  ShenZhen ExceedSpace Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2021-04-17 ZhuoYongHong created
 *
 */

#ifndef TK_MAIN_LOOP_XM_H
#define TK_MAIN_LOOP_XM_H

#include "base/main_loop.h"

BEGIN_C_DECLS

main_loop_t* main_loop_init(int w, int h);

END_C_DECLS

#endif /*TK_MAIN_LOOP_XM_H*/
