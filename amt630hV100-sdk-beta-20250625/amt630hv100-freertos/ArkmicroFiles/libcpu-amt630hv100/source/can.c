#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"

//#define CAN_USE_TX_DEMO

#define CAN_RX_BUF_NUM	32
#define CAN_TX_BUF_NUM	32
CanMsg canRxMsg[CAN_NUM][CAN_RX_BUF_NUM] = {0};
ListItem_t canRxListItem[CAN_NUM][CAN_RX_BUF_NUM];
CanMsg canTxMsg[CAN_NUM][CAN_TX_BUF_NUM] = {0};
ListItem_t canTxListItem[CAN_NUM][CAN_TX_BUF_NUM];

static CanPort_t *pxCanPort[CAN_NUM] = {NULL};
static uint32_t ulCanBase[CAN_NUM] = {REGS_CAN0_BASE, REGS_CAN1_BASE};
static CAN_InitTypeDef CanInitValue[CAN_NUM];
static CAN_FilterInitTypeDef CanFilterInitValue[CAN_NUM];
static int nCanFilterEnable[CAN_NUM];

unsigned char can_set_reset_mode(CAN_TypeDef* CANx)
{
	unsigned char status;
	int i;

	/*检查复位标志*/
	status = CANx->MOD;

	/* 关闭中断 */
	CANx->IER = 0x00;

	for (i = 0; i < 100; i++)
	{
		if((status & CAN_Mode_RM) == CAN_Mode_RM)
		    return CAN_InitStatus_Success;

		/* 设置复位*/
		CANx->MOD |=  ((unsigned char)CAN_Mode_RM);

		/*延时*/
		udelay(10);

		/*检查复位标志*/
		status = CANx->MOD;
	}
	printf("Setting can into reset mode failed!\n");
	return CAN_InitStatus_Failed;  
}

static unsigned char  set_normal_mode(CAN_TypeDef* CANx)
{
	unsigned char status;
	int i;

	/*检查复位标志*/
	status = CANx->MOD;

	for (i = 0; i < 100; i++)
	{
		if((status & CAN_Mode_RM) != CAN_Mode_RM)
		{
			/*开所有中断 （总线错误中断不开）*/
			CANx->IER |= (~(unsigned char)CAN_IR_BEI);	
			return CAN_InitStatus_Success;
		}
		/* 设置正常工作模式*/
		CANx->MOD &= (~(unsigned char) CAN_Mode_RM); 
		/*延时*/
		udelay(10); 
		status = CANx->MOD;	
	}
	printf("Setting can into normal mode failed!\n");
	return CAN_InitStatus_Failed;
}

unsigned char can_set_start(CAN_TypeDef* CANx)
{
	/*复位TX错误计数器*/
	CANx->TXERR = 0;
	/*复位RX错误计数器*/
	CANx->RXERR = 0;
	/*时钟分频寄存器: PeliCAN模式; CBP=1,中止输入比较器, RX0激活*/
	CANx->CDR = 0xC0;	

	return  set_normal_mode(CANx);  
}

unsigned char CAN_Init(CAN_TypeDef* CANx, CAN_InitTypeDef* CAN_InitStruct)
{
	unsigned char InitStatus = CAN_InitStatus_Failed;
	unsigned char status;

	status = CANx->MOD;
	if( status == 0xFF)
	{
		printf("Probe can failed \n");  	   
		return CAN_InitStatus_Failed;
	}

	/* 进入复位模式 */
	InitStatus = can_set_reset_mode(CANx);

	/* set acceptance filter (accept all) */
	CANx->IDE_RTR_DLC = 0x0;
	CANx->ID[0] =  0x0;
	CANx->ID[1] =  0x0;
	CANx->ID[2] =  0x0;
	CANx->ID[3] =  0xFF;
	CANx->BUF[0] = 0xFF;
	CANx->BUF[1] = 0xFF;
	CANx->BUF[2] = 0xFF;

	if((CAN_InitStruct->CAN_Mode & CAN_Mode_SM) == CAN_Mode_SM) 
	{
		/* 睡眠模式 1: 睡眠 0: 唤醒*/
		CANx->MOD|= (unsigned char)CAN_Mode_SM;
	}
	else
	{
		CANx->MOD&=~ (unsigned char)CAN_Mode_SM;
	}

	if((CAN_InitStruct->CAN_Mode & CAN_Mode_LOM) == CAN_Mode_LOM) 
	{
		/*只听模式 1:只听  0:正常 */
		CANx->MOD|= (unsigned char)CAN_Mode_LOM;    
	}
	else
	{
		CANx->MOD&=~ (unsigned char)CAN_Mode_LOM; 
	}

	if((CAN_InitStruct->CAN_Mode & CAN_Mode_AFM) == CAN_Mode_AFM) 
	{
		/*单滤波模式 1:单 0: 双*/
		CANx->MOD |= (unsigned char)CAN_Mode_AFM;    
	}
	else
	{
		CANx->MOD&=~ (unsigned char)CAN_Mode_AFM; 
	}

	if((CAN_InitStruct->CAN_Mode & CAN_Mode_STM) == CAN_Mode_STM) 
	{
		/*自检测模式 1:自检测  0:正常  */
		CANx->MOD |= (unsigned char)CAN_Mode_STM;    
	}
	else
	{
		CANx->MOD&=~ (unsigned char)CAN_Mode_STM;
	}

	/* 配置时钟频率 */
	CANx->BTR0 = (( unsigned char )( unsigned char )CAN_InitStruct->CAN_Prescaler -1) | \
	       (unsigned char)CAN_InitStruct->CAN_SJW << 6;

	CANx->BTR1 = ((unsigned char)CAN_InitStruct->CAN_BS1) | \
	       ((unsigned char)CAN_InitStruct->CAN_BS2 << 4);

	/* 接收发送错误阈值，错误超过该值会产生错误中断
	 * 默认值是96，有需要可以设置不同的值 */
	//CANx->EWLR = 96;

	/* 进入工作模式 */
	can_set_start(CANx);    

	/* 返回初始化结果 */
	return InitStatus;
}

void CAN_Transmit(CAN_TypeDef* CANx, CanMsg* TxMessage)
{
	int i;
	if (TxMessage->IDE == CAN_Id_Extended)
	{
		CANx->ID[0]= TxMessage ->ExtId>> 21;
		CANx->ID[1]= TxMessage ->ExtId>> 13;
		CANx->ID[2]= TxMessage ->ExtId>> 5;
		CANx->ID[3]= TxMessage ->ExtId<<3;
		CANx->IDE_RTR_DLC= (TxMessage ->IDE & 0x01) << 7 |\
		(TxMessage ->RTR & 0x01) << 6 |\
		(TxMessage ->DLC & 0x0F);
		for( i=0;i<TxMessage ->DLC; i++)
		{
			CANx->BUF[i]= TxMessage->Data[i];
		}
	}
	else if (TxMessage->IDE ==CAN_Id_Standard)
	{
		CANx->ID[0]= TxMessage ->StdId>> 3;
		CANx->ID[1]= TxMessage ->StdId<< 5;
		CANx->IDE_RTR_DLC= (TxMessage ->IDE & 0x01) << 7 |\
		(TxMessage ->RTR & 0x01) << 6 |\
		(TxMessage ->DLC & 0x0F);
		CANx->ID[2]= TxMessage ->Data[0];
		CANx->ID[3]= TxMessage ->Data[1];
		for( i=0;i<TxMessage ->DLC-2; i++)
		{
			CANx->BUF[i]= TxMessage->Data[i+2];
		}
	}
	CANx->CMR = CAN_CMR_TR ;
}

void CAN_Receive(CAN_TypeDef* CANx,  CanMsg* RxMessage)
{
	/* 获取 IDE */
	RxMessage->IDE = (CANx->IDE_RTR_DLC  & 0x80)>>7;
	/* 获取 RTR */
	RxMessage->RTR = (CANx->IDE_RTR_DLC  & 0x40)>>6;
	/* 获取 DLC */
	RxMessage->DLC= (CANx->IDE_RTR_DLC  & 0x0F);
	if (RxMessage->IDE == CAN_Id_Standard)
	{
		RxMessage->StdId = CANx->ID[0]<<3 |CANx->ID[1]>>5 ;
		/* 获取数据 */
		RxMessage->Data[0] = (unsigned char)CANx->ID[2];
		RxMessage->Data[1] = (unsigned char)CANx->ID[3];
		RxMessage->Data[2] = (unsigned char)CANx->BUF[0];
		RxMessage->Data[3] = (unsigned char)CANx->BUF[1];
		RxMessage->Data[4] = (unsigned char)CANx->BUF[2];
		RxMessage->Data[5] = (unsigned char)CANx->BUF[3];
		RxMessage->Data[6] = (unsigned char)CANx->BUF[4];
		RxMessage->Data[7] = (unsigned char)CANx->BUF[5];
	}
	else  if (RxMessage->IDE == CAN_Id_Extended)
	{
		RxMessage->ExtId= CANx->ID[0]<<21 |CANx->ID[1]<<13|CANx->ID[2]<<5|CANx->ID[3]>>3 ;
		/* 获取数据 */
		RxMessage->Data[0] = (unsigned char)CANx->BUF[0];
		RxMessage->Data[1] = (unsigned char)CANx->BUF[1];
		RxMessage->Data[2] = (unsigned char)CANx->BUF[2];
		RxMessage->Data[3] = (unsigned char)CANx->BUF[3];
		RxMessage->Data[4] = (unsigned char)CANx->BUF[4];
		RxMessage->Data[5] = (unsigned char)CANx->BUF[5];
		RxMessage->Data[6] = (unsigned char)CANx->BUF[6];
		RxMessage->Data[7] = (unsigned char)CANx->BUF[7];
	}  
}

static void can_reinit(CanPort_t *cap)
{
	if (cap->id == CAN_ID0)
		sys_soft_reset_from_isr(softreset_can0);
	else if (cap->id == CAN_ID1)
		sys_soft_reset_from_isr(softreset_can1);

	CAN_Init(cap->pcan, &CanInitValue[cap->id]);

	if (nCanFilterEnable[cap->id]) {
		vCanSetFilter(cap, &CanFilterInitValue[cap->id]);
	}
}

static void vCanIntHandler(void *param)
{
	CanPort_t *cap = param;
	CAN_TypeDef* CANx = cap->pcan;
	CanMsg RxMessage;
	unsigned char status;

	/*读寄存器清除中断*/
	status = CANx->IR;

	/*接收中断*/
	if (status & CAN_IR_RI)
	{
		/*清除RI 中断*/
		CAN_Receive(CANx, &RxMessage);
		CANx->CMR |= CAN_CMR_RRB; 
		CANx->CMR |= CAN_CMR_CDO; 
		//rt_hw_can_isr(&bxcan0, RT_CAN_EVENT_RX_IND);
		//printf("Can0 int RX happened!\n");
		if (!listLIST_IS_EMPTY(&cap->rxFreeList)) {
			ListItem_t *item = listGET_HEAD_ENTRY(&cap->rxFreeList);
		
			memcpy((void*)listGET_LIST_ITEM_VALUE(item), &RxMessage, sizeof(CanMsg));
			uxListRemove(item);
			vListInsertEnd(&cap->rxRevList, item);
			xSemaphoreGiveFromISR(cap->xRev, 0);
		} else {
			/* can数据吞吐量大的时候中断处理函数添加打印会很容易造成数据接收溢出 */
			;//printf("can rx buf is full, drop a message.\n");
		}
	}
	/*发送中断*/
	if (status  & CAN_IR_TI)
	{
		//printf("Can0 int TX happened!\n");
		xQueueSendFromISR(cap->tx_done, NULL, 0);
	}
	/*数据溢出中断*/
	if (status  & CAN_IR_DOI)
	{
		printf("Can%d int RX OF happened!\n", cap->id);
		can_reinit(cap);
	}
	/*错误中断*/
	if (status  & CAN_IR_EI)
	{
		printf("Can%d int ERROR happened!\n", cap->id);
		if (CANx->SR & CAN_SR_ES) {
			/* 接收或者发送错误超过设置的阈值 */
			can_reinit(cap);
		} else if (CANx->SR & CAN_SR_BS) {
			/* bus off错误，退出reset模式，控制器会在检测到128次11个连续的隐性位
			 * 后恢复到bus on状态 */
			set_normal_mode(CANx);
		}
	}	
}

static void can_transmit_thread(void *param)
{
	CanPort_t *cap = param;
	ListItem_t *item;
	CanMsg *msgs;

	for (;;) {
		xSemaphoreTake(cap->xSend, portMAX_DELAY);

		portENTER_CRITICAL();
		if(!listLIST_IS_EMPTY(&cap->txSendList)) {
			item = listGET_HEAD_ENTRY(&cap->txSendList);
			msgs = (CanMsg *)listGET_LIST_ITEM_VALUE(item);
			uxListRemove(item);
			vListInsertEnd(&cap->txFreeList, item);
			portEXIT_CRITICAL();
			xQueueReset(cap->tx_done);
			CAN_Transmit(cap->pcan, msgs);
			if (xQueueReceive(cap->tx_done, NULL, pdMS_TO_TICKS(1000)) != pdTRUE) {
				/* transmit timeout, reset can */
				can_reinit(cap);
			}
		} else {
			portEXIT_CRITICAL();
		}
	}
}

CanPort_t *xCanOpen(uint32_t id)
{
	int i;
	
	if (id >= CAN_NUM) {
		TRACE_INFO("Wrong input can id.\n");
		return NULL;
	}

	if (pxCanPort[id] != NULL) {
		TRACE_ERROR("Can %d is already in use.\n", id);
		return NULL;
	}
	
	CanPort_t *cap = (CanPort_t *)pvPortMalloc(sizeof(CanPort_t));
	if (cap == NULL) {
		TRACE_ERROR("Out of memory for can%d.\n", id);
		return NULL;
	}
	memset(cap, 0, sizeof(*cap));

	cap->xRxMutex = xSemaphoreCreateMutex();
	configASSERT(cap->xRxMutex);

	cap->xRev = xSemaphoreCreateCounting(CAN_RX_BUF_NUM, 0);
	configASSERT(cap->xRev);

	cap->xSend = xSemaphoreCreateCounting(CAN_TX_BUF_NUM, 0);
	configASSERT(cap->xSend);

	cap->xTxMutex = xSemaphoreCreateMutex();
	configASSERT(cap->xTxMutex);

	cap->tx_done = xQueueCreate(1, 0);
	configASSERT(cap->tx_done);

	cap->id = id;

	vListInitialise(&cap->rxRevList);
	vListInitialise(&cap->rxFreeList);
	for (i = 0; i < CAN_RX_BUF_NUM; i++) {
		vListInitialiseItem(&canRxListItem[cap->id][i]);
		listSET_LIST_ITEM_VALUE(&canRxListItem[cap->id][i], (uint32_t)&canRxMsg[cap->id][i]);
		vListInsertEnd(&cap->rxFreeList, &canRxListItem[cap->id][i]);
	}

	vListInitialise(&cap->txSendList);
	vListInitialise(&cap->txFreeList);
	for (i = 0; i < CAN_TX_BUF_NUM; i++) {
		vListInitialiseItem(&canTxListItem[cap->id][i]);
		listSET_LIST_ITEM_VALUE(&canTxListItem[cap->id][i], (uint32_t)&canTxMsg[cap->id][i]);
		vListInsertEnd(&cap->txFreeList, &canTxListItem[cap->id][i]);
	}


	cap->pcan = (CAN_TypeDef *)ulCanBase[id];
	
	cap->irq = CAN0_IRQn + id;
	request_irq(cap->irq, 0, vCanIntHandler, cap);

	/* Create a task to transmit can msg */
	if (xTaskCreate(can_transmit_thread, "cantrm", configMINIMAL_STACK_SIZE, cap,
		configMAX_PRIORITIES / 3 + 2, &cap->xTxThread) != pdPASS) {
		printf("create can transmit task fail.\n");
		vCanClose(cap);
		return NULL;
	}

	return pxCanPort[id] = cap;
};

void vCanInit(CanPort_t *cap,  CanBPS_t baud, CanMode_t mode)
{
	CAN_InitTypeDef CAN_InitStructure;

	switch (mode)
	{
	case CAN_MODE_NORMAL:
		CAN_InitStructure.CAN_Mode = 0x00;
		break;
	case CAN_MODE_LISEN:
		CAN_InitStructure.CAN_Mode = CAN_Mode_LOM;
		break;
	case CAN_MODE_LOOPBACK:
		CAN_InitStructure.CAN_Mode = CAN_Mode_STM;
		break;
	case CAN_MODE_LOOPBACKANLISEN:
		CAN_InitStructure.CAN_Mode = CAN_Mode_STM|CAN_Mode_LOM;
		break;
	}
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;

	switch (baud)
	{
	case CAN1MBaud: 
		CAN_InitStructure.CAN_Prescaler = 11;
		CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
		break;
	case CAN800kBaud: 
		CAN_InitStructure.CAN_Prescaler = 11;
		CAN_InitStructure.CAN_BS1 = CAN_BS1_8tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
		break;
	case CAN500kBaud: 
		CAN_InitStructure.CAN_Prescaler = 22;
		CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
		break;
	case CAN250kBaud: 
		CAN_InitStructure.CAN_Prescaler = 44;
		CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
		break;
	case CAN125kBaud: 
		CAN_InitStructure.CAN_Prescaler = 44;
		CAN_InitStructure.CAN_BS1 = CAN_BS1_14tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_3tq;
		break;

	default: //250K
			CAN_InitStructure.CAN_Prescaler = 40;
		CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
		CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
		break;
	}

	CanInitValue[cap->id] = CAN_InitStructure;

	CAN_Init(cap->pcan, &CAN_InitStructure);
}

void vCanClose(CanPort_t *cap)
{
	if (cap == NULL) return;

	if (cap->xTxThread)
		vTaskDelete(cap->xTxThread);

	can_set_reset_mode(cap->pcan);

	vQueueDelete(cap->tx_done);
	vSemaphoreDelete(cap->xRev);
	vSemaphoreDelete(cap->xSend);
	vSemaphoreDelete(cap->xRxMutex);
	vSemaphoreDelete(cap->xTxMutex);
	free_irq(cap->irq);
	vPortFree(cap);
	pxCanPort[cap->id] = NULL;
}

void vCanSetFilter(CanPort_t *cap,     CAN_FilterInitTypeDef * CAN_FilterInitStruct)
{
	unsigned long rtr;
	unsigned long fcase;
	unsigned long ide;
	unsigned long thisid, thisid1, thisid2;
	unsigned long thismask, thismask1, thismask2;
	unsigned long firstdata;
	unsigned long datamask;
	unsigned char CAN_FilterId0, CAN_FilterId1, CAN_FilterId2, CAN_FilterId3 ;
	unsigned char CAN_FilterMaskId0, CAN_FilterMaskId1, CAN_FilterMaskId2, CAN_FilterMaskId3;
	CAN_TypeDef* CANx = cap->pcan;

	thisid = CAN_FilterInitStruct->ID;
	thismask = CAN_FilterInitStruct->IDMASK;
	thisid1 = (CAN_FilterInitStruct->ID & 0xFFFF0000 )>>16;
	thismask1 = (CAN_FilterInitStruct->IDMASK & 0xFFFF0000 )>>16;
	thisid2 = (CAN_FilterInitStruct->ID & 0x0000FFFF );
	thismask2 = ( CAN_FilterInitStruct->IDMASK& 0x0000FFFF  );
	rtr = CAN_FilterInitStruct->RTR; 
	ide = CAN_FilterInitStruct->IDE;
	firstdata = CAN_FilterInitStruct->First_Data;
	datamask = CAN_FilterInitStruct->Data_Mask;
	fcase = CAN_FilterInitStruct->MODE; 

	if(ide == 0)//标准帧
	{
		if(fcase == 0)// 0- 双滤波器模式
		{
			CAN_FilterId0  = thisid1>>3;
			CAN_FilterMaskId0 = thismask1>>3;
			CAN_FilterId1  = thisid1<<5 | firstdata>>4| rtr<<4;
			CAN_FilterMaskId1 = thismask1<<4 | datamask>>4 ;
			CAN_FilterId2  = thisid2 >> 3;
			CAN_FilterMaskId2 = thismask2 >>3;
			CAN_FilterId3  = firstdata & 0x0F | thisid2 <<5 | rtr<<4;
			CAN_FilterMaskId3 = datamask <<4  ;
		}
		else if(fcase == 1)// 1-单滤波器模式
		{
			CAN_FilterId0  = thisid>>3;
			CAN_FilterMaskId0 = thismask>>3;
			CAN_FilterId1  = thisid<<5 | rtr<<4;
			CAN_FilterMaskId1 = thismask<<5  ;
			CAN_FilterMaskId1 |= 0x0F ;
			CAN_FilterId2  = 0x00;
			CAN_FilterMaskId2 = 0xFF;
			CAN_FilterId3  = 0x00;
			CAN_FilterMaskId3 = 0xFF  ;
		}
	}
	else if(ide == 1)//扩展帧
	{
		if(fcase == 0)// 0- 双滤波器模式
		{
			CAN_FilterId0  = thisid1>>8;
			CAN_FilterMaskId0 = thismask1>>8;
			CAN_FilterId1  = thisid1 ;
			CAN_FilterMaskId1 = thismask1 ;
			CAN_FilterId2  = thisid2>>8;
			CAN_FilterMaskId2 = thismask2>>8;
			CAN_FilterId3  = thisid2 ;
			CAN_FilterMaskId3 = thismask2 ;
		}
		else if(fcase == 1)// 1-单滤波器模式
		{
			CAN_FilterId0  = thisid>>21;
			CAN_FilterMaskId0 = thismask>>21;
			CAN_FilterId1  = thisid>>13 ;
			CAN_FilterMaskId1 = thismask>>13 ;
			CAN_FilterId2  = thisid>>5;
			CAN_FilterMaskId2 = thismask>>5;
			CAN_FilterId3  = thisid<<3 | rtr<<2;
			CAN_FilterMaskId3 = thismask<<3;
			CAN_FilterMaskId3 |= 0x03;
		}
	}

	/* 进入复位模式 */
	can_set_reset_mode(CANx);

	if(fcase == 1)// 1-单滤波器模式
	{
		/*单滤波模式 */
		CANx->MOD |= (unsigned char)CAN_Mode_AFM;
	}
	else if(fcase == 0)// 0- 双滤波器模式
	{
		/*双滤波模式 */
		CANx->MOD &=(~ (unsigned char) CAN_Mode_AFM);
	}

	CANx->IDE_RTR_DLC = CAN_FilterId0;
	CANx->ID[0] =  CAN_FilterId1;
	CANx->ID[1] =  CAN_FilterId2;
	CANx->ID[2] =  CAN_FilterId3;
	CANx->ID[3] =  CAN_FilterMaskId0;
	CANx->BUF[0] = CAN_FilterMaskId1;
	CANx->BUF[1] = CAN_FilterMaskId2;
	CANx->BUF[2] = CAN_FilterMaskId3;

	CanFilterInitValue[cap->id] = *CAN_FilterInitStruct;
	nCanFilterEnable[cap->id] = 1;

	/* 进入工作模式 */
	can_set_start(CANx);
}


int iCanWrite(CanPort_t *cap, CanMsg* messages, int nmsgs, TickType_t xBlockTime)
{
	TickType_t starttime = xTaskGetTickCount();
	CanMsg *msgs = messages;
	int count = 0;

	xSemaphoreTake(cap->xTxMutex, portMAX_DELAY);

	while (nmsgs) {
		portENTER_CRITICAL();
		if(!listLIST_IS_EMPTY(&cap->txFreeList)) {
			ListItem_t *item = listGET_HEAD_ENTRY(&cap->txFreeList);
			memcpy((void*)listGET_LIST_ITEM_VALUE(item), msgs, sizeof(CanMsg));
			uxListRemove(item);
			vListInsertEnd(&cap->txSendList, item);
			portEXIT_CRITICAL();
			msgs++;
			count++;
			nmsgs--;
			xSemaphoreGive(cap->xSend);
		} else {
			portEXIT_CRITICAL();
			if (xBlockTime && xTaskGetTickCount() - starttime > xBlockTime)
				break;
			vTaskDelay(1);
		}

		if (xBlockTime && xTaskGetTickCount() - starttime > xBlockTime)
			break;
	}

	xSemaphoreGive(cap->xTxMutex);

	return count;
}

int iCanRead(CanPort_t *cap, CanMsg* messages, int nmsgs, TickType_t xBlockTime)
{
	TickType_t starttime = xTaskGetTickCount();
	CanMsg *msgs = messages;
	int count = 0;

	xSemaphoreTake(cap->xRxMutex, portMAX_DELAY);
	while (nmsgs) {
		ListItem_t *item;

		xSemaphoreTake(cap->xRev, pdMS_TO_TICKS(10));
	
		portENTER_CRITICAL();
		if(!listLIST_IS_EMPTY(&cap->rxRevList)) {
			item = listGET_HEAD_ENTRY(&cap->rxRevList);
			memcpy(msgs, (void*)listGET_LIST_ITEM_VALUE(item), sizeof(CanMsg));
			uxListRemove(item);
			vListInsertEnd(&cap->rxFreeList, item);
			msgs++;
			count++;
			nmsgs--;
		}
		portEXIT_CRITICAL();

		if (xBlockTime && xTaskGetTickCount() - starttime > xBlockTime)
			break;
	}
	xSemaphoreGive(cap->xRxMutex);

	return count;

}

int iCanGetReceiveErrorCount(CanPort_t *cap)
{
	if (cap && cap->pcan)
		return cap->pcan->RXERR;

	return 0;
}

int iCanGetTransmitErrorCount(CanPort_t *cap)
{
	if (cap && cap->pcan)
		return cap->pcan->TXERR;

	return 0;
}

#ifdef CAN_USE_TX_DEMO
static void can_txdemo_thread(void *param)
{
	CanPort_t *cap = param;

	CanMsg txmsg = {0};
	txmsg.IDE = CAN_Id_Standard;
	txmsg.DLC = 4;
	txmsg.Data[0] = 0x11;
	txmsg.Data[1] = 0x22;
	txmsg.Data[2] = 0x33;
	txmsg.Data[3] = 0x44;

	//uint32_t last_time = get_timer(0);
	for (;;) {
		iCanWrite(cap, &txmsg, 1, 0);
		
		//printf("%d us.\n", get_timer(last_time));
		//last_time = get_timer(0);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
#endif

static void can_rxdemo_thread(void *param)
{
	CanPort_t *cap = param;

	for (;;) {
		CanMsg rxmsg[8] = {0};
		int revlen;
		int i, j;

		if ((revlen = iCanRead(cap, rxmsg, 8, pdMS_TO_TICKS(200))) > 0) {
			printf("can receive %d messages:\n", revlen);
			for (i = 0; i < revlen; i++) {
				for (j = 0; j < rxmsg[i].DLC; j++)
					printf("%.2x, ", rxmsg[i].Data[j]);
				printf("\n");
			}
		}
	}
}

int can_demo(void)
{
	CanPort_t *cap = xCanOpen(CAN_ID0);

	if (!cap) {
		printf("open can %d fail.\n", CAN_ID0);
		vTaskDelete(NULL);
		return -1;
	}

	vCanInit(cap, CAN250kBaud, CAN_MODE_NORMAL);
#if 1
	CAN_FilterInitTypeDef canfilter = {0};
	/* 只接收ID的第0位为1的帧 */
	canfilter.MODE = 1; /* 单滤波器模式 */
	canfilter.ID = 0x1;
	canfilter.IDMASK = 0x7fe;
	vCanSetFilter(cap, &canfilter);
#endif

	/* Create a task to test read can msg */
	if (xTaskCreate(can_rxdemo_thread, "canrx", configMINIMAL_STACK_SIZE, cap,
		configMAX_PRIORITIES / 3, NULL) != pdPASS) {
		printf("create can rxdemo task fail.\n");
		return -1;
	}

#ifdef CAN_USE_TX_DEMO
	/* Create a task to test write can msg */
	if (xTaskCreate(can_txdemo_thread, "cantx", configMINIMAL_STACK_SIZE, cap,
		configMAX_PRIORITIES / 3 + 1, NULL) != pdPASS) {
		printf("create can txdemo task fail.\n");
		return -1;
	}
#endif

	return 0;	
}

