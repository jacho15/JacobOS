#ifndef CPU_PORTS_H
#define CPU_PORTS_H

#include "cpu/types.h"

//read a byte from an i/o port
static inline u8 inb(u16 port) {
    u8 result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

//write a byte to an i/o port
static inline void outb(u16 port, u8 data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

//read a word from an i/o port
static inline u16 inw(u16 port) {
    u16 result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

//write a word to an i/o port
static inline void outw(u16 port, u16 data) {
    __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}

//small delay by writing to an unused port (used after PIC programming)
static inline void io_wait(void) {
    outb(0x80, 0);
}

//read `count` words from a port into a buffer (ATA PIO data transfer)
static inline void insw(u16 port, void *buf, u32 count) {
    __asm__ volatile ("rep insw" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}

//write `count` words from a buffer to a port
static inline void outsw(u16 port, const void *buf, u32 count) {
    __asm__ volatile ("rep outsw" : "+S"(buf), "+c"(count) : "d"(port));
}

#endif
