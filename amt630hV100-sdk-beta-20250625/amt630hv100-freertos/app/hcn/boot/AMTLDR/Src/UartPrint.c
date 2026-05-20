#include "amt630h.h"
#include "typedef.h"
#include "UartPrint.h"

void InitUart(unsigned int baud)
{
	unsigned int Baud_Rate_Divisor;
	unsigned int val;

    //select pad
   val = rSYS_PAD_CTRL02;
   val &= ~(0xF<<12);
   val |=(0x1<<14)|(0x1<<12);
   rSYS_PAD_CTRL02 = val;
	
	Baud_Rate_Divisor = ((CLK_24MHZ<<3) + baud)/(baud<<1);
	rUART_IBRD = Baud_Rate_Divisor >> 6;
	rUART_FBRD = Baud_Rate_Divisor & 0x3f;
	
	rUART_LCR_H = 0x70;//data len:8 bit,parity checking disable
	rUART_IFLS = 0x19;
	rUART_CR = 0x301;
}

static char HexToChar(unsigned char value)
{
	value &= 0x0f;

	if ( value < 10 )
		return(0x30 + value);
	else
		return(0x60 + value - 9);
}


static void ShortToStr(unsigned short value, char *str)
{
	str[0] = HexToChar(value >> 12);
	str[1] = HexToChar((value >> 8) & 0x0f);
	str[2] = HexToChar((value >> 4) & 0x0f);
	str[3] = HexToChar(value & 0x0f);
	str[4] = 0;
}

void IntToStr(unsigned int value, char *str)
{
	ShortToStr(value >> 16, str);
	ShortToStr(value & 0xffff, str + 4);
	str[8] = 0;
}

void  SendUartString(char * buf)
{
	int i = 0;	
	while ( buf[i] != 0)
	{
		while ( !(rUART_FR & 0x20) )
		{
			rUART_DR = buf[i++];
			if ( buf[i] == 0 )
				return;
		}
		while ( (rUART_FR & 0x20));
	}
}

void SendUartChar(char ch)
{	
	while ( (rUART_FR & 0x20) );
	rUART_DR = ch;
}


void PrintVariableValueHex(char * variable, unsigned int value)
{
	char buf[10];

	SendUartString(variable);
	SendUartString(": 0x");
	IntToStr(value, buf);
	SendUartString(buf);
	SendUartString("\r\n");
}

void SendUartWord(unsigned int data)
{
    char buf[10];
    
    SendUartString("0x");
	IntToStr(data, buf);
	SendUartString(buf);
	SendUartString("\r\n");
}
void uart_puts(const UINT8*buf)
{
	INT32 i = 0;
	while ( buf[i] != 0)
	{
		while( !(rUART_FR & 0x20) )
		{
			if ( buf[i] == '\n' )
			{
				rUART_DR = '\r';
				while( (rUART_FR & 0x20) );
			}

			rUART_DR = buf[i++];
			if ( buf[i] == 0 )
				return;
		}
		while( (rUART_FR & 0x20) );
	}
}
