[BITS 32]

; void gdt_flush(u32 gdt_ptr_addr) -- load the new GDT and reload all segment
; registers. the kernel selectors keep their old values (0x08/0x10) so this is
; transparent to the rest of the kernel.
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10        ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush     ; far jump reloads CS
.flush:
    ret

; void tss_flush(void) -- load the task register with the TSS selector (0x28)
global tss_flush
tss_flush:
    mov ax, 0x28
    ltr ax
    ret
