#ifndef _KEYPAD_H
#define _KEYPAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_START_EVENT			1
#define KEY_STOP_EVENT			2
#define KEY_SAMPLE_EVENT		3

void KeypadInit(void);

void KeyEventHandler(UINT32 ulEvent, UINT32 lpParam, UINT32 wParam);

#ifdef __cplusplus
}
#endif
#endif

