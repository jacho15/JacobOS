#ifndef CPU_GDT_H
#define CPU_GDT_H

#include "cpu/types.h"

//segment selectors in the GDT we build after boot. the kernel selectors keep
//the same offsets the bootloader's GDT used (0x08/0x10) so the reload is
//seamless; we add ring-3 segments and a TSS for user-mode support.
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18   //or'd with RPL 3 -> 0x1B at use
#define GDT_USER_DATA   0x20   //or'd with RPL 3 -> 0x23 at use
#define GDT_TSS         0x28

//build the GDT (incl. ring-3 segments + TSS), load it, and load the task reg
void gdt_install(void);

//set the kernel stack the CPU switches to on a ring3 -> ring0 transition
void tss_set_kernel_stack(u32 esp0);

#endif
