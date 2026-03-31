/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../include/XM_scancode.h"

/* Windows scancode to XM scancode mapping table */
/* derived from Microsoft scan code document, http://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/scancode.doc */

/* *INDENT-OFF* */
static const XM_Scancode windows_scancode_table[] = 
{ 
	/*	0						1							2							3							4						5							6							7 */
	/*	8						9							A							B							C						D							E							F */
	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_ESCAPE,		XM_SCANCODE_1,				XM_SCANCODE_2,				XM_SCANCODE_3,			XM_SCANCODE_4,				XM_SCANCODE_5,				XM_SCANCODE_6,			/* 0 */
	XM_SCANCODE_7,				XM_SCANCODE_8,				XM_SCANCODE_9,				XM_SCANCODE_0,				XM_SCANCODE_MINUS,		XM_SCANCODE_EQUALS,		XM_SCANCODE_BACKSPACE,		XM_SCANCODE_TAB,		/* 0 */

	XM_SCANCODE_Q,				XM_SCANCODE_W,				XM_SCANCODE_E,				XM_SCANCODE_R,				XM_SCANCODE_T,			XM_SCANCODE_Y,				XM_SCANCODE_U,				XM_SCANCODE_I,			/* 1 */
	XM_SCANCODE_O,				XM_SCANCODE_P,				XM_SCANCODE_LEFTBRACKET,	XM_SCANCODE_RIGHTBRACKET,	XM_SCANCODE_RETURN,	XM_SCANCODE_LCTRL,			XM_SCANCODE_A,				XM_SCANCODE_S,			/* 1 */

	XM_SCANCODE_D,				XM_SCANCODE_F,				XM_SCANCODE_G,				XM_SCANCODE_H,				XM_SCANCODE_J,			XM_SCANCODE_K,				XM_SCANCODE_L,				XM_SCANCODE_SEMICOLON,	/* 2 */
	XM_SCANCODE_APOSTROPHE,	XM_SCANCODE_GRAVE,			XM_SCANCODE_LSHIFT,		XM_SCANCODE_BACKSLASH,		XM_SCANCODE_Z,			XM_SCANCODE_X,				XM_SCANCODE_C,				XM_SCANCODE_V,			/* 2 */

	XM_SCANCODE_B,				XM_SCANCODE_N,				XM_SCANCODE_M,				XM_SCANCODE_COMMA,			XM_SCANCODE_PERIOD,	XM_SCANCODE_SLASH,			XM_SCANCODE_RSHIFT,		XM_SCANCODE_PRINTSCREEN,/* 3 */
	XM_SCANCODE_LALT,			XM_SCANCODE_SPACE,			XM_SCANCODE_CAPSLOCK,		XM_SCANCODE_F1,			XM_SCANCODE_F2,		XM_SCANCODE_F3,			XM_SCANCODE_F4,			XM_SCANCODE_F5,		/* 3 */

	XM_SCANCODE_F6,			XM_SCANCODE_F7,			XM_SCANCODE_F8,			XM_SCANCODE_F9,			XM_SCANCODE_F10,		XM_SCANCODE_NUMLOCKCLEAR,	XM_SCANCODE_SCROLLLOCK,	XM_SCANCODE_HOME,		/* 4 */
	XM_SCANCODE_UP,			XM_SCANCODE_PAGEUP,		XM_SCANCODE_KP_MINUS,		XM_SCANCODE_LEFT,			XM_SCANCODE_KP_5,		XM_SCANCODE_RIGHT,			XM_SCANCODE_KP_PLUS,		XM_SCANCODE_END,		/* 4 */

	XM_SCANCODE_DOWN,			XM_SCANCODE_PAGEDOWN,		XM_SCANCODE_INSERT,		XM_SCANCODE_DELETE,		XM_SCANCODE_UNKNOWN,	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_NONUSBACKSLASH,XM_SCANCODE_F11,		/* 5 */
	XM_SCANCODE_F12,			XM_SCANCODE_PAUSE,			XM_SCANCODE_UNKNOWN,		XM_SCANCODE_LGUI,			XM_SCANCODE_RGUI,		XM_SCANCODE_APPLICATION,	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,	/* 5 */

	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_F13,		XM_SCANCODE_F14,			XM_SCANCODE_F15,			XM_SCANCODE_F16,		/* 6 */
	XM_SCANCODE_F17,			XM_SCANCODE_F18,			XM_SCANCODE_F19,			XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,	/* 6 */
	
	XM_SCANCODE_INTERNATIONAL2,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_INTERNATIONAL1,		XM_SCANCODE_UNKNOWN,	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN,	/* 7 */
	XM_SCANCODE_UNKNOWN,		XM_SCANCODE_INTERNATIONAL4,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_INTERNATIONAL5,		XM_SCANCODE_UNKNOWN,	XM_SCANCODE_INTERNATIONAL3,		XM_SCANCODE_UNKNOWN,		XM_SCANCODE_UNKNOWN	/* 7 */
};
/* *INDENT-ON* */
