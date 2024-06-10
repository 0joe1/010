#ifndef TIMER_H
#define TIMER_H
#include "stdint.h"

#define WORK_FREQUENCY 1193180
#define IRQ0_SIG_FREQUENCY 100
#define COUNTER0_COUNT_NUM (WORK_FREQUENCY/IRQ0_SIG_FREQUENCY)
#define PIT_CONTROL_PORT 0x43
#define COUNTER0_NO 0
#define COUNTER0_PORT 0x40
#define COUNTER_MODE2 2
#define RW_LH 3

void timer_init(void);
void sleep_ms(uint_32 ms);

#endif
