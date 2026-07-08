#include "kernel/elf.h"
#include "lib/string.h"

//the user window programs are linked into (matches exec.c / syscall.c)
#define USER_BASE 0x800000u
#define USER_TOP  0xC00000u

#define ET_EXEC   2
#define EM_386    3
#define PT_LOAD   1

typedef struct {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u32 e_entry;
    u32 e_phoff;
    u32 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
    u32 p_type;
    u32 p_offset;
    u32 p_vaddr;
    u32 p_paddr;
    u32 p_filesz;
    u32 p_memsz;
    u32 p_flags;
    u32 p_align;
} __attribute__((packed)) Elf32_Phdr;

int elf_load(const u8 *img, u32 size, u32 *entry_out) {
    if (size < sizeof(Elf32_Ehdr)) return -1;
    const Elf32_Ehdr *eh = (const Elf32_Ehdr*)img;

    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F') return -1;
    if (eh->e_ident[4] != 1) return -1;             //ELFCLASS32
    if (eh->e_machine != EM_386) return -1;
    if (eh->e_type != ET_EXEC) return -1;
    if (eh->e_phoff + (u32)eh->e_phnum * sizeof(Elf32_Phdr) > size) return -1;

    for (u16 i = 0; i < eh->e_phnum; i++) {
        const Elf32_Phdr *ph =
            (const Elf32_Phdr*)(img + eh->e_phoff + i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD) continue;

        //segment (incl. .bss) must fit entirely inside the user window
        if (ph->p_vaddr < USER_BASE) return -1;
        if (ph->p_vaddr + ph->p_memsz > USER_TOP) return -1;
        if (ph->p_vaddr + ph->p_memsz < ph->p_vaddr) return -1;   //overflow
        if (ph->p_offset + ph->p_filesz > size) return -1;

        memcpy((void*)ph->p_vaddr, img + ph->p_offset, ph->p_filesz);
        memset((void*)(ph->p_vaddr + ph->p_filesz), 0,
               ph->p_memsz - ph->p_filesz);          //.bss
    }

    if (eh->e_entry < USER_BASE || eh->e_entry >= USER_TOP) return -1;
    *entry_out = eh->e_entry;
    return 0;
}
