#ifndef _UART_H
#define _UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#include "circ_buf.h"

typedef enum {
	UART_ID0 = 0,
	UART_ID1,
	UART_ID2,
	UART_ID3,
	UART_NUM,
} eUartID;

typedef struct {
	uint32_t id;
	uint32_t regbase;
	int fifosize;
	struct circ_buf rxbuf;
	struct circ_buf txbuf;
	SemaphoreHandle_t xMutex;
	SemaphoreHandle_t xRev;
	SemaphoreHandle_t xSend;
	uint32_t hwFlowCtl;
}UartPort_t;

extern void vDebugConsoleInitialise(void);
extern UartPort_t *xUartOpen(uint32_t id);
extern void vUartInit(UartPort_t *uap, uint32_t baud, uint32_t flags);
extern void vUartClose(UartPort_t *uap);
extern int iUartWrite(UartPort_t *uap, uint8_t *buf, size_t len, TickType_t xBlockTime);
int iUartRead(UartPort_t *uap, uint8_t *buf, size_t len, TickType_t xBlockTime);

int uart_rx_demo(void);

#ifdef __cplusplus
}
#endif

#endif
