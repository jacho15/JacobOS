#include "drivers/serial.h"
#include "cpu/ports.h"

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00); //disable interrupts
    outb(COM1 + 3, 0x80); //enable DLAB (set baud divisor)
    outb(COM1 + 0, 0x03); //divisor lo: 38400 baud
    outb(COM1 + 1, 0x00); //divisor hi
    outb(COM1 + 3, 0x03); //8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); //enable FIFO, clear, 14-byte threshold
    outb(COM1 + 4, 0x0B); //IRQs enabled, RTS/DSR set
}

static int tx_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    //bounded wait: if no serial backend ever drains the port (e.g. in a
    //browser emulator with no COM device) we must not lock up the kernel
    int spin = 100000;
    while (!tx_empty() && spin-- > 0) ;
    outb(COM1, (u8)c);
}

void serial_print(const char *s) {
    while (*s) serial_putc(*s++);
}
