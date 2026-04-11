#ifndef _CAN_H
#define _CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#define CAN0               ( (CAN_TypeDef* )REGS_CAN0_BASE)
#define CAN1               ( (CAN_TypeDef* )REGS_CAN1_BASE)

#define CAN_InitStatus_Failed              ((unsigned char)0x00) /*!< CAN initialization failed */
#define CAN_InitStatus_Success             ((unsigned char)0x01) /*!< CAN initialization OK */

#define CAN_SJW_1tq                 ((unsigned char)0x00)  /*!< 1 time quantum */
#define CAN_SJW_2tq                 ((unsigned char)0x01)  /*!< 2 time quantum */
#define CAN_SJW_3tq                 ((unsigned char)0x02)  /*!< 3 time quantum */
#define CAN_SJW_4tq                 ((unsigned char)0x03)  /*!< 4 time quantum */

#define CAN_BS1_1tq                 ((unsigned char)0x00)  /*!< 1 time quantum */
#define CAN_BS1_2tq                 ((unsigned char)0x01)  /*!< 2 time quantum */
#define CAN_BS1_3tq                 ((unsigned char)0x02)  /*!< 3 time quantum */
#define CAN_BS1_4tq                 ((unsigned char)0x03)  /*!< 4 time quantum */
#define CAN_BS1_5tq                 ((unsigned char)0x04)  /*!< 5 time quantum */
#define CAN_BS1_6tq                 ((unsigned char)0x05)  /*!< 6 time quantum */
#define CAN_BS1_7tq                 ((unsigned char)0x06)  /*!< 7 time quantum */
#define CAN_BS1_8tq                 ((unsigned char)0x07)  /*!< 8 time quantum */
#define CAN_BS1_9tq                 ((unsigned char)0x08)  /*!< 9 time quantum */
#define CAN_BS1_10tq                ((unsigned char)0x09)  /*!< 10 time quantum */
#define CAN_BS1_11tq                ((unsigned char)0x0A)  /*!< 11 time quantum */
#define CAN_BS1_12tq                ((unsigned char)0x0B)  /*!< 12 time quantum */
#define CAN_BS1_13tq                ((unsigned char)0x0C)  /*!< 13 time quantum */
#define CAN_BS1_14tq                ((unsigned char)0x0D)  /*!< 14 time quantum */
#define CAN_BS1_15tq                ((unsigned char)0x0E)  /*!< 15 time quantum */
#define CAN_BS1_16tq                ((unsigned char)0x0F)  /*!< 16 time quantum */

#define CAN_BS2_1tq                 ((unsigned char)0x00)  /*!< 1 time quantum */
#define CAN_BS2_2tq                 ((unsigned char)0x01)  /*!< 2 time quantum */
#define CAN_BS2_3tq                 ((unsigned char)0x02)  /*!< 3 time quantum */
#define CAN_BS2_4tq                 ((unsigned char)0x03)  /*!< 4 time quantum */
#define CAN_BS2_5tq                 ((unsigned char)0x04)  /*!< 5 time quantum */
#define CAN_BS2_6tq                 ((unsigned char)0x05)  /*!< 6 time quantum */
#define CAN_BS2_7tq                 ((unsigned char)0x06)  /*!< 7 time quantum */
#define CAN_BS2_8tq                 ((unsigned char)0x07)  /*!< 8 time quantum */

#define CAN_Id_Standard           0
#define CAN_Id_Extended           1
#define CAN_RTR_DATA               0
#define CAN_RTR_Remote            1

/*!< CAN 控制状态寄存器 */
/************************** CAN_MOD 寄存器位定义*******************************/
#define CAN_Mode_RM                 ((unsigned char)0x01)  /*!< 复位模式 */
#define CAN_Mode_LOM                ((unsigned char)0x02)  /*!< 只听模式 1:只听  0:正常  */
#define CAN_Mode_STM                ((unsigned char)0x04)  /*!< 正常工作模式1:自检测  0:正常  */
#define CAN_Mode_AFM                ((unsigned char)0x08)  /*!< 单/双滤波模式 1:单 0: 双*/
#define CAN_Mode_SM                 ((unsigned char)0x10)  /*!< 睡眠模式1: 睡眠 0: 唤醒 */

/************************** CAN_CMR 寄存器位定义*******************************/
 #define  CAN_CMR_TR                         ((unsigned char)0x01)         /*!< 发送请求 1: 当前信息被发送  0: 空 */
 #define  CAN_CMR_AT                         ((unsigned char)0x02)         /*!< 中止发送 1: 等待发送的信息取消  0: 空缺  */
 #define  CAN_CMR_RRB                        ((unsigned char)0x04)         /*!< 释放接收缓冲器  1:释放  0: 无动作 */
 #define  CAN_CMR_CDO                        ((unsigned char)0x08)         /*!< 清除数据溢出  1:清除  0: 无动作    */
//#define  CAN_CMR_GTS                        ((unsigned char)0x10)        /*!< STD模式< 睡眠: 1:进入睡眠  0: 唤醒  */
 #define  CAN_CMR_SRR                        ((unsigned char)0x10)         /*!< 自接收请求  1:  0:   */
 #define  CAN_CMR_EFF                        ((unsigned char)0x80)         /*!< 扩展模式 1:扩展帧 0: 标准帧  */

/************************** CAN_SR 寄存器位定义********************************/
 #define  CAN_SR_BBS                         ((unsigned char)0x01)         /*!< 接收缓存器状态1: 满  0: 空 */
 #define  CAN_SR_DOS                         ((unsigned char)0x02)         /*!< 数据溢出状态 1: 溢出  0: 空缺  */
 #define  CAN_SR_TBS                         ((unsigned char)0x04)         /*!< 发送缓存器状态1: 释放  0: 锁定 */
 #define  CAN_SR_TCS                         ((unsigned char)0x08)         /*!< 发送完毕状态1: 完毕  0: 未完毕    */
 #define  CAN_SR_RS                          ((unsigned char)0x10)         /*!< 接收状态1: 接收  0: 空闲  */
 #define  CAN_SR_TS                          ((unsigned char)0x20)         /*!< 发送状态1:  发送 0:  空闲*/
 #define  CAN_SR_ES                          ((unsigned char)0x40)         /*!< 出错状态1:出错 0: 正常 */
 #define  CAN_SR_BS                          ((unsigned char)0x80)         /*!< 总线状态1: 关闭  0: 开启  */
 
/************************** CAN_IR 中断寄存器位定义****************************/
 #define  CAN_IR_RI                          ((unsigned char)0x01)         /*!< 接收中断 */
 #define  CAN_IR_TI                          ((unsigned char)0x02)         /*!< 发送中断 */
 #define  CAN_IR_EI                          ((unsigned char)0x04)         /*!< 错误中断 */
 #define  CAN_IR_DOI                         ((unsigned char)0x08)         /*!< 数据溢出中断  */
 #define  CAN_IR_WUI                         ((unsigned char)0x10)         /*!< 唤醒中断 */
 #define  CAN_IR_EPI                         ((unsigned char)0x20)         /*!< 错误消极中断 */
 #define  CAN_IR_ALI                         ((unsigned char)0x40)         /*!< 仲裁丢失中断 */
 #define  CAN_IR_BEI                         ((unsigned char)0x80)         /*!< 总线错误中断  */
 
/************************* CAN_IER 中断使能寄存器位定义************************/
 #define  CAN_IER_RIE                         ((unsigned char)0x01)        /*!< 接收中断使能 */
 #define  CAN_IER_TIE                         ((unsigned char)0x02)        /*!< 发送中断使能 */
 #define  CAN_IER_EIE                         ((unsigned char)0x04)        /*!< 错误中断使能 */
 #define  CAN_IER_DOIE                        ((unsigned char)0x08)        /*!< 数据溢出中断使能  */
 #define  CAN_IER_WUIE                        ((unsigned char)0x10)        /*!< 唤醒中断使能 */
 #define  CAN_IER_EPIE                        ((unsigned char)0x20)        /*!< 错误消极中断使能 */
 #define  CAN_IER_ALIE                        ((unsigned char)0x40)        /*!< 仲裁丢失中断使能 */
 #define  CAN_IER_BEIE                        ((unsigned char)0x80)        /*!< 总线错误中断使能  */
 
typedef enum 
{
	CAN1MBaud=0,    // 1 MBit/sec
	CAN800kBaud,    // 800 kBit/sec
	CAN500kBaud,    // 500 kBit/sec
	CAN250kBaud,    // 250 kBit/sec
	CAN125kBaud,    // 125 kBit/sec
	CAN100kBaud,    // 100 kBit/sec
	CAN50kBaud,     // 50 kBit/sec
	CAN40kBaud,     // 40 kBit/sec
} CanBPS_t;

typedef enum 
{
	CAN_MODE_NORMAL=0,
	CAN_MODE_LISEN,
	CAN_MODE_LOOPBACK,
	CAN_MODE_LOOPBACKANLISEN,
} CanMode_t;

typedef struct
{
   unsigned int MOD;
   unsigned int CMR;
   unsigned int SR;
   unsigned int IR;
   unsigned int IER;
   unsigned int reserved0;
   unsigned int BTR0;
   unsigned int BTR1;
   unsigned int OCR;
   unsigned int reserved[2];
   unsigned int ALC;
   unsigned int ECC ;
   unsigned int EWLR;
   unsigned int RXERR;
   unsigned int TXERR;
   unsigned int IDE_RTR_DLC;
   unsigned int ID[4];
   unsigned int BUF[8];
   unsigned int RMC;
   unsigned int RBSA;
   unsigned int CDR;
} CAN_TypeDef;

typedef struct
{
  unsigned char  CAN_Prescaler;    /* 波特率分频系数1 to 63. */
  unsigned char  CAN_Mode;         /*0x10:睡眠0x08:单,双滤波 0x40:正常工作0x20:只听 0x01:复位*/
  unsigned char  CAN_SJW;          /*同步跳转宽度 */
  unsigned char  CAN_BS1;          /*时间段1计数值*/
  unsigned char  CAN_BS2;          /*时间段2计数值*/ 
  
} CAN_InitTypeDef;

typedef struct
{
  unsigned char  IDE;        /*0: 使用标准标识符1: 使用扩展标识符*/
  unsigned char  RTR;    /*0: 数据帧     1: 远程帧*/
  unsigned char  MODE;        /* 0- 双滤波器模式;1-单滤波器模式*/
  unsigned long  First_Data;    /*双滤波器模式下信息第一个数据字节*/
  unsigned long  Data_Mask;    /*双滤波器模式下信息第一个数据字节屏蔽*/
  unsigned long  ID;        /*验收代码*/
  /*
 双滤波器-  扩展帧: 2个滤波器的前16位,分别放在ID 的前16位和 ID的后16位.
 双滤波器-  标准帧: 2个滤波器的11位,分别放在ID 的前16位和 ID的后16位,第1个滤波器同时使用First_Data和Data_Mask
 单滤波器-  扩展帧: 使用29位, 放在ID 的后29位.
 单滤波器-  标准帧: 使用11位, 放在ID 的后11位.
  */
  unsigned long  IDMASK;    /*验收屏蔽*/
} CAN_FilterInitTypeDef;

typedef struct
{
  unsigned long StdId;  /* 11位ID*/
  unsigned long ExtId;  /*29位ID**/
  unsigned char IDE;    /*IDE: 标识符选择
                                     该位决定发送邮箱中报文使用的标识符类型
                                     0: 使用标准标识符
                                     1: 使用扩展标识符*/
  unsigned char RTR;     /*远程发送请求
                                       0: 数据帧
                                       1: 远程帧*/
  unsigned char DLC;     /*数据帧长度*/
  unsigned char Data[8]; /*8字节数据*/
} CanMsg;

unsigned char CAN_Init(CAN_TypeDef* CANx, CAN_InitTypeDef* CAN_InitStruct);
void CAN_Transmit(CAN_TypeDef* CANx, CanMsg* TxMessage);
void CAN_Receive(CAN_TypeDef* CANx,  CanMsg* RxMessage);

unsigned char  can_set_reset_mode(CAN_TypeDef* CANx);
unsigned char  can_set_start(CAN_TypeDef* CANx);

typedef enum {
	CAN_ID0 = 0,
	CAN_ID1,
	CAN_NUM,
} eCanID;

typedef struct {
    uint32_t baud_rate;
    uint32_t msgboxsz;
    uint32_t sndboxnumber;
    uint32_t mode      : 8;
    uint32_t privmode  : 8;
    uint32_t reserved  : 16;
    uint32_t ticks;
} CanConfigure_t;

typedef struct {
	uint32_t id;
	int irq;
	CAN_TypeDef *pcan;
	SemaphoreHandle_t xRxMutex;
	SemaphoreHandle_t xRev;
	SemaphoreHandle_t xSend;
	SemaphoreHandle_t xTxMutex;
	TaskHandle_t xTxThread;
	QueueHandle_t tx_done;
	List_t rxRevList;
	List_t rxFreeList;
	List_t txSendList;
	List_t txFreeList;
} CanPort_t;

CanPort_t *xCanOpen(uint32_t id);
void vCanClose(CanPort_t *cap);
void vCanSetFilter(CanPort_t *cap,	 CAN_FilterInitTypeDef * CAN_FilterInitStruct);
int iCanWrite(CanPort_t *cap, CanMsg* messages, int nmsgs, TickType_t xBlockTime);
int iCanRead(CanPort_t *cap, CanMsg* messages, int nmsgs, TickType_t xBlockTime);
void vCanInit(CanPort_t *cap,  CanBPS_t baud, CanMode_t mode);
int iCanGetReceiveErrorCount(CanPort_t *cap);
int iCanGetTransmitErrorCount(CanPort_t *cap);

int can_demo(void);

#ifdef __cplusplus
}
#endif

#endif
