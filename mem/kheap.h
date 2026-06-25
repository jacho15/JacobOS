#ifndef MEM_KHEAP_H
#define MEM_KHEAP_H

#include "cpu/types.h"

//kernel heap region (identity-mapped, above the kernel image and stack)
#define HEAP_START 0x400000u   //4 MB
#define HEAP_SIZE  0x400000u   //4 MB

void  kheap_init(void);
void *kmalloc(size_t size);
void  kfree(void *ptr);

u32   kheap_used(void);   //bytes currently allocated (payloads only)
u32   kheap_free(void);   //bytes still available

#endif
