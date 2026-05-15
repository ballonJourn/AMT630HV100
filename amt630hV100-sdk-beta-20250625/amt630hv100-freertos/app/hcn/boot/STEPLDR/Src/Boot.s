
        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION IRQ_STACK:DATA:NOROOT(3)
        SECTION FIQ_STACK:DATA:NOROOT(3)
        SECTION SVC_STACK:DATA:NOROOT(3)
        SECTION ABT_STACK:DATA:NOROOT(3)
        SECTION UND_STACK:DATA:NOROOT(3)
        SECTION CSTACK:DATA:NOROOT(3)

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#define __ASSEMBLY__

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------
#define IRAM_BASE	0x300000
#define SYS_CPU_CTL	0xe4900208	
#define AIC         0xFFFFF000
#define AIC_IVR     0x10
#define AIC_EOICR   0x38

#define ARM_MODE_ABT     0x17
#define ARM_MODE_FIQ     0x11
#define ARM_MODE_IRQ     0x12
#define ARM_MODE_SVC     0x13
#define ARM_MODE_SYS     0x1F

#define I_BIT            0x80
#define F_BIT            0x40

//------------------------------------------------------------------------------
//         Startup routine
//------------------------------------------------------------------------------

/*
   Exception vectors
 */
//        SECTION .vectors:CODE:NOROOT(2)
		SECTION .intvec:CODE:NOROOT(2)
        PUBLIC  __vector
		PUBLIC  __iar_program_start
		  
		ARM	; Always ARM mode after reset
		
__vector: 
		ldr  	pc,	Reset
		LDR     PC, Undefined_Addr		
		LDR     PC, SWI_Addr
		LDR     PC, Prefetch_Addr
		LDR     PC, Abort_Addr
		NOP     							; Reserved vector
		LDR     PC, IRQ_Addr
		LDR     PC, FIQ_Addr

        IMPORT  undef_handler
        IMPORT  swi_handler
        IMPORT  prefetch_handler
        IMPORT  data_abort_handler
        IMPORT  AIC_IrqHandler
        IMPORT  fiq_handler

Reset:           dc32	__iar_program_start
Undefined_Addr:  dc32	undef_handler		;Undefined_Handler
SWI_Addr:        dc32	swi_handler			;SWI_Handler
Prefetch_Addr:   dc32	prefetch_handler	;ExceptionPAB
Abort_Addr:      dc32	data_abort_handler	;ExceptionDAB
Reserved_Addr:   dc32 	0					;ExceptionREV
IRQ_Addr:        dc32	irqHandler
FIQ_Addr:        dc32	fiq_handler

MODE_MSK DEFINE 0x1F            ; Bit mask for mode bits in CPSR

USR_MODE DEFINE 0x10            ; User mode
FIQ_MODE DEFINE 0x11            ; Fast Interrupt Request mode
IRQ_MODE DEFINE 0x12            ; Interrupt Request mode
SVC_MODE DEFINE 0x13            ; Supervisor mode
ABT_MODE DEFINE 0x17            ; Abort mode
UND_MODE DEFINE 0x1B            ; Undefined Instruction mode
SYS_MODE DEFINE 0x1F            ; System mode

CP_DIS_MASK         DEFINE  0xFFFFEFFA

		SECTION .text:CODE:NOROOT(2)
		EXTERN	?main
		REQUIRE __vector

__iar_program_start:
        b       reset_handler
//        DCD     0x424b5241
//        DCD     0
//        DCD     0

reset_handler:
;==================================================================
; Reset registers
;==================================================================
		MOV		r2, #0
		MOV		r3, #0
		MOV		r4, #0
		MOV		r5, #0
		MOV		r6, #0
		MOV		r7, #0
		MOV		r8, #0
		MOV		r9, #0
		MOV		r10, #0
		MOV		r11, #0
		MOV		r12, #0

;==================================================================
; Disable caches, MMU and branch prediction in case they were left enabled from an earlier run
; This does not need to be done from a cold reset
;==================================================================
        MRC     p15, 0, r0, c1, c0, 0       ; Read CP15 System Control register
        BIC     r0, r0, #(0x1 << 12)        ; Clear I bit 12 to disable I Cache
        ;ORR     r0, r0, #(0x1 << 12)        ; Set I bit 12 to enable I Cache
        BIC     r0, r0, #(0x1 << 2)         ; Clear C bit  2 to disable D Cache
        BIC     r0, r0, #0x1                ; Clear M bit  0 to disable MMU
        BIC     r0, r0, #(0x1 << 11)        ; Clear Z bit 11 to disable branch prediction
        MCR     p15, 0, r0, c1, c0, 0       ; Write value back to CP15 System Control register
		
;==================================================================
; Cache Invalidation code for Cortex-A7
; NOTE: Neither Caches, nor MMU, nor BTB need post-reset invalidation on Cortex-A7,
; but forcing a cache invalidation, makes the code more portable to other CPUs (e.g. Cortex-A9)
;==================================================================     
        ; Invalidate L1 Instruction Cache
        MRC     p15, 1, r0, c0, c0, 1      ; Read Cache Level ID Register (CLIDR)
        TST		r0, #0x3                   ; Harvard Cache?
        MOV     r0, #0                     ; SBZ
        MCRNE   p15, 0, r0, c7, c5, 0      ; ICIALLU - Invalidate instruction cache and flush branch target cache
        
        ; Invalidate Data/Unified Caches
        
        MRC     p15, 1, r0, c0, c0, 1      ; Read CLIDR
        ANDS    r3, r0, #0x07000000        ; Extract coherency level
        MOV     r3, r3, LSR #23            ; Total cache levels << 1
        BEQ     Finished                   ; If 0, no need to clean
    
        MOV     r10, #0                    ; R10 holds current cache level << 1
Loop1   ADD     r2, r10, r10, LSR #1       ; R2 holds cache "Set" position 
        MOV     r1, r0, LSR r2             ; Bottom 3 bits are the Cache-type for this level
        AND     r1, r1, #7                 ; Isolate those lower 3 bits
        CMP     r1, #2
        BLT     Skip                       ; No cache or only instruction cache at this level
        
        MCR     p15, 2, r10, c0, c0, 0     ; Write the Cache Size selection register
        ISB                                ; ISB to sync the change to the CacheSizeID reg
        MRC     p15, 1, r1, c0, c0, 0      ; Reads current Cache Size ID register
        AND     r2, r1, #7                 ; Extract the line length field
        ADD     r2, r2, #4                 ; Add 4 for the line length offset (log2 16 bytes)
        LDR     r4, =0x3FF
        ANDS    r4, r4, r1, LSR #3         ; R4 is the max number on the way size (right aligned)
        CLZ     r5, r4                     ; R5 is the bit position of the way size increment
        LDR     r7, =0x7FFF
        ANDS    r7, r7, r1, LSR #13        ; R7 is the max number of the index size (right aligned)

Loop2   MOV     r9, r4                     ; R9 working copy of the max way size (right aligned)

Loop3   ORR     r11, r10, r9, LSL r5       ; Factor in the Way number and cache number into R11
        ORR     r11, r11, r7, LSL r2       ; Factor in the Set number
        MCR     p15, 0, r11, c7, c6, 2     ; Invalidate by Set/Way
        SUBS    r9, r9, #1                 ; Decrement the Way number
        BGE     Loop3
        SUBS    r7, r7, #1                 ; Decrement the Set number
        BGE     Loop2
Skip    ADD     r10, r10, #2               ; increment the cache number
        CMP     r3, r10
        BGT     Loop1

Finished
		
;==================================================================
; Invalidate TLB
;==================================================================	
		MOV		r0, #0
		MCR 	p15, 0, r0, c8, c7, 0

;==================================================================
; Branch Prediction Enable
;==================================================================
		;MOV r1, #0
		;MRC p15, 0, r1, c1, c0, 0 /* Read Control Register configuration data */
		;ORR r1, r1, #(0x1 << 11) /* Global BP Enable bit */
		;MCR p15, 0, r1, c1, c0, 0 /* Write Control Register configuration data */

; Initialize the stack pointers.
; The pattern below can be used for any of the exception stacks:
; FIQ, IRQ, SVC, ABT, UND, SYS.
; The USR mode uses the same stack as SYS.
; The stack segments must be defined in the linker command file,
; and be declared above.
        mrs     r0,cpsr                             ; Original PSR value
        bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
        orr     r0,r0,#SVC_MODE                     ; Set Supervisor mode bits
        msr     cpsr_c,r0                           ; Change the mode
        ldr     sp,=SFE(SVC_STACK)                  ; End of SVC_STACK

        bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
        orr     r0,r0,#ABT_MODE                     ; Set Abort mode bits
        msr     cpsr_c,r0                           ; Change the mode
        ldr     sp,=SFE(ABT_STACK)                  ; End of ABT_STACK

        bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
        orr     r0,r0,#UND_MODE                     ; Set Undefined mode bits
        msr     cpsr_c,r0                           ; Change the mode
        ldr     sp,=SFE(UND_STACK)                  ; End of UND_STACK

        bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
        orr     r0,r0,#FIQ_MODE                     ; Set FIR mode bits
        msr     cpsr_c,r0                           ; Change the mode
        ldr     sp,=SFE(FIQ_STACK)                  ; End of FIQ_STACK

        bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
        orr     r0,r0,#IRQ_MODE                     ; Set IRQ mode bits
        msr     cpsr_c,r0                           ; Change the mode
        ldr     sp,=SFE(IRQ_STACK)                  ; End of IRQ_STACK

        bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
        orr     r0,r0,#SYS_MODE                     ; Set System mode bits
        msr     cpsr_c,r0                           ; Change the mode
        ldr     sp,=SFE(CSTACK)                     ; End of CSTACK
		  
        /* Branch to main() */
        LDR     r0, =?main
        BLX     r0

        /* Loop indefinitely when program is finished */
loop4:
        B       loop4
		
		
/*
   Handles incoming interrupt requests by branching to the corresponding
   handler, as defined in the AIC. Supports interrupt nesting.
 */
 
irqHandler:
		NOP
		NOP
		;/* IRQ entry {{{ */           /* save r0 in r13_IRQ */
		SUB     lr,lr,#4            /* put return address in r0_SYS */
		STMFD   sp!,{lr}              /* save r1 in r14_IRQ (lr) */
		MRS     lr,spsr             /* put the SPSR in r1_SYS */
		STMFD   sp!,{r0,lr}
		ldr 	r0, = AIC_IrqHandler
		;   ldr 	r0, [r0];
		MSR     cpsr_c,#(ARM_MODE_SYS | I_BIT) /* SYSTEM, no IRQ, but FIQ enabled! */       /* save SPSR and PC on SYS stack */
		// mcr	p15, 0, r0, c7, c10, 5	
		STMFD   sp!, {r1-r3, r4, r12, lr} /* save all other regs on SYS stack */
		AND     r1, sp, #4
		SUB     sp, sp, r1
		STMFD   sp!, {r1, lr}
		BLX     r0
		LDMIA   sp!, {r1, lr}
		ADD     sp, sp, r1
		LDMIA   sp!, {r1-r3, r4, r12, lr}
		MSR     CPSR_c, #ARM_MODE_IRQ | I_BIT	
		LDMIA   sp!, {r0, lr}
		MSR     SPSR_cxsf, lr
		LDMIA   sp!, {pc}^	


        END
		  