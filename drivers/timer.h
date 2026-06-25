#ifndef DRIVERS_TIMER_H
#define DRIVERS_TIMER_H

#include "cpu/types.h"

//programmable interval timer on IRQ0. drives preemptive scheduling.
void init_timer(u32 freq_hz);
u32  timer_ticks(void);

#endif
