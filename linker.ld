ENTRY(main)

SECTIONS {
    . = 0x10000;

    /* kernel_entry.o is linked first so the very first byte at 0x10000 is the
       entry stub the bootloader jumps to. loading here (not 0x1000) keeps the
       growing kernel from overwriting the bootloader/GDT at 0x7C00. */
    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /* marks the end of the loaded kernel image; the physical memory
       allocator hands out frames starting above this address */
    kernel_end = .;
}
