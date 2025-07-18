#include "FreeRTOS.h"
#include "board.h"
#include "sysinfo.h"
#include "chip.h"

static const char* abort_status[][2]=
{
  // IFSR status        ,       DFSR status
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//0
  {"Unknown(reserved status)",                          "Alignment Fault"                               },//1
  {"Debug Event",                                       "Debug Event"                                   },//2
  {"Access flag - section",                             "Access flag - section"                         },//3
  {"Unknown(reserved status)",                          "Instruction cache maintenance"                 },//4
  {"Translation fault - section",                       "Translation fault - section"                   },//5
  {"Access flag - Page",                                "Access flag - Page"                            },//6
  {"Translation fault -Page",                           "Translation fault -Page"                       },//7
  {"Synchronous external abort",                        "Synchronous external abort, nontranslation"    },//8
  {"Domain fault - Section",                            "Domain fault - Section"                        },//9
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//10
  {"Domain fault - Page",                               "Domain fault - Page"                           },//11
  {"Synchronous external abort - L1 Translation",       "Synchronous external abort - L1 Translation"   },//12
  {"Permission fault - Section",                        "Permission fault - Section"                    },//13
  {"Synchronous external abort - L2 Translation",       "Synchronous external abort - L2 Translation"   },//14
  {"Permission fault - Page",                           "Permission fault - Page"                       },//15
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//16
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//17
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//18
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//19
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//20
  {"Unknown(reserved status)",                          "Unknown(reserved status)"                      },//21
  {"Unknown(reserved status)",                          "Asynchronous external abort"}

};

void LowLevelInit( void )
{
}

void Abort_C_Handler( void)
{
    uint32_t v1,v2, dfsr;
    v1= 0;
    v2= 0;
    asm("mrc   p15, 0, %0, c5, c0, 0" : : "r"(v1));
    asm("mrc   p15, 0, %0, c6, c0, 0" : : "r"(v2));

    dfsr = ((v1 >> 4) & 0x0F);
    printf("\n\r######################################################################\n\r");
    printf("Data Abort occured in %x domain\n\r", (unsigned int)dfsr);
    dfsr = (((v1 & 0x400) >> 6) | (v1 & 0x0F));
    printf("Data abort fault reason is: %s\n\r", (char*)abort_status[dfsr][1]);
    printf("Data fault occured at Address = 0x%08x\n\n\r",(unsigned int)v2);


    printf("-[Info]-Data fault status register value = 0x%x\n\r",(unsigned int)v1);

    while(1);

}

void Prefetch_C_Handler( void)
{
    uint32_t v1,v2, ifsr;
    v1= 0;
    v2= 0;

    asm("mrc   p15, 0, %0, c5, c0, 1" : : "r"(v1));
    asm("mrc   p15, 0, %0, c6, c0, 2" : : "r"(v2));

    ifsr = (((v1 & 0x400) >> 6) | (v1 & 0x0F));
    printf("\n\r######################################################################\n\r");
    printf("Instruction prefetch abort reason is: %s\n\r", (char*)abort_status[ifsr][0]);
    printf("Instruction prefetch Fault occured at Address = 0x%08x\n\n\r",(unsigned int)v2);

    printf("-[INFO]- Prefetch Fault status register value by = 0x%x\n\r",(unsigned int)v1);

    while(1);

}

void Undefined_C_Handler( void)
{
  printf("Undefined abort \n\r");
  while(1);
}

typedef enum {
	VBUF_STATUS_FREE = 0,
	VBUF_STATUS_USED,
	VBUF_STATUS_RENDERED,
} VIDEO_BUFFER_STATUS;

typedef struct {
	uint32_t addr;
	int status;
}VideoBufInfo;

static SemaphoreHandle_t vdisbuf_mutex = NULL;
static VideoBufInfo vdisbufs[VIDEO_DISPLAY_BUF_NUM] = {0};

void VideoDisplayBufInit(void)
{
	int i;

	vdisbuf_mutex = xSemaphoreCreateRecursiveMutex();

	for (i = 0; i < VIDEO_DISPLAY_BUF_NUM; i++) {
		vdisbufs[i].addr = (uint32_t)pvPortMalloc(VIDEO_DISPLAY_WIDTH * VIDEO_DISPLAY_HEIGHT * 2);
		vdisbufs[i].status = VBUF_STATUS_FREE;
	}
}

int xVideoDisplayBufTake(uint32_t xTicksToWait)
{
	return xSemaphoreTakeRecursive(vdisbuf_mutex, xTicksToWait);
}

void vVideoDisplayBufGive(void)
{
	xSemaphoreGiveRecursive(vdisbuf_mutex);
}

uint32_t ulVideoDisplayBufGet(void)
{
	uint32_t bufaddr = 0;
	int i;
	
	xSemaphoreTakeRecursive(vdisbuf_mutex, portMAX_DELAY);

	for (i = 0; i < VIDEO_DISPLAY_BUF_NUM; i++) {
		if (vdisbufs[i].status == VBUF_STATUS_FREE) {
			bufaddr = vdisbufs[i].addr;
			vdisbufs[i].status = VBUF_STATUS_USED;
			break;
		}
	}

	xSemaphoreGiveRecursive(vdisbuf_mutex);
	
	configASSERT(bufaddr != 0);

	return bufaddr;
}

uint32_t ulVideoDisplayBufGetSize(void)
{
	return VIDEO_DISPLAY_WIDTH * VIDEO_DISPLAY_HEIGHT * 2;
}

void vVideoDisplayBufRender(uint32_t buf_addr)
{
	int i, j;

	xSemaphoreTakeRecursive(vdisbuf_mutex, portMAX_DELAY);
	
	for (i = 0; i < VIDEO_DISPLAY_BUF_NUM; i++) {
		if (vdisbufs[i].addr == buf_addr) {
			for (j = 0; j < VIDEO_DISPLAY_BUF_NUM; j++) {
				if (vdisbufs[j].status == VBUF_STATUS_RENDERED)
					vdisbufs[j].status = VBUF_STATUS_FREE;
			}
			vdisbufs[i].status = VBUF_STATUS_RENDERED;
			break;
		}
	}
	
	xSemaphoreGiveRecursive(vdisbuf_mutex);
}

void vVideoDisplayBufFree(uint32_t buf_addr)
{
	int i;

	xSemaphoreTakeRecursive(vdisbuf_mutex, portMAX_DELAY);

	for (i = 0; i < VIDEO_DISPLAY_BUF_NUM; i++) {
		if (vdisbufs[i].addr == buf_addr) {
			vdisbufs[i].status = VBUF_STATUS_FREE;
			break;
		}
	}

	xSemaphoreGiveRecursive(vdisbuf_mutex);
}

unsigned int ulVideoDisplayGetBufferAddr(int index)
{
	configASSERT(index < VIDEO_DISPLAY_BUF_NUM);
	return vdisbufs[index].addr;
}
