#ifndef UART_PRINT_H__
#define UART_PRINT_H__

void InitUart(unsigned int baud);
void SendUartString(char * buf);
void SendUartChar(char ch);
void PrintVariableValueHex(char * variable, unsigned int value);
void SendUartWord(unsigned int data);
void IntToStr(unsigned int value, char *str);

#endif

