#ifndef _TOUCH_H
#define _TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} calibration;

#define TOUCH_START_EVENT			1
#define TOUCH_STOP_EVENT			2
#define TOUCH_SAMPLE_EVENT			3

UINT32 AdjustTouch(void);

void InitializeCalibration(int a0, int a1, int a2, int a3, int a4, int a5, int a6);

int LoadTouchConfigure(void);

void TouchInit(void);

void TouchEventHandler(UINT32 ulEvent, UINT32 lpParam, UINT32 wParam);

#ifdef __cplusplus
}
#endif

#endif /* _TOUCH_H */


