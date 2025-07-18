 //#define VECTOR_ENABLE	TRUE
 #define	VECTOR_ENABLE	FALSE

;Pre-defined run mode constants
USERMODE    		EQU 	0x10
FIQMODE     		EQU 	0x11
IRQMODE     		EQU 	0x12
SVCMODE     		EQU 	0x13
ABORTMODE   		EQU 	0x17
UNDEFMODE   		EQU 	0x1b
MODEMASK    		EQU 	0x1f
NOINT       		EQU 	0xc0


;Stacks defination for each run mode
_STACK_BASEADDRESS	EQU		0xC0008000

UserStackLen		EQU		0x10
SVCStackLen			EQU		0x2000
UndefStackLen		EQU		0x10
AbortStackLen		EQU		0x10
IRQStackLen			EQU		0x400
FIQStackLen			EQU		0x10

UserStackStart		EQU		(_STACK_BASEADDRESS)      
UndefStackStart		EQU		(UserStackStart - UserStackLen) 
AbortStackStart		EQU		(UndefStackStart - UndefStackLen) 
IRQStackStart		EQU		(AbortStackStart - AbortStackLen) 
FIQStackStart		EQU		(IRQStackStart - IRQStackLen) 
SVCStackStart		EQU		(FIQStackStart - FIQStackLen) 


VICL_BASE			EQU		0xE0C00000
VICH_BASE			EQU		0xE0B00000
ENABLE_REG_OFFSET	EQU		0x10
CLR_REG_OFFSET		EQU		0x14
ADDRESS_REG_OFFSET	EQU		0xF00
VICL_RAW_STATUS   	EQU		0xE0C00008    ;Interrupt raw status low 32 vector
VICH_RAW_STATUS   	EQU		0xE0B00008    ;Interrupt raw status high 32 vector
VICL_SEL_REG   		EQU		0xE0C0000C    ;Interrupt select reg for low 32 vector
VICH_SEL_REG   		EQU		0xE0B0000C    ;Interrupt select reg for high 32 vector
VICL_ENABLE			EQU		0xE0C00010    ;Interrupt Enable control low 32 vector
VICH_ENABLE			EQU		0xE0B00010    ;Interrupt Enable control high 32 vector
VICL_CLR			EQU		0xE0C00014    ;Interrupt clear control low 32 vector
VICH_CLR			EQU		0xE0B00014    ;Interrupt clear control high 32 vector
VICL_ADDR_BASE		EQU		0xE0C00100    
VICH_ADDR_BASE		EQU		0xE0B00100    
VICL_PROORITY_BASE	EQU		0xE0C00200
VICH_PROORITY_BASE	EQU		0xE0B00200

;ENABLE_VE			EQU		0x01000000	
VICL_ADDRESS 		EQU 	0xE0C00F00
VICH_ADDRESS 		EQU 	0xE0B00F00


WDT_CTL			EQU		0xE4B00000

ITCM0_BaseAddress		EQU		0xA0000000	;ITCM0 base : 0xA0000000, Size : 8KB,
ITCM1_BaseAddress		EQU		0xA0002000	;ITCM1 base : 0xA0002000, Size : 8KB,
DTCM0_BaseAddress		EQU		0xA0004000	;DTCM0 base : 0xA0004000, Size : 8KB,
DTCM1_BaseAddress		EQU		0xA0006000	;DTCM1 base : 0xA0006000, Size : 8KB,
