#ifndef SOLIX_TIMER_H
#define SOLIX_TIMER_H

#include "types.h"

// Timer frequency
#define TIMER_FREQUENCY 100    // 100 Hz (10ms intervals)

// Timer functions
void timer_init(void);
void timer_handler(void);
uint32_t timer_get_ticks(void);
void timer_wait(uint32_t ticks);

#endif
