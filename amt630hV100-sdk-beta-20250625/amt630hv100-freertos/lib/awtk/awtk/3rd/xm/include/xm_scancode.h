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

/**
 *  \file XM_scancode.h
 *
 *  Defines keyboard scancodes.
 */

#ifndef XM_scancode_h_
#define XM_scancode_h_


/**
 *  \brief The XM keyboard scancode representation.
 *
 *  Values of this type are used to represent keyboard keys, among other places
 *  in the \link XM_Keysym::scancode key.keysym.scancode \endlink field of the
 *  XM_Event structure.
 *
 *  The values in this enumeration are based on the USB usage page standard:
 *  http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
typedef enum
{
    XM_SCANCODE_UNKNOWN = 0,

    /**
     *  \name Usage page 0x07
     *
     *  These values are from usage page 0x07 (USB keyboard page).
     */
    /* @{ */

    XM_SCANCODE_A = 4,
    XM_SCANCODE_B = 5,
    XM_SCANCODE_C = 6,
    XM_SCANCODE_D = 7,
    XM_SCANCODE_E = 8,
    XM_SCANCODE_F = 9,
    XM_SCANCODE_G = 10,
    XM_SCANCODE_H = 11,
    XM_SCANCODE_I = 12,
    XM_SCANCODE_J = 13,
    XM_SCANCODE_K = 14,
    XM_SCANCODE_L = 15,
    XM_SCANCODE_M = 16,
    XM_SCANCODE_N = 17,
    XM_SCANCODE_O = 18,
    XM_SCANCODE_P = 19,
    XM_SCANCODE_Q = 20,
    XM_SCANCODE_R = 21,
    XM_SCANCODE_S = 22,
    XM_SCANCODE_T = 23,
    XM_SCANCODE_U = 24,
    XM_SCANCODE_V = 25,
    XM_SCANCODE_W = 26,
    XM_SCANCODE_X = 27,
    XM_SCANCODE_Y = 28,
    XM_SCANCODE_Z = 29,

    XM_SCANCODE_1 = 30,
    XM_SCANCODE_2 = 31,
    XM_SCANCODE_3 = 32,
    XM_SCANCODE_4 = 33,
    XM_SCANCODE_5 = 34,
    XM_SCANCODE_6 = 35,
    XM_SCANCODE_7 = 36,
    XM_SCANCODE_8 = 37,
    XM_SCANCODE_9 = 38,
    XM_SCANCODE_0 = 39,

    XM_SCANCODE_RETURN = 40,
    XM_SCANCODE_ESCAPE = 41,
    XM_SCANCODE_BACKSPACE = 42,
    XM_SCANCODE_TAB = 43,
    XM_SCANCODE_SPACE = 44,

    XM_SCANCODE_MINUS = 45,
    XM_SCANCODE_EQUALS = 46,
    XM_SCANCODE_LEFTBRACKET = 47,
    XM_SCANCODE_RIGHTBRACKET = 48,
    XM_SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
    XM_SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate XM_SCANCODE_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because XM
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
    XM_SCANCODE_SEMICOLON = 51,
    XM_SCANCODE_APOSTROPHE = 52,
    XM_SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
    XM_SCANCODE_COMMA = 54,
    XM_SCANCODE_PERIOD = 55,
    XM_SCANCODE_SLASH = 56,

    XM_SCANCODE_CAPSLOCK = 57,

    XM_SCANCODE_F1 = 58,
    XM_SCANCODE_F2 = 59,
    XM_SCANCODE_F3 = 60,
    XM_SCANCODE_F4 = 61,
    XM_SCANCODE_F5 = 62,
    XM_SCANCODE_F6 = 63,
    XM_SCANCODE_F7 = 64,
    XM_SCANCODE_F8 = 65,
    XM_SCANCODE_F9 = 66,
    XM_SCANCODE_F10 = 67,
    XM_SCANCODE_F11 = 68,
    XM_SCANCODE_F12 = 69,

    XM_SCANCODE_PRINTSCREEN = 70,
    XM_SCANCODE_SCROLLLOCK = 71,
    XM_SCANCODE_PAUSE = 72,
    XM_SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    XM_SCANCODE_HOME = 74,
    XM_SCANCODE_PAGEUP = 75,
    XM_SCANCODE_DELETE = 76,
    XM_SCANCODE_END = 77,
    XM_SCANCODE_PAGEDOWN = 78,
    XM_SCANCODE_RIGHT = 79,
    XM_SCANCODE_LEFT = 80,
    XM_SCANCODE_DOWN = 81,
    XM_SCANCODE_UP = 82,

    XM_SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
    XM_SCANCODE_KP_DIVIDE = 84,
    XM_SCANCODE_KP_MULTIPLY = 85,
    XM_SCANCODE_KP_MINUS = 86,
    XM_SCANCODE_KP_PLUS = 87,
    XM_SCANCODE_KP_ENTER = 88,
    XM_SCANCODE_KP_1 = 89,
    XM_SCANCODE_KP_2 = 90,
    XM_SCANCODE_KP_3 = 91,
    XM_SCANCODE_KP_4 = 92,
    XM_SCANCODE_KP_5 = 93,
    XM_SCANCODE_KP_6 = 94,
    XM_SCANCODE_KP_7 = 95,
    XM_SCANCODE_KP_8 = 96,
    XM_SCANCODE_KP_9 = 97,
    XM_SCANCODE_KP_0 = 98,
    XM_SCANCODE_KP_PERIOD = 99,

    XM_SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
    XM_SCANCODE_APPLICATION = 101, /**< windows contextual menu, compose */
    XM_SCANCODE_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
    XM_SCANCODE_KP_EQUALS = 103,
    XM_SCANCODE_F13 = 104,
    XM_SCANCODE_F14 = 105,
    XM_SCANCODE_F15 = 106,
    XM_SCANCODE_F16 = 107,
    XM_SCANCODE_F17 = 108,
    XM_SCANCODE_F18 = 109,
    XM_SCANCODE_F19 = 110,
    XM_SCANCODE_F20 = 111,
    XM_SCANCODE_F21 = 112,
    XM_SCANCODE_F22 = 113,
    XM_SCANCODE_F23 = 114,
    XM_SCANCODE_F24 = 115,
    XM_SCANCODE_EXECUTE = 116,
    XM_SCANCODE_HELP = 117,
    XM_SCANCODE_MENU = 118,
    XM_SCANCODE_SELECT = 119,
    XM_SCANCODE_STOP = 120,
    XM_SCANCODE_AGAIN = 121,   /**< redo */
    XM_SCANCODE_UNDO = 122,
    XM_SCANCODE_CUT = 123,
    XM_SCANCODE_COPY = 124,
    XM_SCANCODE_PASTE = 125,
    XM_SCANCODE_FIND = 126,
    XM_SCANCODE_MUTE = 127,
    XM_SCANCODE_VOLUMEUP = 128,
    XM_SCANCODE_VOLUMEDOWN = 129,
/* not sure whether there's a reason to enable these */
/*     XM_SCANCODE_LOCKINGCAPSLOCK = 130,  */
/*     XM_SCANCODE_LOCKINGNUMLOCK = 131, */
/*     XM_SCANCODE_LOCKINGSCROLLLOCK = 132, */
    XM_SCANCODE_KP_COMMA = 133,
    XM_SCANCODE_KP_EQUALSAS400 = 134,

    XM_SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    XM_SCANCODE_INTERNATIONAL2 = 136,
    XM_SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
    XM_SCANCODE_INTERNATIONAL4 = 138,
    XM_SCANCODE_INTERNATIONAL5 = 139,
    XM_SCANCODE_INTERNATIONAL6 = 140,
    XM_SCANCODE_INTERNATIONAL7 = 141,
    XM_SCANCODE_INTERNATIONAL8 = 142,
    XM_SCANCODE_INTERNATIONAL9 = 143,
    XM_SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
    XM_SCANCODE_LANG2 = 145, /**< Hanja conversion */
    XM_SCANCODE_LANG3 = 146, /**< Katakana */
    XM_SCANCODE_LANG4 = 147, /**< Hiragana */
    XM_SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
    XM_SCANCODE_LANG6 = 149, /**< reserved */
    XM_SCANCODE_LANG7 = 150, /**< reserved */
    XM_SCANCODE_LANG8 = 151, /**< reserved */
    XM_SCANCODE_LANG9 = 152, /**< reserved */

    XM_SCANCODE_ALTERASE = 153, /**< Erase-Eaze */
    XM_SCANCODE_SYSREQ = 154,
    XM_SCANCODE_CANCEL = 155,
    XM_SCANCODE_CLEAR = 156,
    XM_SCANCODE_PRIOR = 157,
    XM_SCANCODE_RETURN2 = 158,
    XM_SCANCODE_SEPARATOR = 159,
    XM_SCANCODE_OUT = 160,
    XM_SCANCODE_OPER = 161,
    XM_SCANCODE_CLEARAGAIN = 162,
    XM_SCANCODE_CRSEL = 163,
    XM_SCANCODE_EXSEL = 164,

    XM_SCANCODE_KP_00 = 176,
    XM_SCANCODE_KP_000 = 177,
    XM_SCANCODE_THOUSANDSSEPARATOR = 178,
    XM_SCANCODE_DECIMALSEPARATOR = 179,
    XM_SCANCODE_CURRENCYUNIT = 180,
    XM_SCANCODE_CURRENCYSUBUNIT = 181,
    XM_SCANCODE_KP_LEFTPAREN = 182,
    XM_SCANCODE_KP_RIGHTPAREN = 183,
    XM_SCANCODE_KP_LEFTBRACE = 184,
    XM_SCANCODE_KP_RIGHTBRACE = 185,
    XM_SCANCODE_KP_TAB = 186,
    XM_SCANCODE_KP_BACKSPACE = 187,
    XM_SCANCODE_KP_A = 188,
    XM_SCANCODE_KP_B = 189,
    XM_SCANCODE_KP_C = 190,
    XM_SCANCODE_KP_D = 191,
    XM_SCANCODE_KP_E = 192,
    XM_SCANCODE_KP_F = 193,
    XM_SCANCODE_KP_XOR = 194,
    XM_SCANCODE_KP_POWER = 195,
    XM_SCANCODE_KP_PERCENT = 196,
    XM_SCANCODE_KP_LESS = 197,
    XM_SCANCODE_KP_GREATER = 198,
    XM_SCANCODE_KP_AMPERSAND = 199,
    XM_SCANCODE_KP_DBLAMPERSAND = 200,
    XM_SCANCODE_KP_VERTICALBAR = 201,
    XM_SCANCODE_KP_DBLVERTICALBAR = 202,
    XM_SCANCODE_KP_COLON = 203,
    XM_SCANCODE_KP_HASH = 204,
    XM_SCANCODE_KP_SPACE = 205,
    XM_SCANCODE_KP_AT = 206,
    XM_SCANCODE_KP_EXCLAM = 207,
    XM_SCANCODE_KP_MEMSTORE = 208,
    XM_SCANCODE_KP_MEMRECALL = 209,
    XM_SCANCODE_KP_MEMCLEAR = 210,
    XM_SCANCODE_KP_MEMADD = 211,
    XM_SCANCODE_KP_MEMSUBTRACT = 212,
    XM_SCANCODE_KP_MEMMULTIPLY = 213,
    XM_SCANCODE_KP_MEMDIVIDE = 214,
    XM_SCANCODE_KP_PLUSMINUS = 215,
    XM_SCANCODE_KP_CLEAR = 216,
    XM_SCANCODE_KP_CLEARENTRY = 217,
    XM_SCANCODE_KP_BINARY = 218,
    XM_SCANCODE_KP_OCTAL = 219,
    XM_SCANCODE_KP_DECIMAL = 220,
    XM_SCANCODE_KP_HEXADECIMAL = 221,

    XM_SCANCODE_LCTRL = 224,
    XM_SCANCODE_LSHIFT = 225,
    XM_SCANCODE_LALT = 226, /**< alt, option */
    XM_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
    XM_SCANCODE_RCTRL = 228,
    XM_SCANCODE_RSHIFT = 229,
    XM_SCANCODE_RALT = 230, /**< alt gr, option */
    XM_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

    XM_SCANCODE_MODE = 257,    /**< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special KMOD_MODE for it I'm adding it here
                                 */

    /* @} *//* Usage page 0x07 */

    /**
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    XM_SCANCODE_AUDIONEXT = 258,
    XM_SCANCODE_AUDIOPREV = 259,
    XM_SCANCODE_AUDIOSTOP = 260,
    XM_SCANCODE_AUDIOPLAY = 261,
    XM_SCANCODE_AUDIOMUTE = 262,
    XM_SCANCODE_MEDIASELECT = 263,
    XM_SCANCODE_WWW = 264,
    XM_SCANCODE_MAIL = 265,
    XM_SCANCODE_CALCULATOR = 266,
    XM_SCANCODE_COMPUTER = 267,
    XM_SCANCODE_AC_SEARCH = 268,
    XM_SCANCODE_AC_HOME = 269,
    XM_SCANCODE_AC_BACK = 270,
    XM_SCANCODE_AC_FORWARD = 271,
    XM_SCANCODE_AC_STOP = 272,
    XM_SCANCODE_AC_REFRESH = 273,
    XM_SCANCODE_AC_BOOKMARKS = 274,

    /* @} *//* Usage page 0x0C */

    /**
     *  \name Walther keys
     *
     *  These are values that Christian Walther added (for mac keyboard?).
     */
    /* @{ */

    XM_SCANCODE_BRIGHTNESSDOWN = 275,
    XM_SCANCODE_BRIGHTNESSUP = 276,
    XM_SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                           switch, video mode switch */
    XM_SCANCODE_KBDILLUMTOGGLE = 278,
    XM_SCANCODE_KBDILLUMDOWN = 279,
    XM_SCANCODE_KBDILLUMUP = 280,
    XM_SCANCODE_EJECT = 281,
    XM_SCANCODE_SLEEP = 282,

    XM_SCANCODE_APP1 = 283,
    XM_SCANCODE_APP2 = 284,

    /* @} *//* Walther keys */

    /**
     *  \name Usage page 0x0C (additional media keys)
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    XM_SCANCODE_AUDIOREWIND = 285,
    XM_SCANCODE_AUDIOFASTFORWARD = 286,

    /* @} *//* Usage page 0x0C (additional media keys) */

    /* Add any other keys here. */

    XM_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                 for array bounds */
} XM_Scancode;

#endif /* XM_scancode_h_ */

/* vi: set ts=4 sw=4 expandtab: */
