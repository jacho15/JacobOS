/* user programs are linked to run in the ring-3 window at 8MB (see exec.c).
   _start is the entry point; the loader copies these segments verbatim. */
ENTRY(_start)

SECTIONS {
    . = 0x800000;

    .text   : { *(.text*)   }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*)   }
    .bss    : { *(.bss*)    }
}
