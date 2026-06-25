#ifndef CPU_ISR_H
#define CPU_ISR_H

#include "cpu/types.h"

//register state captured by the assembly stubs, in the exact order pushed
typedef struct {
    u32 ds;                                      //saved data segment
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;  //pusha
    u32 int_no, err_code;                        //pushed by stub
    u32 eip, cs, eflags, useresp, ss;            //pushed by cpu
} registers_t;

typedef void (*isr_t)(registers_t*);

//hardware IRQ vector numbers after PIC remap (master at 0x20, slave at 0x28)
#define IRQ0  32  //timer
#define IRQ1  33  //keyboard
#define IRQ2  34
#define IRQ3  35
#define IRQ4  36
#define IRQ5  37
#define IRQ6  38
#define IRQ7  39
#define IRQ8  40
#define IRQ9  41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

void isr_install(void);
void register_interrupt_handler(u8 n, isr_t handler);

#endif
