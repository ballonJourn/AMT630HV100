#ifndef UART_PRINT_H__
#define UART_PRINT_H__

/* UART Register Offsets. */
#define UART_DR		0x00	/* Data read or written from the interface. */
#define UART_RSR	0x04	/* Receive status register (Read). */
#define UART_FR		0x18	/* Flag register (Read only). */
#define UART_ILPR	0x20	/* IrDA low power counter register. */
#define UART_IBRD	0x24	/* Integer baud rate divisor register. */
#define UART_FBRD	0x28	/* Fractional baud rate divisor register. */
#define UART_LCRH	0x2c	/* Line control register. */
#define UART_CR		0x30	/* Control register. */
#define UART_IFLS	0x34	/* Interrupt fifo level select. */
#define UART_IMSC	0x38	/* Interrupt mask. */
#define UART_RIS	0x3c	/* Raw interrupt status. */
#define UART_MIS	0x40	/* Masked interrupt status. */
#define UART_ICR	0x44	/* Interrupt clear register. */
#define UART_DMACR	0x48	/* DMA control register. */


#define UART_DR_OE		(1 << 11)
#define UART_DR_BE		(1 << 10)
#define UART_DR_PE		(1 << 9)
#define UART_DR_FE		(1 << 8)

#define UART_RSR_OE 		0x08
#define UART_RSR_BE 		0x04
#define UART_RSR_PE 		0x02
#define UART_RSR_FE 		0x01

#define UART_FR_RI			0x100
#define UART_FR_TXFE		0x080
#define UART_FR_RXFF		0x040
#define UART_FR_TXFF		0x020
#define UART_FR_RXFE		0x010
#define UART_FR_BUSY		0x008
#define UART_FR_DCD 		0x004
#define UART_FR_DSR 		0x002
#define UART_FR_CTS 		0x001
#define UART_FR_TMSK		(UART_FR_TXFF + UART_FR_BUSY)

#define UART_CR_CTSEN		0x8000	/* CTS hardware flow control */
#define UART_CR_RTSEN		0x4000	/* RTS hardware flow control */
#define UART_CR_OUT2		0x2000	/* OUT2 */
#define UART_CR_OUT1		0x1000	/* OUT1 */
#define UART_CR_RTS			0x0800	/* RTS */
#define UART_CR_DTR			0x0400	/* DTR */
#define UART_CR_RXE			0x0200	/* receive enable */
#define UART_CR_TXE			0x0100	/* transmit enable */
#define UART_CR_LBE			0x0080	/* loopback enable */
#define UART_CR_RTIE		0x0040
#define UART_CR_TIE 		0x0020
#define UART_CR_RIE 		0x0010
#define UART_CR_MSIE		0x0008
#define UART_CR_IIRLP		0x0004	/* SIR low power mode */
#define UART_CR_SIREN		0x0002	/* SIR enable */
#define UART_CR_UARTEN		0x0001	/* UART enable */
 
#define UART_LCRH_SPS		0x80
#define UART_LCRH_WLEN_8	0x60
#define UART_LCRH_WLEN_7	0x40
#define UART_LCRH_WLEN_6	0x20
#define UART_LCRH_WLEN_5	0x00
#define UART_LCRH_FEN		0x10
#define UART_LCRH_STP2		0x08
#define UART_LCRH_EPS		0x04
#define UART_LCRH_PEN		0x02
#define UART_LCRH_BRK		0x01


#define UART_IFLS_RX1_8	(0 << 3)
#define UART_IFLS_RX2_8	(1 << 3)
#define UART_IFLS_RX4_8	(2 << 3)
#define UART_IFLS_RX6_8	(3 << 3)
#define UART_IFLS_RX7_8	(4 << 3)
#define UART_IFLS_TX1_8	(0 << 0)
#define UART_IFLS_TX2_8	(1 << 0)
#define UART_IFLS_TX4_8	(2 << 0)
#define UART_IFLS_TX6_8	(3 << 0)
#define UART_IFLS_TX7_8	(4 << 0)

#define UART_OEIM		(1 << 10)	/* overrun error interrupt mask */
#define UART_BEIM		(1 << 9)	/* break error interrupt mask */
#define UART_PEIM		(1 << 8)	/* parity error interrupt mask */
#define UART_FEIM		(1 << 7)	/* framing error interrupt mask */
#define UART_RTIM		(1 << 6)	/* receive timeout interrupt mask */
#define UART_TXIM		(1 << 5)	/* transmit interrupt mask */
#define UART_RXIM		(1 << 4)	/* receive interrupt mask */
#define UART_DSRMIM		(1 << 3)	/* DSR interrupt mask */
#define UART_DCDMIM		(1 << 2)	/* DCD interrupt mask */
#define UART_CTSMIM		(1 << 1)	/* CTS interrupt mask */
#define UART_RIMIM		(1 << 0)	/* RI interrupt mask */

#define UART_OEIS		(1 << 10)	/* overrun error interrupt status */
#define UART_BEIS		(1 << 9)	/* break error interrupt status */
#define UART_PEIS		(1 << 8)	/* parity error interrupt status */
#define UART_FEIS		(1 << 7)	/* framing error interrupt status */
#define UART_RTIS		(1 << 6)	/* receive timeout interrupt status */
#define UART_TXIS		(1 << 5)	/* transmit interrupt status */
#define UART_RXIS		(1 << 4)	/* receive interrupt status */
#define UART_DSRMIS		(1 << 3)	/* DSR interrupt status */
#define UART_DCDMIS		(1 << 2)	/* DCD interrupt status */
#define UART_CTSMIS		(1 << 1)	/* CTS interrupt status */
#define UART_RIMIS		(1 << 0)	/* RI interrupt status */



#define UART485_RBR_THR_DLL              0x000
#define UART485_DLH_IER                  0x004
#define UART485_IIR_FCR                  0x008
#define UART485_RBR                      0x000
#define UART485_THR                      0x000
#define UART485_DLL                      0x000
#define UART485_DLH                      0x004
#define UART485_IER                      0x004
#define UART485_IIR                      0x008
#define UART485_FCR                      0x008
#define UART485_LCR                      0x00C
#define UART485_MCR                      0x010
#define UART485_LSR                      0x014
#define UART485_MSR                      0x018
#define UART485_SCR                      0x01C
#define UART485_LPDLL                    0x020
#define UART485_LPDLH                    0x024
#define UART485_RESERVED                 0x028
#define UART485_SRBR                     0x030
#define UART485_STHR                     0x06C
#define UART485_FAR                      0x070
#define UART485_TFR                      0x074
#define UART485_RFW                      0x078
#define UART485_USR                      0x07C
#define UART485_TFL                      0x080
#define UART485_RFL                      0x084
#define UART485_SRR                      0x088
#define UART485_SRTS                     0x08C
#define UART485_SBCR                     0x090
#define UART485_SDMAM                    0x094
#define UART485_SFE                      0x098
#define UART485_SRT                      0x09C
#define UART485_STET                     0x0A0
#define UART485_HTX                      0x0A4
#define UART485_DMASA                    0x0A8
#define UART485_TCR                      0x0AC
#define UART485_DE_EN                    0x0B0
#define UART485_RE_EN                    0x0B4
#define UART485_DET                      0x0B8
#define UART485_TAT                      0x0BC
#define UART485_DLF                      0x0C0
#define UART485_RAR                      0x0C4
#define UART485_TAR                      0x0C8
#define UART485_LCR_EXT                  0x0CC
#define UART485_CPR                      0x0F4
#define UART485_UCV                      0x0F8
#define UART485_CTR                      0x0FC

#define UART485_LSR_RFE					(1 << 7)
#define UART485_LSR_TEMT				(1 << 6)
#define UART485_LSR_THRE				(1 << 5)
#define UART485_LSR_BI					(1 << 4)
#define UART485_LSR_FE					(1 << 3)
#define UART485_LSR_PE					(1 << 2)
#define UART485_LSR_OE					(1 << 1)
#define UART485_LSR_DR					(1 << 0)

#define UART485_IIR_IID_MASK			(0xf << 0)
#define UART485_IIR_IID_MODEM_STATUS		0x0
#define UART485_IIR_IID_NO_INT_PENDING		0x1
#define UART485_IIR_IID_THR_EMPTY			0x2
#define UART485_IIR_IID_REV_DATA_AVAIL		0x4
#define UART485_IIR_IID_REV_LINE_STATUS		0x6
#define UART485_IIR_IID_BUSY_DETECT			0x7
#define UART485_IIR_IID_CHAR_TIMEOUT		0xc



#define CSIZE	0x0003
#define   CS8	0x0000
#define   CS6	0x0001
#define   CS7	0x0002
#define   CS5	0x0003
#define CSTOPB	0x0004
#define PARENB	0x0010
#define PARODD	0x0020
#define CMSPAR	0x0100	/* mark or space (stick) parity */
#define CRTSCTS	0x0200	/* flow control */

typedef enum {
	UART_ID0 = 0,
	UART_ID1,
	UART_ID2,
	UART_ID3,
	UART_NUM,
} eUartID;

void InitUart(unsigned int baud);
void SendUartString(char * buf);
void SendUartChar(char ch);
void PrintVariableValueHex(char * variable, unsigned int value);
void SendUartWord(unsigned int data);
void IntToStr(unsigned int value, char *str);
void uart_init(int id, unsigned int baud, unsigned int flags);
void updateFromUart(int id);

#endif

