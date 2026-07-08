#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include "cpu/types.h"

//load a static 32-bit ET_EXEC image from memory: copy its PT_LOAD segments to
//their link addresses, zero .bss, and report the entry point.
//returns 0 on success (entry_out set), -1 if the image is invalid/unsupported.
int elf_load(const u8 *img, u32 size, u32 *entry_out);

#endif
