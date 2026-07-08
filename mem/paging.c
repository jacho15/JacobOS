#include "mem/paging.h"
#include "mem/pmm.h"

//a single page directory of 4MB pages. entry i maps the 4MB at i*4MB
//identity (virtual == physical), which covers the kernel, VGA, stack and heap.
static u32 page_directory[1024] __attribute__((aligned(4096)));

#define PDE_PRESENT 0x1
#define PDE_RW      0x2
#define PDE_USER    0x4
#define PDE_4MB     0x80

void paging_init(void) {
    u32 mapped = PHYS_MEM_SIZE >> 22; //number of 4MB pages to identity-map
    for (u32 i = 0; i < 1024; i++) {
        if (i < mapped)
            page_directory[i] = (i << 22) | PDE_PRESENT | PDE_RW | PDE_4MB;
        else
            page_directory[i] = PDE_RW; //not present
    }

    __asm__ volatile ("mov %0, %%cr3" : : "r"(page_directory));

    //enable 4MB pages (CR4.PSE)
    u32 cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x10;
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));

    //enable paging (CR0.PG)
    u32 cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}

void paging_set_user(u32 vaddr) {
    page_directory[vaddr >> 22] |= PDE_USER;
    //flush the TLB so the new permission takes effect (reload cr3)
    __asm__ volatile ("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax");
}
