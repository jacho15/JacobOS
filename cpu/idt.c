#include "cpu/idt.h"

static idt_gate_t idt[IDT_ENTRIES];
static idt_register_t idt_reg;

void set_idt_gate(int n, u32 handler) {
    idt[n].low_offset  = (u16)(handler & 0xFFFF);
    idt[n].selector    = KERNEL_CS;
    idt[n].always0     = 0;
    idt[n].flags       = 0x8E; //present, ring 0, 32-bit interrupt gate
    idt[n].high_offset = (u16)((handler >> 16) & 0xFFFF);
}

void set_idt_gate_dpl(int n, u32 handler, u8 dpl) {
    idt[n].low_offset  = (u16)(handler & 0xFFFF);
    idt[n].selector    = KERNEL_CS;
    idt[n].always0     = 0;
    idt[n].flags       = (u8)(0x8E | (dpl << 5)); //present, dpl, 32-bit int gate
    idt[n].high_offset = (u16)((handler >> 16) & 0xFFFF);
}

void load_idt(void) {
    idt_reg.base  = (u32)&idt;
    idt_reg.limit = sizeof(idt) - 1;
    __asm__ volatile ("lidt (%0)" : : "r"(&idt_reg));
}
