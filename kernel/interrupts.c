#include "interrupts.h"
#include "kernel.h"
#include "../include/screen.h"

// IDT table
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

// Exception messages
static const char* exception_messages[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception"
};

// IRQ handlers
static interrupt_handler_t irq_handlers[16];

// Initialize interrupt system
void interrupts_init(void) {
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Set up exception handlers
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    
    // Set up IRQ handlers
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
    
    // Set up system call handler
    idt_set_gate(SYSCALL_INT, (uint32_t)syscall_handler, 0x08, 0x8E);
    
    // Set up IDT pointer
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Load IDT
    __asm__ volatile("lidt %0" : : "m" (idt_ptr));
    
    // Initialize PIC
    // Remap IRQs
    outb(0x20, 0x11);   // Start initialization sequence
    outb(0xA0, 0x11);
    outb(0x20, 0x20);   // Map IRQs to 32-47
    outb(0xA0, 0x28);
    outb(0x20, 0x04);   // Tell PIC slave at IRQ2
    outb(0xA0, 0x02);
    outb(0x20, 0x01);   // 8086 mode
    outb(0xA0, 0x01);
    outb(0x20, 0x00);   // Enable interrupts
    outb(0xA0, 0x00);
}

// Set IDT gate
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

// Exception handler
void exception_handler(uint8_t exc_num) {
    screen_print("\n!!! KERNEL EXCEPTION !!!\n");
    
    if (exc_num < sizeof(exception_messages) / sizeof(char*)) {
        screen_print("Exception: ");
        screen_print(exception_messages[exc_num]);
        screen_print("\n");
    } else {
        screen_print("Unknown exception\n");
    }
    
    panic("Unrecoverable exception");
}

// IRQ handler
void irq_handler(uint8_t irq) {
    // Call registered handler if exists
    if (irq_handlers[irq]) {
        irq_handlers[irq]();
    }
    
    // Send EOI to PIC
    if (irq >= 8) {
        outb(0xA0, 0x20);  // Slave PIC
    }
    outb(0x20, 0x20);     // Master PIC
    
    // Schedule next process on timer interrupt
    if (irq == IRQ_TIMER) {
        process_schedule();
    }
}

// Register IRQ handler
void irq_register_handler(uint8_t irq, interrupt_handler_t handler) {
    irq_handlers[irq] = handler;
}

// I/O port functions
void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a" (value), "dN" (port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

// System call handler
void syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    switch (eax) {
        case SYS_EXIT:
            process_exit(ebx);
            break;
        case SYS_FORK:
            // Return child PID in EAX for parent, 0 for child
            eax = process_create();
            break;
        case SYS_READ:
            // Read from file descriptor EBX into buffer ECX, count EDX
            // eax = sys_read(ebx, (void*)ecx, edx);
            break;
        case SYS_WRITE:
            // Write to file descriptor EBX from buffer ECX, count EDX
            // eax = sys_write(ebx, (void*)ecx, edx);
            break;
        default:
            screen_print("Unknown system call: ");
            screen_print_hex(eax);
            screen_print("\n");
            break;
    }
}
