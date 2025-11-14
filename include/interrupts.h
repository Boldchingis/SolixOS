#ifndef SOLIX_INTERRUPTS_H
#define SOLIX_INTERRUPTS_H

#include "types.h"

// Interrupt descriptor table entry
typedef struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// Interrupt handler function type
typedef void (*interrupt_handler_t)(void);

// Exception numbers
#define EXC_DIVIDE_BY_ZERO 0
#define EXC_DEBUG 1
#define EXC_NMI 2
#define EXC_BREAKPOINT 3
#define EXC_OVERFLOW 4
#define EXC_BOUND_RANGE 5
#define EXC_INVALID_OPCODE 6
#define EXC_DEVICE_NOT_AVAIL 7
#define EXC_DOUBLE_FAULT 8
#define EXC_INVALID_TSS 10
#define EXC_SEGMENT_NOT_PRESENT 11
#define EXC_STACK_FAULT 12
#define EXC_GENERAL_PROTECTION 13
#define EXC_PAGE_FAULT 14
#define EXC_X87_FPU 16
#define EXC_ALIGNMENT_CHECK 17
#define EXC_MACHINE_CHECK 18
#define EXC_SIMD_FP 19

// IRQ numbers
#define IRQ_TIMER 0
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE 2
#define IRQ_COM2 3
#define IRQ_COM1 4
#define IRQ_LPT2 5
#define IRQ_FLOPPY 6
#define IRQ_LPT1 7
#define IRQ_RTC 8
#define IRQ_FREE1 9
#define IRQ_FREE2 10
#define IRQ_MOUSE 12
#define IRQ_FPU 13
#define IRQ_ATA1 14
#define IRQ_ATA2 15

// System call interrupt
#define SYSCALL_INT 0x80

// Functions
void interrupts_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void irq_handler(uint8_t irq);
void exception_handler(uint8_t exc_num);

// Assembly interrupt handlers
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

#endif
