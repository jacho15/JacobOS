#ifndef CPU_IDT_H
#define CPU_IDT_H

#include "cpu/types.h"

#define IDT_ENTRIES 256
#define KERNEL_CS   0x08

//one entry in the interrupt descriptor table
typedef struct {
    u16 low_offset;   //handler address bits 0..15
    u16 selector;     //code segment selector
    u8  always0;
    u8  flags;        //gate type / privilege / present
    u16 high_offset;  //handler address bits 16..31
} __attribute__((packed)) idt_gate_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_register_t;

void set_idt_gate(int n, u32 handler);
//like set_idt_gate but with an explicit descriptor privilege level, so ring-3
//code can reach the gate (needed for the int 0x80 syscall vector with dpl 3)
void set_idt_gate_dpl(int n, u32 handler, u8 dpl);
void load_idt(void);

#endif
