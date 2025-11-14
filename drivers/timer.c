#include "timer.h"
#include "interrupts.h"
#include "screen.h"

// Timer state
static volatile uint32_t timer_ticks = 0;

// Initialize timer (PIT)
void timer_init(void) {
    // Register timer interrupt handler
    irq_register_handler(IRQ_TIMER, timer_handler);
    
    // Set timer frequency
    uint32_t divisor = 1193180 / TIMER_FREQUENCY;
    
    // Send command to PIT
    outb(0x43, 0x36);
    
    // Send divisor
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    
    timer_ticks = 0;
}

// Timer interrupt handler
void timer_handler(void) {
    timer_ticks++;
}

// Get current tick count
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// Wait for specified number of ticks
void timer_wait(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while (timer_ticks - start < ticks) {
        __asm__ volatile("hlt");
    }
}
