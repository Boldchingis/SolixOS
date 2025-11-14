; Interrupt Service Routines for SolixOS
; Handles CPU exceptions and hardware interrupts

[BITS 32]

; Macro to create interrupt service routine without error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0      ; Push dummy error code
    push %1     ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro to create interrupt service routine with error code
%macro ISR_ERR 1
global isr%1
isr%1:
    push %1     ; Push interrupt number (error code already pushed)
    jmp isr_common_stub
%endmacro

; Macro to create IRQ handler
%macro IRQ 2
global irq%1
irq%1:
    push 0      ; Push dummy error code
    push %2     ; Push interrupt number
    jmp irq_common_stub
%endmacro

; Exception handlers (no error code)
ISR_NOERR 0     ; Division by zero
ISR_NOERR 1     ; Debug
ISR_NOERR 2     ; Non-maskable interrupt
ISR_NOERR 3     ; Breakpoint
ISR_NOERR 4     ; Overflow
ISR_NOERR 5     ; Bound range exceeded
ISR_NOERR 6     ; Invalid opcode
ISR_NOERR 7     ; Device not available

; Exception handlers (with error code)
ISR_ERR 8       ; Double fault
ISR_NOERR 9     ; Coprocessor segment overrun
ISR_ERR 10      ; Invalid TSS
ISR_ERR 11      ; Segment not present
ISR_ERR 12      ; Stack-segment fault
ISR_ERR 13      ; General protection fault
ISR_ERR 14      ; Page fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; x87 FPU error
ISR_ERR 17      ; Alignment check
ISR_NOERR 18    ; Machine check
ISR_NOERR 19    ; SIMD floating-point exception

; IRQ handlers
IRQ 0, 32       ; Timer
IRQ 1, 33       ; Keyboard
IRQ 2, 34       ; Cascade
IRQ 3, 35       ; COM2/COM4
IRQ 4, 36       ; COM1/COM3
IRQ 5, 37       ; LPT2
IRQ 6, 38       ; Floppy
IRQ 7, 39       ; LPT1
IRQ 8, 40       ; RTC
IRQ 9, 41       ; Free
IRQ 10, 42      ; Free
IRQ 11, 43      ; Free
IRQ 12, 44      ; Mouse
IRQ 13, 45      ; FPU
IRQ 14, 46      ; ATA1
IRQ 15, 47      ; ATA2

; Common interrupt handler stub
extern exception_handler
extern irq_handler

isr_common_stub:
    ; Save registers
    pusha
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call exception handler
    call exception_handler
    
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret

irq_common_stub:
    ; Save registers
    pusha
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call IRQ handler
    call irq_handler
    
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret
