#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"

#ifdef REMOTE_SUPPORT
/* Remote */
#define rRC_DATA0						0x00
#define rRC_DATA1						0x04
#define rRC_DATA2						0x08
#define rRC_DATA3						0x0C
#define rRC_DATA4						0x10
#define rRC_DATA5						0x14
#define rRC_DATA6						0x18
#define rRC_DATA7						0x1C
#define rRC_CODEBUF						0x20
#define rRC_CODEVAL						0x24
#define rRC_KEYVAL						0x28
#define rRC_STATUS						0x2C
#define rRC_RT_USER_CODE_3_4         	0x30
#define rRC_RT_INTER_REQ_CLR          	0x34

#define NORMAL_KEY						0
#define RELEASE_KEY						1
#define REPEAT_KEY						2

#define REMOTE_NULL						0
#define	REMOTE_SCAN						1			// scan state
#define	REMOTE_CHECK					2			// check state
#define	REMOTE_DELAY					3			// repeat delay state
#define	REMOTE_REPEAT					4			// repeat state

#define REMOTE_STATE_IDLE				0
#define REMOTE_STATE_PRESS				1
#define REMOTE_STATE_REPEATE			2

#define REMOTE_PRESS_EVENT				0
#define REMOTE_RELEASE_EVENT			1
#define REMOTE_REPEATE_EVENT			2


//static UINT32 lg_ulRemoteStateMachine = REMOTE_STATE_IDLE;
//static unsigned short Remotekey_delay_time;
//static int Remotekey_state;
//static UINT32 lg_ulLastRepeatMs = 0;
static UINT32 lg_ulMaxRepeatMs = 0;
volatile UINT32 cRemoteKey;               // 当前键值
volatile UINT32 cRemoteStatus;            // 当前键状态


static void remote_config(void) //进行遥控接收器的参数配置
{
	printf("remote_config\n");

#if 1
	// nec
	INT8 pulse_data_polarity = 0x01;//[3 :0 ]
	INT8 valid_bitrange = 0x04;//[6 :4 ]
	INT8 dispersion = 0x5;//[15:8 ]
	INT8 prediv = 0x3f;//[23:16]   24M: 0x1f  48M: 0x3f
	INT8 filtertime = 0x10;//[31:24]

	INT8 start_valueh = 0x69;//[23:16]
	INT8 start_valuel = 0x34;//[31:24]

	INT8 one_valueh = 0x6; //0x6;//[7 : 0]
	INT8 one_valuel = 0x13;//[15: 8]

	INT8 zero_valueh = 0x6;//[23:16]
	INT8 zero_valuel = 0x6;//[31:24]

	INT8 rp0_valueh = 0x0f;//[7 : 0]
	INT8 rp0_valuel = 0x0f;//[15: 8]

	INT8 rp1_valueh = 0x0f;//[23:16]
	INT8 rp1_valuel = 0x0f;//[31:24]

	INT8 rp2_3_valueh = 0x69;//[7 : 0]
	INT8 rp2_3_valuel = 0x1c;//[15: 8]

	INT8 rp4_5_valueh =0x6;//[23:16]
	INT8 rp4_5_valuel = 0xf5;//[31:24]

	INT8 keyrelease_timeh = 0x00;//[7 : 0]
	INT8 keyrelease_timel = 0x59;//[15: 8]
    INT8  RP_5L_delta=0XF0;
	INT8 int_num = 0x01;//[23:16]
	INT8 mode_sel_reg = 0x03;//[25:24]
	INT8 nec_release_int_en = 0x01;//[26]
	UINT8 user_code1_l = 0x00;//[7 : 0]
	UINT8 user_code1_h = 0xFF;//[15: 8]
	UINT8 user_code2_l = 0x00;//[23:16]
	UINT8 user_code2_h =0xFF;//[31:24]
	INT32 NEC_bit_2_pulse = 0x2ff;//[15: 0]
//	INT8 user_code_sel_reg = 0x01;//[23:16]

//	INT8 user_code3_l = 0x02;//[7 : 0]
//	INT8 user_code3_h = 0x00;//[15: 8]
//	INT8 user_code4_l = 0xff;//[23:16]
//	INT8 user_code4_h =0x00;//[31:24]
	
	INT8 user_code_sel =0;
	INT8 custom_jud_sel=1;
	INT8 custom_not_jud_sel=1;
	INT8 data_not_jud_sel=1;
    uint32_t val;
   
    uint32_t remote_param0, remote_param1, remote_param2, remote_param3 ;
	uint32_t remote_param4, remote_param5, remote_param6, remote_param7 ;
#endif

	remote_param0  = (pulse_data_polarity<<0)
				+ (valid_bitrange<<4)
				+ (dispersion<<7)
				+ (prediv<<15)
				+ (filtertime<<24);

	remote_param1 = (RP_5L_delta<< 0)
				+ (start_valueh<<16)
			   + (start_valuel<<24);


	remote_param2 = (one_valueh<<0)
			   + (one_valuel<<8)
			   + (zero_valueh<<16)
			   + (zero_valuel<<24);

	remote_param3 = (rp0_valueh<<0)
			   + (rp0_valuel<<8)
			   + (rp1_valueh<<16)
			   + (rp1_valuel<<24);

	remote_param4 = (rp2_3_valueh<<0)
			   + (rp2_3_valuel<<8)
			   + (rp4_5_valueh<<16)
			   + (rp4_5_valuel<<24);

	remote_param5 = (keyrelease_timel<<0)
			   + (keyrelease_timeh<<8)
			   + (int_num<<16)
			   + (mode_sel_reg<<24)
			   + (nec_release_int_en << 26);


	remote_param6 = ((user_code1_l<<0)
			   + (user_code1_h<<8)
			   + (user_code2_l<<16)
			   + (user_code2_h<<24));

/*	rRC_RT_USER_CODE_3_4 =  (user_code3_l<<0)
			   + (user_code3_h<<8)
			   + (user_code4_l<<16)
			   + (user_code4_h<<24);
*/
	remote_param7 =(NEC_bit_2_pulse<<0)
			   + (user_code_sel<<16)
			   +(custom_jud_sel<<24)
			   +(custom_not_jud_sel<<25)
			   +(data_not_jud_sel<<26);

	val = readl(REGS_SYSCTL_BASE + SYS_PER_CLK_CFG);
	val &= ~0x0f;
	writel(val, REGS_SYSCTL_BASE + SYS_PER_CLK_CFG);
    writel(remote_param0, REGS_RCRT_BASE+rRC_DATA0);
	writel(remote_param1, REGS_RCRT_BASE+rRC_DATA1);
	writel(remote_param2, REGS_RCRT_BASE+rRC_DATA2);
	writel(remote_param3, REGS_RCRT_BASE+rRC_DATA3);
	writel(remote_param4, REGS_RCRT_BASE+rRC_DATA4);
	writel(remote_param5, REGS_RCRT_BASE+rRC_DATA5);
	writel(remote_param6, REGS_RCRT_BASE+rRC_DATA6);
	writel(remote_param7, REGS_RCRT_BASE+rRC_DATA7);
}


/*********************************************************************
	Set the report interval for remote repeating event
Parameter:
	ulMillisecond: interval, unit as millisecond, this parameter should be the multiply of
	 10 millsecond.
*********************************************************************/
void SetRemoteKeyRepeateInterval(UINT32 ulMillisecond)
{
	lg_ulMaxRepeatMs = ulMillisecond/10;
}

/*********************************************************************
	Get the report interval for remote repeating event
Return:
	millisendonds for interval
*********************************************************************/
UINT32 GetRemoteKeyRepeatInterval(void)
{
	return lg_ulMaxRepeatMs * 10;
}

static void  remote_int_handler(void *param)
{
//	printf("Enter remote interrupt!\n");
	cRemoteStatus=(readl(REGS_RCRT_BASE + rRC_STATUS))&0x0F;// 取遥控码状态

	cRemoteKey=readl(REGS_RCRT_BASE + rRC_KEYVAL);

	printf(" cRemoteKey is%x.\n",cRemoteKey);

	writel(0xff, REGS_RCRT_BASE+rRC_RT_INTER_REQ_CLR);
	if(cRemoteStatus & 0x01)
		printf(" release detect\n");	
	else
		printf(" cRemoteStatus: %x cRemoteKey: %x\n",cRemoteStatus,cRemoteKey);
}

void RemoteKeyInit(void)
{
	cRemoteKey = REMOTE_NULL;
	cRemoteStatus = NORMAL_KEY;
	sys_soft_reset(softreset_rcrt);
	remote_config();
	request_irq(RCRT_IRQn,0,remote_int_handler,NULL);
	printf("RemoteKeyInit\n");
}
#endif
