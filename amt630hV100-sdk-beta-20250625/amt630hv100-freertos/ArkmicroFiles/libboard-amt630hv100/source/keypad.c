#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "keypad.h"

#if !defined(VG_ONLY) && !defined(AWTK)
#include "lvgl/lvgl.h"
#else
typedef int16_t lv_coord_t;
typedef uint8_t lv_indev_state_t;
typedef struct {
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;

enum { LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR };

enum {
    LV_KEY_UP        = 17,  /*0x11*/
    LV_KEY_DOWN      = 18,  /*0x12*/
    LV_KEY_RIGHT     = 19,  /*0x13*/
    LV_KEY_LEFT      = 20,  /*0x14*/
    LV_KEY_ESC       = 27,  /*0x1B*/
    LV_KEY_DEL       = 127, /*0x7F*/
    LV_KEY_BACKSPACE = 8,   /*0x08*/
    LV_KEY_ENTER     = 10,  /*0x0A, '\n'*/
    LV_KEY_NEXT      = 9,   /*0x09, '\t'*/
    LV_KEY_PREV      = 11,  /*0x0B, '*/
    LV_KEY_HOME      = 2,   /*0x02, STX*/
    LV_KEY_END       = 3,   /*0x03, ETX*/
};

typedef struct {
    lv_point_t point; /**< For LV_INDEV_TYPE_POINTER the currently pressed point*/
    uint32_t key;     /**< For LV_INDEV_TYPE_KEYPAD the currently pressed key*/
    uint32_t btn_id;  /**< For LV_INDEV_TYPE_BUTTON the currently pressed button*/
    int16_t enc_diff; /**< For LV_INDEV_TYPE_ENCODER number of steps since the previous read*/

    lv_indev_state_t state; /**< LV_INDEV_STATE_REL or LV_INDEV_STATE_PR*/
} lv_indev_data_t;
#endif

#define KEY_FILTER_DATA         ///< 定义按键滤值功能

#define KEYAD_1		0  //10
#define KEYAD_2		1152  //0
#define KEYAD_3		1658 //1440
#define KEYAD_4		2268 //2080
#define KEYAD_5		2833 //2710
#define KEYAD_6		3367 //3170
#define KEYAD_7		4000 //3520
#define KEYAD_NUM	7

#ifndef KEY_FILTER_DATA
#define KEYAD_RANGE		30
#else
#define KEYAD_RANGE	  	500
#define KEY_ERROR_BY_NEIGHBOR_RANGE    (20)      ///< 按键稳定以后，相邻数据误差范围,值不要设的过大
#endif

static int KeyAdcValue[KEYAD_NUM] =
{
	KEYAD_1,
	KEYAD_2,
	KEYAD_3,
	KEYAD_4,
	KEYAD_5,
	KEYAD_6,
	KEYAD_7,
};

static unsigned int KeypadMap[KEYAD_NUM] = 
{
	LV_KEY_HOME,
	LV_KEY_ENTER,
	LV_KEY_ESC,
	LV_KEY_UP,
	LV_KEY_DOWN,
	LV_KEY_LEFT,
	LV_KEY_RIGHT,
}; 

#define NULL_KEY				0xFFFF
#define KEY_STATE_IDLE			0
#define KEY_STATE_START			1
#define KEY_STATE_PRESS			2

static UINT32 lg_ulKeyMachineState = KEY_STATE_IDLE;
static UINT32 lg_ulLastKey = NULL_KEY;

#define KEY_POOL_SIZE			8
static UINT32 lg_arrKeyAvgPool[KEY_POOL_SIZE];
static UINT32 lg_ulKeyIndex = 0;
char test_key_value = 0;

extern void SendKeypadInputEventFromISR(lv_indev_data_t *indata);

static void PushKeyToAVGPool(UINT32 ulKeyValue)
{
	if(lg_ulKeyIndex < KEY_POOL_SIZE)
	{
		lg_arrKeyAvgPool[lg_ulKeyIndex] = ulKeyValue;
		lg_ulKeyIndex++;
	}
}

static UINT32 AVGPoolIsFull(void)
{
	return lg_ulKeyIndex == KEY_POOL_SIZE;
}
					
static UINT32 GetKeyAVGValue(void)
{
	UINT32 i;
	UINT32 ulSum;

	ulSum = 0;
#ifndef KEY_FILTER_DATA
	for(i=0;i<lg_ulKeyIndex;i++)
	{
		ulSum += lg_arrKeyAvgPool[i];
	}

	return ulSum/lg_ulKeyIndex;
#else

    bool is_first = true;
    UINT32 count = 0;

    for (i = 0; i < lg_ulKeyIndex-1; i++) {
        if (abs(lg_arrKeyAvgPool[i] - lg_arrKeyAvgPool[i+1]) < KEY_ERROR_BY_NEIGHBOR_RANGE)
        {
            if (is_first)
            {
                is_first = false;
                ulSum += lg_arrKeyAvgPool[i];
                count++;
                //printf("i = %d value = %d\n", i, lg_arrKeyAvgPool[i]);
            }
            ulSum += lg_arrKeyAvgPool[i+1];
            count++;
            //printf("i = %d value = %d\n", i+1, lg_arrKeyAvgPool[i+1]);
        }
        else
        {
            is_first = true;
        }
    }
    if (count == 0)
        return 0;

   return ulSum/count;
#endif
}

static void ResetKeyAVGPool(void)
{
	lg_ulKeyIndex = 0;
}


static UINT32 CheckKey(UINT32 ulSampleValue)
{
#ifndef KEY_FILTER_DATA
	UINT32 i;
	
	for(i = 0; i < (sizeof(KeyAdcValue) / sizeof(KeyAdcValue[0])); i++ )
	{
		if((ulSampleValue >= KeyAdcValue[i] - KEYAD_RANGE) && (ulSampleValue <= KeyAdcValue[i] + KEYAD_RANGE))
		{
			return i;
		}
	}

	return NULL_KEY;
#else
	UINT32 i ;
    UINT32 id = 0;
    UINT32 range = abs(ulSampleValue - KeyAdcValue[0]);
	
	for (i = 1; i < (sizeof(KeyAdcValue) / sizeof(KeyAdcValue[0])); i++)
	{
		//if((ulSampleValue >= KeyAdcValue[i] - KEYAD_DOWN_RANGE) && (ulSampleValue <= KeyAdcValue[i] + KEYAD_UPWARD_RANGE))
		if (abs(ulSampleValue - KeyAdcValue[i]) < range)
		{
		    range = abs(ulSampleValue - KeyAdcValue[i]);
			id = i;
		}
	}

    if (range < KEYAD_RANGE)
    {
        return id;
    }
    
	return NULL_KEY;

#endif
}

static void SendKeyPress(UINT32 key)
{
	lv_indev_data_t indata = {0};
	indata.key = key;
	indata.state = LV_INDEV_STATE_PR;
	SendKeypadInputEventFromISR(&indata);
}

static void SendKeyRelease(UINT32 key)
{
	lv_indev_data_t indata = {0};
	indata.key = key;
	indata.state = LV_INDEV_STATE_REL;
	SendKeypadInputEventFromISR(&indata);
}

void KeyEventHandler(UINT32 ulEvent, UINT32 lpParam, UINT32 wParam)
{
	switch(lg_ulKeyMachineState)
	{
		case KEY_STATE_IDLE:
			if(ulEvent == KEY_START_EVENT)
			{
				lg_ulKeyMachineState = KEY_STATE_START;
			}
			break;
			
		case KEY_STATE_START:
			test_key_value = !test_key_value;
			if(ulEvent == KEY_SAMPLE_EVENT)
			{
				ResetKeyAVGPool();
				PushKeyToAVGPool(lpParam);
				lg_ulKeyMachineState = KEY_STATE_PRESS;
			}
			else if(ulEvent == KEY_STOP_EVENT)
			{
				lg_ulKeyMachineState = KEY_STATE_IDLE;
				lg_ulLastKey = NULL_KEY;
			}
			break;
			
		case KEY_STATE_PRESS:
			if(ulEvent == KEY_SAMPLE_EVENT)
			{
				PushKeyToAVGPool(lpParam);
				if(AVGPoolIsFull())
				{
					UINT32 ulKeySampleValue;
					UINT32 ulNewKeyCode;
					
					ulKeySampleValue = GetKeyAVGValue();
					ulNewKeyCode = CheckKey(ulKeySampleValue);
					ResetKeyAVGPool();
					
					if(ulNewKeyCode != NULL_KEY)
					{
						if(lg_ulLastKey != ulNewKeyCode && lg_ulLastKey != NULL_KEY)
						{
							lg_ulKeyMachineState = KEY_STATE_IDLE;
							//send key release event;							
							SendKeyRelease(KeypadMap[lg_ulLastKey]);
							lg_ulLastKey = NULL_KEY;
						}
						else if (lg_ulLastKey == ulNewKeyCode)
						{
							//Send repeat key event;
							SendKeyPress(KeypadMap[lg_ulLastKey]);
						}
						else if (lg_ulLastKey == NULL_KEY)
						{
							//Send key press event;
							SendKeyPress(KeypadMap[ulNewKeyCode]);
						}
						lg_ulLastKey = ulNewKeyCode;
					}
					else
					{
						lg_ulKeyMachineState = KEY_STATE_IDLE;
						if(lg_ulLastKey != NULL_KEY)
						{
							//send key release event
							SendKeyRelease(KeypadMap[lg_ulLastKey]);
							lg_ulLastKey = NULL_KEY;
						}
					}
				}
			}
			else if(ulEvent == KEY_STOP_EVENT)
			{
				lg_ulKeyMachineState = KEY_STATE_IDLE;
				if(lg_ulLastKey != NULL_KEY)
				{
					//send key release event
					SendKeyRelease(KeypadMap[lg_ulLastKey]);
					lg_ulLastKey = NULL_KEY;
				}
			}
			
			break;
	}
}

void KeypadInit(void)
{
	adc_channel_enable(ADC_CH_AUX0);
}
