#ifndef MEM_PMM_H
#define MEM_PMM_H

#include "cpu/types.h"

#define FRAME_SIZE       4096u
//assumed RAM size: this custom bootloader gives us no BIOS memory map, so we
//assume the 32MB the Makefile boots qemu with (-m 32M)
#define PHYS_MEM_SIZE    0x2000000u   //32 MB
//first 8MB are reserved: low memory, the kernel image, the boot stack, and the
//kernel heap all live here, so the frame allocator only hands out frames above
#define PMM_RESERVED_END 0x800000u

void pmm_init(void);
u32  pmm_alloc_frame(void);   //returns a physical address, or 0 when full
void pmm_free_frame(u32 addr);
u32  pmm_total_frames(void);
u32  pmm_used_frames(void);

#endif
