#ifndef CPU_SYSCALL_H
#define CPU_SYSCALL_H

#include "cpu/isr.h"

//user/kernel ABI: eax = syscall number, ebx/ecx/edx = args, result back in eax.
#define SYS_WRITE 1   //ebx = buf, ecx = len  -> bytes written
#define SYS_EXIT  2   //ebx = exit code       -> does not return
#define SYS_YIELD 3   //                       -> 0

//dispatched from the int 0x80 stub (interrupt.asm)
void syscall_handler(registers_t *r);

#endif
