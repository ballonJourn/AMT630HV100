	
// ??Hmi??
#if XM_HMI_HOST

#include "xm_hmi_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include <VG/vgext.h> 
#include "rtos.h"
#include "xm_event.h"
#include "xm_user.h"
#include "vg_lcdc.h"
#include <tkc/fs.h>
#include <tkc/platform.h>
#include "tkc/mem.h"
#include "board.h"
#include "pxp.h"
#include "lcd.h"



#define CLASS_NAME 	"XM_VG"
#define WINDOW_TITLE "XM_VG"

#define	EnterCriticalSection		OS_Use
#define	LeaveCriticalSection		OS_Unuse
#define	DeleteCriticalSection	OS_DeleteRSema
#define	InitializeCriticalSection	OS_CREATERSEMA
#define	CRITICAL_SECTION	OS_RSEMA

extern int OS_WaitCSemaTimed (OS_CSEMA* pCSema, int TimeOut);
extern unsigned long XM_GetTickCount	(void);
extern  void  vgScanVideoMemoryUsage (
	VGuint* TotalFreeBytes,		/* VideoMemory????????? */
	VGuint* MaximumAllocateBytes /* VideoMemory??????????? */
);

static void init_pc_autotest(void);	// ?????????????

static CRITICAL_SECTION event_critical_section;
static OS_CSEMA event_csema;

#define	MAX_BUFFER_SIZE		0x3F

static XM_EVENT				eventBuffer[MAX_BUFFER_SIZE+1];		// ????
static volatile	int	   eventBPos;			// ??????
static volatile	int		eventEPos;			// ??????

// ????
static unsigned int			win_width;
static unsigned int			win_height;

static int tp_clicked = 0;


static float dpi_ratio = 1.0;

// ?????????????
// ?????
//		1		?????????????
//		0		?????????????
int XM_TpEventProc (unsigned int tp_event, unsigned int xPos, unsigned int yPos, unsigned int ticket)
{
	int ret;
	if(win_width == 0 || win_height == 0)
		return 0;
	
	EnterCriticalSection (&event_critical_section);
	if(ticket == 0)
		ticket = XM_GetTickCount();
	do {
		// ??????????
		if( ((eventEPos + 1) & MAX_BUFFER_SIZE) == (eventBPos & MAX_BUFFER_SIZE) )
		{
			ret = 0;
		}
		else
		{
			XM_EVENT *event;	
			if (tp_event == XM_EVENT_TOUCHDOWN)
			{
				if (tp_clicked)
				{
					// TOUCHUP????, ?????
					break;
				}
				tp_clicked = 1;
			}
			else if(tp_event == XM_EVENT_TOUCHUP)
			{
				// ?????TOUCHUP
				if(tp_clicked == 0)
					break;
				tp_clicked = 0;
			}
			// ??????????????
			event = eventBuffer + (eventEPos & MAX_BUFFER_SIZE);
			memset (event, 0, sizeof(XM_TouchEvent));
			event->type = tp_event;
			event->tp.timestamp = ticket;
			event->tp.type = tp_event;
			event->tp.x = xPos;
			event->tp.y = yPos;
			
			eventEPos ++;
			
			// ?????
			OS_SignalCSema (&event_csema);
			
			ret = 1;
		}	
	} while (0);
	
	LeaveCriticalSection (&event_critical_section);
	return ret;
}

#ifdef NINE
// ??NINE?????????
// nine_event     nine??
// ticket         ?????
// ?????
//		1		?????????????
//		0		?????????????
int XM_NineEventProc(nine_event_t *nine_event, unsigned int ticket)
{
  int ret;
  EnterCriticalSection(&event_critical_section);
  // ??????????
  if (((eventEPos + 1) & MAX_BUFFER_SIZE) == (eventBPos & MAX_BUFFER_SIZE))
  {
    ret = 0;
  }
  else
  {
    XM_EVENT *event;
    // ??????????????
    event = eventBuffer + (eventEPos & MAX_BUFFER_SIZE);
    memset(event, 0, sizeof(XM_NineEvent));
    event->type = XM_EVENT_NINE;
    event->nine.type = XM_EVENT_NINE;
    event->nine.timestamp = ticket;
    memcpy(&event->nine.event, nine_event, sizeof(nine_event_t));

    eventEPos++;

    // ?????
    OS_SignalCSema (&event_csema);

    ret = 1;
  }

  LeaveCriticalSection(&event_critical_section);
  return ret;
}
#endif

// ??????
int XM_WaitEvent (XM_EVENT *event, unsigned int timeout)
{
	int ret = 0;
	if(OS_WaitCSemaTimed (&event_csema, timeout) == 0)
	{
		EnterCriticalSection (&event_critical_section);
		if( eventBPos != eventEPos )
		{
			memcpy (event, eventBuffer + (eventBPos & MAX_BUFFER_SIZE), sizeof(XM_EVENT));
			eventBPos ++;
			ret = 1;
		}
		LeaveCriticalSection (&event_critical_section);
	}
	return ret;
}




float XM_GetWindowDpiRatio(void)
{
	dpi_ratio = 1.0f;
	return dpi_ratio;
}


static void windowDestroy(void) 
{

}

void XM_HmiHost_WindowDestroy(void) 
{
	DeleteCriticalSection (&event_critical_section);
	OS_DeleteCSema (&event_csema);
	windowDestroy ();
}



void * XM_HmiHost_WindowCreate(const char* title,
                              const unsigned int width,
                              const unsigned int height) 
{
	OS_CreateCSema (&event_csema, 0);
	InitializeCriticalSection (&event_critical_section); 
	eventBPos = eventEPos = 0;
	win_width = width;
	win_height = height;
	tp_clicked = 0;
  
  init_pc_autotest ();
	return (void *)1;
}

//extern uint64_t get_time_ms64(void);

void XM_HmiHost_WindowBuffersSwap(char *surface_pixels, unsigned int width, unsigned int height, unsigned int bpp)
{
	void xm_vg_release_gpu_fb (void);
	//unsigned int t1 = (unsigned int) get_time_ms64();
	unsigned int lcd_buffer = (unsigned int)xm_vg_require_gpu_fb();

	//lcd_buffer += xm_vg_get_osd_stride() * (xm_vg_get_height() - 1);
	bpp = xm_vg_get_bpp ();
	// 630H ???ARGB8888?RGB565??
#ifdef ENABLE_FB_DIRTY_RECTS_COPY
	extern int vg_surface_sync(unsigned int lcd_buffer, unsigned int width, unsigned int height);
	// ??????????????LCD??FB
	if (vg_surface_sync(lcd_buffer, width, height))
	{
		// ????
	}
	else
#endif
	{
#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
		unsigned int lcd_tmp_buf = ark_lcd_get_virt_addr();
		int src_width = xm_vg_get_width();
		int src_height =  xm_vg_get_height();
		int dst_width = src_width;
		int dst_height = src_height;
		int src_format, dst_format, lcd_format;
		static int lcd_first_show = 0;
		int ret;

#if		((LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_90) || (LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_270))
		dst_width = src_height;
		dst_height = src_width;
#endif
		if (bpp == 32)
		{
			src_format = PXP_SRC_FMT_RGB888;
			dst_format = PXP_OUT_FMT_ARGB8888;
			lcd_format = LCD_OSD_FORAMT_ARGB888;
			vgReadPixels((void *)lcd_tmp_buf, xm_vg_get_osd_stride(), VG_sARGB_8888, 0, 0, src_width, src_height);
		}
		else
		{
			src_format = PXP_SRC_FMT_RGB565;
			dst_format = PXP_OUT_FMT_RGB565;
			lcd_format = LCD_OSD_FORAMT_RGB565;
			vgReadPixels((void *)lcd_tmp_buf, xm_vg_get_osd_stride(), VG_sRGB_565, 0, 0, src_width, src_height);
		}
		ret = pxp_scaler_rotate(lcd_tmp_buf, 0, 0, src_format, src_width, src_height,
				 (uint32_t)lcd_buffer, 0, dst_format, dst_width, dst_height, LCD_ROTATE_ANGLE);
		if(ret < 0)
		{
			printf("%s pxp_scaler_rotate failed\n", __func__);
			//...
		}
		if(!lcd_first_show)
		{
			ark_lcd_set_osd_size(LCD_UI_LAYER, dst_width, dst_height);
			ark_lcd_set_osd_format(LCD_UI_LAYER, lcd_format);
			lcd_first_show = 1;
		}
#else
		if (bpp == 32)
		{
			vgReadPixels((void *)lcd_buffer, xm_vg_get_osd_stride(), VG_sARGB_8888, 0, 0, xm_vg_get_width(), xm_vg_get_height());
		}
		else
		{
			vgReadPixels((void *)lcd_buffer, xm_vg_get_osd_stride(), VG_sRGB_565, 0, 0, xm_vg_get_width(), xm_vg_get_height());
		}
#endif
	}

	//unsigned int t2 = (unsigned int) get_time_ms64();
	//unsigned int t3 = (unsigned int)get_time_ms64();
	//printf("ReadPixel = %d, rel = %d ms\r\n", t2 - t1, t3 - t2);
	xm_vg_release_gpu_fb ();	

	//VGuint TotalFreeBytes, MaximumAllocateBytes;
	//vgScanVideoMemoryUsage (&TotalFreeBytes, &MaximumAllocateBytes);
	//printf ("TotalFreeBytes=%d, MaximumAllocateBytes=%d\n",TotalFreeBytes, MaximumAllocateBytes);
}

void XM_GetWidowSize(int *w, int *h)
{
	*w = win_width;
	*h = win_height;
}

void XM_GetDrawableSize(int *w, int *h)
{

}

extern int nine_at_define(unsigned  char *script_data);

#define AT_FILE_NAME  "./autotest.at"
static int run_at_script(void)
{
	int32_t file_size;
	unsigned char *script = NULL;
	do {
		file_size = file_get_size (AT_FILE_NAME);
    	if(file_size < 0)
      		return -1;
	 	
		script = (unsigned char *)TKMEM_ZALLOCN(unsigned char, file_size + 1);
		if (script) {
			file_read_part (AT_FILE_NAME, script, file_size, 0);
			printf("run AT script file(%s) ...\n", AT_FILE_NAME);
			nine_at_define((unsigned char *)script);
			printf("run AT script file(%s) finish\n", AT_FILE_NAME);
			TKMEM_FREE(script);
		}

	} while (0);
	return 0;
}

static void pc_autotest_task(void)
{
  int fs_size = -1;
  while(1)
  {
    // ??SD????????
    int32_t size = file_get_size (AT_FILE_NAME);
    if(fs_size != size && size > 0)
    {
      fs_size = size;
      run_at_script (); 
    }
    else
    {
      fs_size = -1;
    }
    sleep_ms (1000);
    
  }
}

static OS_TASK pc_autotest_os_task;
static unsigned int pc_autotest_stack[0x4000 / 4];
static void init_pc_autotest(void)
{
#ifdef NINE
	OS_CREATETASK(
    &pc_autotest_os_task,
		"pc_autotest",
		pc_autotest_task,
		200,
		pc_autotest_stack    
   );
	//hAutotestThread = CreateThread(NULL, 0x10000, pc_autotest_task, NULL, 0, NULL);
#endif
}

#endif /* XM_HMI_HOST */


