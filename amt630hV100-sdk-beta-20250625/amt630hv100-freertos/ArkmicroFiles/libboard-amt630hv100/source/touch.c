#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "touch.h"

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

#ifdef ADC_TOUCH

#define TOUCH_STATE_IDLE			0
#define TOUCH_STATE_START			1
#define TOUCH_STATE_SAMPLE			2
#define TOUCH_STATE_STOP			3

#define MAX_POINT_POOL_SIZE			2
#define FILTER_MAX              	3

#define  MAXPANADCLEN    			20

typedef struct {
	int x;
	int y;
} POINT;

typedef struct
{
	UINT16 adcX;
	UINT16 adcY;
} PAN_DATA;

typedef struct
{
	UINT32 *pValue;
	UINT32 header;
	UINT32 trail;
	UINT32 quelen;
} ADCValueQueue;

static calibration cal;
static UINT32 lg_ulTouchMachineState = TOUCH_STATE_IDLE;
static POINT lg_stLastMovePoint;
ADCValueQueue  panADCXQueue;
ADCValueQueue  panADCYQueue;
UINT32 PANADC_X[MAXPANADCLEN];
UINT32 PANADC_Y[MAXPANADCLEN];

static INT32 Queue_ADCValue_Init(ADCValueQueue *pQueue, UINT32 *pValue, UINT32 quelen)
{
	INT32 ret=0;
	
	if((pQueue != 0) && (pValue != 0) && (quelen > 0))
	{
		pQueue->pValue = pValue;
		pQueue->quelen = quelen;
		pQueue->header = 0;
		pQueue->trail = 0;
		ret =1;
	}

	return ret;
}

static INT32 Queue_ADCValue_Length(ADCValueQueue *pQueue)
{
	INT32 queuelen=0;
	
	if(pQueue != 0)
	{
		if(pQueue->trail < pQueue->header)
		{
			queuelen = pQueue->quelen +(pQueue->trail- pQueue->header);
		}
		else
			queuelen =  pQueue->trail- pQueue->header;
	}

	return queuelen;
}

static INT32 Queue_ADCValue_Add(ADCValueQueue *pQueue,unsigned int value)
{
	INT32 ret=0;
	UINT32 *pValue=0;
	
	if(pQueue != 0)
	{
		pValue = pQueue->pValue + pQueue->trail;
		*pValue = value;

		pQueue->trail++;
		if(pQueue->trail >= pQueue->quelen)
		{
			pQueue->trail = 0;
		}

		if(pQueue->trail == pQueue->header)
		{
			pQueue->header++;
			if(pQueue->header >= pQueue->quelen)
			{
				pQueue->header = 1;
				pQueue->trail=0;
			}
		}

		ret=1;
	}

	return ret;
}

static INT32 Queue_ADCValue_Read(ADCValueQueue *pQueue, UINT32 *pValue)
{
	INT32 ret=0;
	UINT32 *pHeaderValue=0;

	if((pQueue != 0) && (pValue != 0))
	{
		if(Queue_ADCValue_Length(pQueue) > 0)
		{
			pHeaderValue = pQueue->pValue + pQueue->header;
			*pValue = *pHeaderValue;

			pQueue->header++;
			if(pQueue->header >= pQueue->quelen)
			{
				pQueue->header = 0;
			}

			ret=1;
		}
	}

	return ret;
}

//////////////////////////////////////////////////
static UINT32 GetADCTouch(PAN_DATA *pPan)
{
	UINT32 panX;
	UINT32 panY;

	INT32 ret=0;

	if(pPan != 0)
	{
		ret = Queue_ADCValue_Read(&panADCXQueue,&panX);
		if(ret==1)
		{
			ret = Queue_ADCValue_Read(&panADCYQueue,&panY);
			if(ret == 1)
			{
				pPan->adcX = panX;
				pPan->adcY = panY;
			}
		}
	}

	return ret;
}

static void CalcCoord(PAN_DATA *pPan, POINT *pPT);

#define TOUCH_DOT_REGION	10

extern void SendTouchInputEventFromISR(lv_indev_data_t *indata);

/***********************************************************************************************
通过大量试验发现触摸屏存在的问题有
	1: 触摸刚开始的几个点可能任意哪个点或者哪几个点会有较大偏差;
	2: 释放前的采样点很大几率不准确。
	
为了应对各种错误情况，采取的措施有
	1: 首先简单地丢弃掉最后一个采样点
	2: 提高采样频率，保证最开始的几个采样点都在一个比较小的范围
	    内(即使快速滑动，刚开始不会有很大速度，因此也能保证起始点
	    比较聚集)。在这种前提下提取开始采样时的若干个点进行分析,
	    找出坐标值比较接近的点数最多的点，其他点则认为是错误点丢
	    弃掉，如果找不到的话则认为所有点都不稳定，重新再提取之后
	    的若干个点进行分析。
*************************************************************************************************/
#define SCOPE_ADJUST			8

#define ANALYSIS_POINT_SUCCESS	3
#define ANALYSIS_POINT_COUNT	5
#define DISCARD_POINT_COUNT		2
#define POINT_POOL_SIZE			(ANALYSIS_POINT_COUNT+DISCARD_POINT_COUNT)

#define MYABS(x)	(((x)>=0) ? (x) : (-(x)))
/*
*********************************************************************************************************
* Description: 分析采样最开始的若干点，丢弃错误点
*
* Arguments  : point[] : 采样点坐标值数组
			   num	   : 用于分析的采样点个数		
*
* Returns    : 有效点的个数，返回0表示所有点都无效

* Notes      : 剩余的有效点重新从原数组起始位置开始依次放置
*********************************************************************************************************
*/
static int Touch_Start_Analyse(POINT point[], int num)
{
	int i, j;
	int key[ANALYSIS_POINT_COUNT] = {0};
	int near_by_point[ANALYSIS_POINT_COUNT] = {0};
	int effective_near_by_points[ANALYSIS_POINT_COUNT] = {0};
	int max = -1;
	int samplecnt = 0;
	
	if(num > ANALYSIS_POINT_COUNT)
		num = ANALYSIS_POINT_COUNT;

	for(i=0; i<num; i++)
	{
		//计算每个点和其接近的点的个数并记录下距离最近的点
		near_by_point[i] = 1;
		for(j=0; j<num; j++)
		{
			if(j == i)
				continue;
			
			if(MYABS(point[i].x - point[j].x)<SCOPE_ADJUST && MYABS(point[i].y - point[j].y)<SCOPE_ADJUST)
			{
				key[i]++;
				near_by_point[j] = 1;
			}
			else
				near_by_point[j] = 0;
		}
		
		//筛选出相近点数最多的点并记录下与其距离最近的点位置
		if(key[i] > max)
		{
			max = key[i];
			for(j=0;j<num;j++)
			{
				effective_near_by_points[j] = near_by_point[j];
			}
		}
	}

	//有效点个数不够，判定所有点都不稳定全部丢弃
	if(max < ANALYSIS_POINT_SUCCESS-1)
		return 0;

	//移除所有无效点，有效点从原数组位置0开始依次放置
	for(i=0; i<num; i++)
	{
		if(effective_near_by_points[i])
		{
			point[samplecnt] = point[i];
			samplecnt++;
		}
	}

	return samplecnt;
}

static void Handler_Touch_IntEvent(UINT32 ulEvent, UINT32 lpParam, UINT32 wParam)
{
	static int s_SampleCnt;
	static int s_PoolIndex;
	static POINT s_PointPool[POINT_POOL_SIZE];
		
	switch(lg_ulTouchMachineState)
	{
		case TOUCH_STATE_IDLE:
			if(ulEvent == TOUCH_START_EVENT)
			{
				lg_ulTouchMachineState = TOUCH_STATE_START;
				s_SampleCnt = 0;
				s_PoolIndex = 0;
			}
			break;
			
		case TOUCH_STATE_START:
			if(ulEvent == TOUCH_STOP_EVENT)
			{
				lg_ulTouchMachineState = TOUCH_STATE_IDLE;
			}
			else if(ulEvent == TOUCH_SAMPLE_EVENT)
			{
				POINT pt;
				PAN_DATA stPanData;
				lv_indev_data_t indata = {0};
				int i;
				
				//get cordinate	
				stPanData.adcX = lpParam;
				stPanData.adcY = wParam;	
				CalcCoord(&stPanData, &pt);	
				if(pt.x < 0 || pt.y < 0)
					return;

				if(pt.x < 0 || pt.x >= LCD_WIDTH || pt.y < 0 || pt.y >= LCD_HEIGHT)
					return;
					
				//防止ANALYSIS_POINT_START个点内包含最后要丢弃的点，刚开始要暂存
				//ANALYSIS_POINT_START + DISCARD_POINT_END个点
				if(s_SampleCnt < ANALYSIS_POINT_COUNT + DISCARD_POINT_COUNT)
				{
					s_PointPool[s_SampleCnt++] = pt;
					if(s_SampleCnt < ANALYSIS_POINT_COUNT + DISCARD_POINT_COUNT) 
						return;
				} 
				
				s_SampleCnt = Touch_Start_Analyse(s_PointPool, ANALYSIS_POINT_COUNT);	
				if(s_SampleCnt == 0)
				{
					for(i=0; i<DISCARD_POINT_COUNT; i++)
						s_PointPool[i] = s_PointPool[ANALYSIS_POINT_COUNT+i];
					s_SampleCnt = DISCARD_POINT_COUNT;
					return;
				}
				//send press event
				indata.point.x = s_PointPool[0].x;
				indata.point.y = s_PointPool[0].y;
				indata.state = LV_INDEV_STATE_PR;
				SendTouchInputEventFromISR(&indata);

				lg_ulTouchMachineState = TOUCH_STATE_SAMPLE;

				//将为满足最后丢弃任务而暂存的点复制到分析后剩余点之后
				for(i=0; i<DISCARD_POINT_COUNT; i++)
					s_PointPool[s_SampleCnt + i] = s_PointPool[ANALYSIS_POINT_COUNT+i];
				s_PoolIndex = 1;
				//计算此时剩余的未操作的有效点数
				s_SampleCnt = s_SampleCnt - 1 + DISCARD_POINT_COUNT;
				//send move event in the pool
				while(s_SampleCnt > DISCARD_POINT_COUNT)
				{
					int xdiff, ydiff;
					unsigned int totaldiff;
					
					xdiff = s_PointPool[s_PoolIndex].x - lg_stLastMovePoint.x;
					ydiff = s_PointPool[s_PoolIndex].y - lg_stLastMovePoint.y;
					totaldiff = xdiff * xdiff + ydiff * ydiff;
					if(totaldiff > 4)
					{
						indata.point.x = s_PointPool[s_PoolIndex].x;
						indata.point.y = s_PointPool[s_PoolIndex].y;
						indata.state = LV_INDEV_STATE_PR;
						SendTouchInputEventFromISR(&indata);
						lg_stLastMovePoint.x = s_PointPool[s_PoolIndex].x;
						lg_stLastMovePoint.y = s_PointPool[s_PoolIndex].y; 				
					}
					s_PoolIndex++;
					s_SampleCnt--;
				}
			}
			break;
			
		case TOUCH_STATE_SAMPLE:
			if(ulEvent == TOUCH_SAMPLE_EVENT)
			{
				//caculate move center cordinate
				//if move, then send move event		
				PAN_DATA stPanData;
				POINT pt;
				int xDiff = 0, yDiff = 0;
				unsigned int totalDiff = 0;
				
				//get cordinate					
				stPanData.adcX = lpParam;
				stPanData.adcY = wParam;	
				CalcCoord(&stPanData, &pt);
				if(pt.x < 0 || pt.y < 0)
					return;

				if(pt.x < 0 || pt.x >= LCD_WIDTH || pt.y < 0 || pt.y >= LCD_HEIGHT)
					return;
					
				if(s_SampleCnt < DISCARD_POINT_COUNT)
				{	
					int index = (s_PoolIndex+s_SampleCnt++) % POINT_POOL_SIZE;
					
					s_PointPool[index] = pt;
					return;
				}
				
				xDiff = s_PointPool[s_PoolIndex].x - lg_stLastMovePoint.x;
				yDiff = s_PointPool[s_PoolIndex].y - lg_stLastMovePoint.y;
				totalDiff = xDiff * xDiff + yDiff * yDiff;
				if(totalDiff > 4)
				{
					lv_indev_data_t indata = {0};
					
					indata.point.x = s_PointPool[s_PoolIndex].x;
					indata.point.y = s_PointPool[s_PoolIndex].y;
					indata.state = LV_INDEV_STATE_PR;
					SendTouchInputEventFromISR(&indata);
					lg_stLastMovePoint.x = s_PointPool[s_PoolIndex].x;
					lg_stLastMovePoint.y = s_PointPool[s_PoolIndex].y; 				
				}
				s_PointPool[(s_PoolIndex+DISCARD_POINT_COUNT)%POINT_POOL_SIZE] = pt;
				s_PoolIndex = (s_PoolIndex+1)%POINT_POOL_SIZE;
			}
			else if(ulEvent == TOUCH_STOP_EVENT)
			{
				lv_indev_data_t indata = {0};
				
				//send release event
				indata.point.x = lg_stLastMovePoint.x;
				indata.point.y = lg_stLastMovePoint.y;
				indata.state = LV_INDEV_STATE_REL;
				SendTouchInputEventFromISR(&indata);
				lg_ulTouchMachineState = TOUCH_STATE_IDLE;
			}
			break;
			
		case TOUCH_STATE_STOP:
			break;
	}
}


static volatile UINT32 lg_bTouchStart = 0;
static UINT32 CheckTouchStart()
{
	return lg_bTouchStart == 1;
}

static UINT32 CheckTouchStop()
{
	return lg_bTouchStart == 0;
}

static UINT32 GetTouchSampleCounter()
{
	return Queue_ADCValue_Length(&panADCXQueue);
}

static UINT32 lg_bTouchAdjusted = 0;
void TouchEventHandler(UINT32 ulEvent, UINT32 lpParam, UINT32 wParam)
{
	if(lg_bTouchAdjusted)
	{
		Handler_Touch_IntEvent(ulEvent, lpParam, wParam);	
	}
	else
	{

		if(ulEvent ==  TOUCH_START_EVENT)
		{
			lg_bTouchStart = 1;
			Queue_ADCValue_Init(&panADCXQueue, PANADC_X, MAXPANADCLEN);
			Queue_ADCValue_Init(&panADCYQueue, PANADC_Y, MAXPANADCLEN);
		}
		else if(ulEvent ==  TOUCH_SAMPLE_EVENT)
		{
			Queue_ADCValue_Add(&panADCXQueue,lpParam);
			Queue_ADCValue_Add(&panADCYQueue,wParam);	
		}
		else if(ulEvent ==  TOUCH_STOP_EVENT)
		{
			lg_bTouchStart = 0;
		}	
	}
}

/***************************************************************************************
	触摸屏坐标校验代码
***************************************************************************************/


#define GRID_RANGE  12
#define HIT_RANGE   2
#define ABS_HIT     ((GRID_RANGE+HIT_RANGE)*2)

#define TOUCH_HIGH_EXACTION 0
#define TOUCH_MID_EXACTION 0
#define TOUCH_LOW_EXACTION 1


#define TOUCH_REVISE_DEBUG		1

#if TOUCH_REVISE_DEBUG
	#define TOUCH_DEBUG_MSG(fmt, args...)	printf(fmt, ##args)
#else
	#define TOUCH_DEBUG_MSG(fmt, args...)
#endif



#define  SYSTEM_ERROR 100.0

#define  SCALE_AERROR 2// 3 这里考虑触摸屏的差异修改20130618
#define  SCALE_DERROR 5
static double abs_ax,abs_dx,abs_ly,abs_ry;
static double abs_lx,abs_rx;
static double abs_err1,abs_err2,abs_err3,abs_err4,abs_err5;
static double abs_cx,abs_cy;
static int lg_direction = 0;
 
#if  TOUCH_HIGH_EXACTION
	#define SYSTEM_ERROR1 100.0
#elif  TOUCH_MID_EXACTION
	#define SYSTEM_ERROR1 150.0
#elif  TOUCH_LOW_EXACTION
	#define SYSTEM_ERROR1 200.0
#else
	#define SYSTEM_ERROR1 150.0
#endif

/* static void InitializeCalibration(int a0, int a1, int a2, int a3, int a4, int a5, int a6)
{
	cal.a[0] = a0;
	cal.a[1] = a1;
	cal.a[2] = a2;
	cal.a[3] = a3;
	cal.a[4] = a4;
	cal.a[5] = a5;
	cal.a[6] = a6;
} */

static void SaveCalibration(calibration *pCalibration)
{

}

static int perform_calibration(calibration *cal) {
	int j;
	float n, x, y, x2, y2, xy, z, zx, zy;
	float det, a, b, c, e, f, i;
	float scaling = 65536.0;

// Get sums for matrix
	n = x = y = x2 = y2 = xy = 0;
	for(j=0;j<5;j++) {
		n += 1.0;
		x += (float)cal->x[j];
		y += (float)cal->y[j];
		x2 += (float)(cal->x[j]*cal->x[j]);
		y2 += (float)(cal->y[j]*cal->y[j]);
		xy += (float)(cal->x[j]*cal->y[j]);
	}

// Get determinant of matrix -- check if determinant is too small
	det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
	if(det < 0.1 && det > -0.1) {
		printf("ts_calibrate: determinant is too small -- %f\n",det);
		return 0;
	}

// Get elements of inverse matrix
	a = (x2*y2 - xy*xy)/det;
	b = (xy*y - x*y2)/det;
	c = (x*xy - y*x2)/det;
	e = (n*y2 - y*y)/det;
	f = (x*y - n*xy)/det;
	i = (n*x2 - x*x)/det;

// Get sums for x calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->xfb[j];
		zx += (float)(cal->xfb[j]*cal->x[j]);
		zy += (float)(cal->xfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer x coord
	cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

// Get sums for y calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->yfb[j];
		zx += (float)(cal->yfb[j]*cal->x[j]);
		zy += (float)(cal->yfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer y coord
	cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

// If we got here, we're OK, so assign scaling to a[6] and return
	cal->a[6] = (int)scaling;

	return 1;
}

void CleanTouchAdjustParameter(void)
{
	Queue_ADCValue_Init(&panADCXQueue, PANADC_X, MAXPANADCLEN);
	Queue_ADCValue_Init(&panADCYQueue, PANADC_Y, MAXPANADCLEN);
	lg_bTouchAdjusted = 0;
}

void TouchInit(void)
{
	CleanTouchAdjustParameter();
}

static void CalcCoord(PAN_DATA *pPan, POINT *pPT)
{ 
	pPT->x = (pPan->adcX * cal.a[1] + pPan->adcY * cal.a[2] + cal.a[0])/cal.a[6];
	pPT->y = (pPan->adcX * cal.a[4] + pPan->adcY * cal.a[5] + cal.a[3])/cal.a[6];
}

static unsigned int GetTouchHit(PAN_DATA *pPan)
{
	unsigned int PanReq;
	PAN_DATA Pan[50];
	unsigned int i, Count;

	for(i = 0; i < 50; i++)
	{
		Pan[i].adcX = 0xFFFF;
		Pan[i].adcY = 0xFFFF;
	}

ReStartCheck:
	while(!CheckTouchStart());
	while(!CheckTouchStop());
	if(GetTouchSampleCounter() < 16)
		goto ReStartCheck;

	AIC_DisableIT(ADC_IRQn);
	while(1)
	{		
		PanReq = GetADCTouch(&Pan[49]);

		if(PanReq==0)
		{
			break;
		}
		for(i = 0; i < 49; i++)
		{
			Pan[i].adcX = Pan[i+1].adcX;
			Pan[i].adcY = Pan[i+1].adcY;
		}
	}
	AIC_EnableIT(ADC_IRQn);
	
	Count = 0;

	for(i = 0; i < 49; i++)
	{
		if(Pan[i].adcY < 0xFFF)
		{
			Count++;
		}
	}

	pPan->adcX = Pan[49-Count/2].adcX;
	pPan->adcY = Pan[49-Count/2].adcY;
	

	TOUCH_DEBUG_MSG("Hit Count = %d\n", Count);
	TOUCH_DEBUG_MSG("pPan->adcX=%d\n",pPan->adcX);
	TOUCH_DEBUG_MSG("pPan->adcY=%d\n",pPan->adcY);


	return TRUE;
}

static void ClearBitmap(lv_color_t color, const lv_area_t * area, lv_color_t * color_p)
{
    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > LCD_WIDTH - 1 ? LCD_WIDTH - 1 : area->x2;
    int32_t act_y2 = area->y2 > LCD_HEIGHT - 1 ? LCD_HEIGHT - 1 : area->y2;

    /*32 or 24 bit per pixel*/
    if(LCD_BPP == 32) {
        uint32_t * fbp32 = (uint32_t *)color_p;
        int32_t x, y;
        for(y = act_y1; y <= act_y2; y++) {
            fbp32 = (uint32_t *)color_p + y * LCD_WIDTH;
			for (x = act_x1; x <= act_x2; x++)
            	fbp32[x] = color.full;
        }
    }
    /*16 bit per pixel*/
    else if(LCD_BPP == 16) {
        uint16_t * fbp16 = (uint16_t *)color_p;
        int32_t x, y;
        for(y = act_y1; y <= act_y2; y++) {
            fbp16 = (uint16_t *)color_p + y * LCD_WIDTH;
			for (x = act_x1; x <= act_x2; x++)
            	fbp16[x] = color.full;
        }
    }	
}

static void PutHitCursor(int x, int y)
{
	lv_area_t area;
	area.x1 = x - GRID_RANGE;
	area.y1 = y;
	area.x2 = x + GRID_RANGE + HIT_RANGE - 1;
	area.y2 = y + HIT_RANGE - 1;
	ClearBitmap(LV_COLOR_WHITE, &area, (lv_color_t*)ark_lcd_get_virt_addr());
	area.x1 = x;
	area.y1 = y - GRID_RANGE;
	area.x2 = x + HIT_RANGE - 1;
	area.y2 = y + GRID_RANGE + HIT_RANGE - 1;
	ClearBitmap(LV_COLOR_WHITE, &area, (lv_color_t*)ark_lcd_get_virt_addr());
	area.x1 = x;
	area.y1 = y;
	area.x2 = x + HIT_RANGE - 1;
	area.y2 = y + HIT_RANGE - 1;	
	ClearBitmap(LV_COLOR_BLACK, &area, (lv_color_t*)ark_lcd_get_virt_addr());
	CP15_clean_dcache_for_dma((uint32_t)(lv_color_t*)ark_lcd_get_virt_addr(), 
		(uint32_t)(lv_color_t*)ark_lcd_get_virt_addr() + FB_SIZE);
}

static void ClrHitCursor(int x, int y)
{
	lv_area_t area;
	area.x1 = x - GRID_RANGE;
	area.y1 = y - GRID_RANGE;
	area.x2 = x + GRID_RANGE + HIT_RANGE - 1;
	area.y2 = y + GRID_RANGE + HIT_RANGE - 1;
	ClearBitmap(LV_COLOR_BLACK, &area, (lv_color_t*)ark_lcd_get_virt_addr());
	CP15_clean_dcache_for_dma((uint32_t)(lv_color_t*)ark_lcd_get_virt_addr(), 
		(uint32_t)(lv_color_t*)ark_lcd_get_virt_addr() + FB_SIZE);
}

static int compare_data(calibration *cal)
{
	int ret=0;
	int diff_x;
	int diff_y;

	diff_x = (int)fabs(cal->x[0] - cal->x[1]);
	diff_y = (int)fabs(cal->y[0] - cal->y[1]);

	if(diff_x > diff_y)
		lg_direction = 0;
	else
		lg_direction = 1;

	TOUCH_DEBUG_MSG("lg_direction = %d\r\n", lg_direction);

	if(lg_direction == 0)
	{
		if((cal->x [0]<cal->x [1])&&(cal->x [3]<cal->x [2]))
			ret=1;
		if((cal->x [0]>cal->x [1])&&(cal->x [3]>cal->x [2]))
			ret=1;
	}
	else
	{
		if((cal->y[0]<cal->y[1])&&(cal->y[3]<cal->y[2]))
			ret=1;
		if((cal->y[0]>cal->y[1])&&(cal->y[3]>cal->y[2]))
			ret=1;
	}
	
	return ret;
}

static int square_judge(calibration *cal)
{
	int ret=0;

	if(lg_direction == 0)
	{
		abs_ax=fabs(cal->x [0] - cal->x [1]);
		abs_dx=fabs(cal->x [2]-cal->x [3]);
		abs_err1=fabs(abs_ax -abs_dx);
		TOUCH_DEBUG_MSG("***abs_ax=%d, abs_dx=%d, abs_err1=%d, SYSTEM_ERROR1=%d*******\n", (int)abs_ax, (int)abs_dx, (int)abs_err1, (int)SYSTEM_ERROR1);


		abs_lx=fabs(cal->x [0]-cal->x [4]);
		abs_rx=fabs(cal->x[3]-cal->x [4]);
		abs_err2=fabs(abs_lx -abs_rx);
		TOUCH_DEBUG_MSG("***abs_lx=%d,abs_rx=%d,abs_err2=%d****2\n", (int)abs_lx, (int)abs_rx, (int)abs_err2);


		abs_ly=fabs(cal->y [0]-cal->y [3]);
		abs_ry=fabs(cal->y [1]-cal->y [2]);
		abs_err3=fabs(abs_ly -abs_ry);
		TOUCH_DEBUG_MSG("***abs_ly=%d, abs_ry=%d, abs_err3=%d****2\n", (int)abs_ly, (int)abs_ry, (int)abs_err3);
		

		if(abs_err1<SYSTEM_ERROR1&&abs_err2<SYSTEM_ERROR1&&abs_err3<SYSTEM_ERROR1)
			ret=1;
	}
	else
	{
		abs_ax=fabs(cal->y[0] - cal->y[1]);
		abs_dx=fabs(cal->y[2]-cal->y[3]);
		abs_err1=fabs(abs_ax -abs_dx);
		TOUCH_DEBUG_MSG("***abs_ax=%d, abs_dx=%d, abs_err1=%d, SYSTEM_ERROR1=%d*******\n", (int)abs_ax, (int)abs_dx, (int)abs_err1, (int)SYSTEM_ERROR1);


		abs_lx=fabs(cal->y[0]-cal->y[4]);
		abs_rx=fabs(cal->y[3]-cal->y[4]);
		abs_err2=fabs(abs_lx -abs_rx);
		TOUCH_DEBUG_MSG("***abs_lx=%d, abs_rx=%d, abs_err2=%d****2\n", (int)abs_lx, (int)abs_rx, (int)abs_err2);


		abs_ly=fabs(cal->x[0]-cal->x[3]);
		abs_ry=fabs(cal->x[1]-cal->x[2]);
		abs_err3=fabs(abs_ly -abs_ry);
		TOUCH_DEBUG_MSG("***abs_ly=%d, abs_ry=%d, abs_err3=%d****2\n", (int)abs_ly, (int)abs_ry, (int)abs_err3);


		if(abs_err1<SYSTEM_ERROR1&&abs_err2<SYSTEM_ERROR1&&abs_err3<SYSTEM_ERROR1)
			ret=1;
	}

	TOUCH_DEBUG_MSG("***ret=%d****4\n",ret);

	return ret;
}

static int judge_center(calibration *cal)
{
	int ret=0;

	if(lg_direction == 0)
	{
		abs_cx=fabs(cal->x [4]-cal->x [0])*2;
		abs_cy=fabs(cal->y [4]-cal->y [0])*2;
		abs_err4=fabs(abs_cx -abs_ax);
		abs_err5=fabs(abs_cy -abs_ly);
		TOUCH_DEBUG_MSG("***abs_cx=%d, abs_cy=%d, abs_err4=%d, abs_err5=%d****2\n", (int)abs_cx, (int)abs_cy, (int)abs_err4, (int)abs_err5);
		
		if(abs_err4<SYSTEM_ERROR1&&abs_err5<SYSTEM_ERROR1)
			 ret=1;
	}
	else
	{
		abs_cx=fabs(cal->y[4]-cal->y[0])*2;
		abs_cy=fabs(cal->x[4]-cal->x[0])*2;
		abs_err4=fabs(abs_cx -abs_ax);
		abs_err5=fabs(abs_cy -abs_ly);
		TOUCH_DEBUG_MSG("***abs_cx=%d, abs_cy=%d, abs_err4=%d, abs_err5=%d****2\n", (int)abs_cx, (int)abs_cy, (int)abs_err4, (int)abs_err5);
		
		if(abs_err4<SYSTEM_ERROR1&&abs_err5<SYSTEM_ERROR1)
			 ret=1;
	}
	
	return ret;
}

static int filter_data(calibration *cal)
{
	int ret=0;
	float abs_lcd;
	int scale;
	int i;
	
	TOUCH_DEBUG_MSG("***filter_data****1\n");
	for(i=0;i<5;i++)
	{
		TOUCH_DEBUG_MSG("cal->x[%d] = %d, cal->y[%d] = %d\r\n", i, (int)cal->x[i], i, (int)cal->y[i]);
	}
	
	if(compare_data( cal))
	{
		TOUCH_DEBUG_MSG("*** Pass compare_data ****\n");
		
		if(square_judge(cal))
		{
			TOUCH_DEBUG_MSG("*** Pass square_judge ****\n");
			
			abs_lcd=fabs(cal->xfb[1] -cal->xfb[0]);
			TOUCH_DEBUG_MSG("***abs_ax=%d, abs_lcd=%d****\n", (int)abs_ax, (int)abs_lcd);
			scale=(int)(abs_ax/abs_lcd);
			TOUCH_DEBUG_MSG("***scale=%d****4\n",scale);
			if(SCALE_AERROR<scale&&SCALE_DERROR>scale)
			{	
				TOUCH_DEBUG_MSG("*** Scale check ok ****\n");
				if(judge_center(cal))
				{
					ret=1;
				}
				else 
				{
					TOUCH_DEBUG_MSG("*** Failed at judge_center****\n");
				}
					
				 
			}
		}
	}
	TOUCH_DEBUG_MSG("*** filter_data ret=%d****\n",ret);
	
	return ret;
}

static void get_sample (calibration *cal,
			int index, int x, int y, char *name)
{
	PAN_DATA ts_cord;

	PutHitCursor(x, y);
//	getxy (ts, &cal->x [index], &cal->y [index]);
	GetTouchHit(&ts_cord);
	cal->x [index] = ts_cord.adcX;
	cal->y [index] = ts_cord.adcY;
	
	ClrHitCursor(x, y);

	cal->xfb [index] = x;
	cal->yfb [index] = y;

	TOUCH_DEBUG_MSG("get_sample %s : X = %4d Y = %4d\n", name, cal->x [index], cal->y [index]);
}

unsigned int AdjustTouch(void)
{
	unsigned int i;


#if 0
	 cal.a[0]=56822272;
	 cal.a[1]=-14948;
	 cal.a[2]=36;
	 cal.a[3]=38820672;
	 cal.a[4]=-59;
	 cal.a[5]=-11704;
	 cal.a[6]=65536;

	 lg_bTouchAdjusted = 1;
 #else
	while(1)
	{
		get_sample (&cal, 0, 50, 50, "Top left");
		get_sample (&cal, 1, LCD_WIDTH - 50, 50, "Top right");
		get_sample (&cal, 2, LCD_WIDTH - 50, LCD_HEIGHT - 50, "Bot right");
		get_sample (&cal, 3, 50, LCD_HEIGHT - 50, "Bot left");
		get_sample (&cal, 4, LCD_WIDTH / 2, LCD_HEIGHT / 2, "Center");

		if(filter_data(&cal))
		{
			perform_calibration (&cal);
			for(i=0;i<7;i++)
			{
				printf("the result cal->a[%d]=%d\n", i, cal.a[i]);
			}
			SaveCalibration(&cal);
			
			break;     
		}		
	}
#endif

	lg_bTouchAdjusted = 1;

	return 0;	// 5次均无效, 取默认值
}

int LoadTouchConfigure(void)
{
	return -1;
}

#endif
