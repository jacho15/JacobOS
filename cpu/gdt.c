#include "cpu/gdt.h"
#include "lib/string.h"

//6 entries: null, kernel code/data, user code/data, and one TSS
#define GDT_ENTRIES 6

typedef struct {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  gran;       //limit high nibble + flags
    u8  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) gdt_ptr_t;

//the 32-bit task state segment. the CPU only reads ss0/esp0 (the ring-0 stack
//to switch to on a privilege change); everything else stays zero.
typedef struct {
    u32 prev_tss;
    u32 esp0, ss0;
    u32 esp1, ss1;
    u32 esp2, ss2;
    u32 cr3, eip, eflags;
    u32 eax, ecx, edx, ebx, esp, ebp, esi, edi;
    u32 es, cs, ss, ds, fs, gs;
    u32 ldt;
    u16 trap, iomap_base;
} __attribute__((packed)) tss_t;

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;
static tss_t       tss;

//defined in gdt_flush.asm
extern void gdt_flush(u32 gdt_ptr_addr);
extern void tss_flush(void);

static void set_gate(int i, u32 base, u32 limit, u8 access, u8 flags) {
    gdt[i].limit_low = (u16)(limit & 0xFFFF);
    gdt[i].base_low  = (u16)(base & 0xFFFF);
    gdt[i].base_mid  = (u8)((base >> 16) & 0xFF);
    gdt[i].access    = access;
    gdt[i].gran      = (u8)(((limit >> 16) & 0x0F) | (flags << 4));
    gdt[i].base_high = (u8)((base >> 24) & 0xFF);
}

void tss_set_kernel_stack(u32 esp0) { tss.esp0 = esp0; }

void gdt_install(void) {
    set_gate(0, 0, 0, 0, 0);                       //null
    set_gate(1, 0, 0xFFFFF, 0x9A, 0xC);            //kernel code
    set_gate(2, 0, 0xFFFFF, 0x92, 0xC);            //kernel data
    set_gate(3, 0, 0xFFFFF, 0xFA, 0xC);            //user code   (DPL 3)
    set_gate(4, 0, 0xFFFFF, 0xF2, 0xC);            //user data   (DPL 3)

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = GDT_KERNEL_DATA;
    tss.iomap_base = sizeof(tss);                  //no I/O bitmap
    set_gate(5, (u32)&tss, sizeof(tss) - 1, 0x89, 0x0);  //32-bit TSS (available)

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (u32)&gdt;
    gdt_flush((u32)&gdt_ptr);
    tss_flush();
}
