#include "drivers/timer.h"
#include "cpu/isr.h"
#include "cpu/ports.h"
#include "kernel/task.h"

#define PIT_FREQ    1193182u
#define PIT_CMD     0x43
#define PIT_CH0     0x40

static volatile u32 ticks;

static void timer_callback(registers_t *r) {
    (void)r;
    ticks++;
    //preemptive round-robin: hand the CPU to the next task every tick
    schedule();
}

u32 timer_ticks(void) { return ticks; }

void init_timer(u32 freq_hz) {
    u32 divisor = PIT_FREQ / freq_hz;
    outb(PIT_CMD, 0x36);                       //channel 0, lo/hi, mode 3
    outb(PIT_CH0, (u8)(divisor & 0xFF));
    outb(PIT_CH0, (u8)((divisor >> 8) & 0xFF));
    register_interrupt_handler(IRQ0, timer_callback);
}
