;********************************************************************
;*          crt0_arm.s                                              *
;*            Startup Routine (ARMASM)		                    *
;*                                                                  *
;*          Copyright (C) 1999-2001 NINTENDO Co.,Ltd.               *
;********************************************************************
    INCLUDE     AgbDefineArm.s
    INCLUDE     AgbMemoryMapArm.s
    INCLUDE     AgbSyscallDefineArm.s
    INCLUDE     AgbMacroArm.s

    IMPORT      __main
	EXPORT     	__FarFunction,__FarProcedure,version
;--------------------------------------------------------------------
;-                      Reset                                    -
;--------------------------------------------------------------------
	AREA        Init, CODE, READONLY
		
	ENTRY
	
start
	INCLUDE     rom_header_arm.s
	CODE32
TstNDSorGBA
	mov		r0,#0x2000000
	mov		r1,#0x120
	strh		r1,[r0]
	mov		r2,#0x2200000
	mov		r3,#0x340
	strh		r3,[r2]
	ldrh		r1,[r0]
	ldrh		r3,[r2]
	cmp		r1,r3
	ldr		r0,=0x80fd100        ;nds loader所在位置
	bxne	r0
start_vector
	mov	r0,#PSR_IRQ_MODE	; 准备切换到IRQ管理模式
	msr     cpsr_c,r0		; 切换到IRQ管理模式
    ldr     sp, sp_irq              ; 设置堆栈,为IRQ中断作准备
        
    mov	r0,#PSR_SYS_MODE	; 准备切换到系统模式
	msr     cpsr_c,r0		; 切换到系统模式
	ldr     sp, sp_usr              ; 设置堆栈用户模式


    ldr     r1, =INTR_VECTOR_BUF    ; 设置中断向量缓冲
    adr     r0, intr_main		; 设置中断控制函数
    str     r0, [r1]
    ldr     r1, =__main            ; 进入C语言主循环体
    mov     lr, pc
    bx      r1
    
    b       start_vector            ; 程序执行完毕后重启

    ALIGN
sp_usr  DCD     WRAM_END - 0x100
sp_irq  DCD     WRAM_END - 0x60

    IMPORT      IntrTable		;中断向量表
    EXPORT      intr_main		;中断控制
    
    ALIGN
    CODE32
intr_main
        mov     r3, #0x4000000				; Check IE/IF
        add     r3, r3, #0x200				; r3: REG_IE
        ldr     r2, [r3]
        and     r1, r2, r2, lsr #16 		; r1: IE & IF
        ands    r0, r1, #0x2000  		; Game Pak Interrupt
loop    bne     loop
        mov     r2, #0
        ands    r0, r1, #0x0001   		; V-blank Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0002   ; H-blank Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0004   ; V-counter Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0008    ; Timer 0 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0010    ; Timer 1 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0020    ; Timer 2 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0040    ; Timer 3 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0080       ; Serial Communication Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0100      ; DMA0 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0200      ; DMA1 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0400      ; DMA2 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x0800      ; DMA3 Interrupt
        bne     jump_intr
        add     r2, r2, #4
        ands    r0, r1, #0x1000       ; Key Interrupt

jump_intr
        strh    r0, [r3, #2]            ; IF Clear           11c
        ldr     r1, =IntrTable          ; Jump to user IRQ process
        add     r1, r1, r2
	ldr     r0, [r1]
        bx      r0

    CODE16
__FarFunction
    CODE16
__FarProcedure
        bx      r0
        nop
        nop          ; This nop is here to allow unmapped memory to be used as

version DCD     0x5AA51720

    END
