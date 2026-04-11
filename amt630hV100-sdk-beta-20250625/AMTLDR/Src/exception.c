#include "UartPrint.h"

void undef_handler()
{
	SendUartString("\r\nUndef EXC");
	while(1);
}

void prefetch_handler()
{
	SendUartString("\r\nPrefetch EXC");
	while(1);
}

void data_abort_handler()
{
	SendUartString("\r\nData abort EXC");
	while(1);
}

void irq_handler()
{
	SendUartString("\r\nIRQ EXC");
	while(1);
}

void fiq_handler()
{
	SendUartString("\r\nFIQ EXC");
	while(1);
}

void swi_handler()
{
	SendUartString("\r\nSWI EXC");
	while(1);
}

