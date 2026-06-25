;specify 16 bit code, load at the bootloader address
[BITS 16]
[ORG 0x7C00]

;number of sectors of kernel to load (kept generous as the kernel grows)
KERNEL_SECTORS equ 120

start:
    mov [boot_drive], dl     ; BIOS hands us the boot drive in dl

;output a hello message
    mov si, hello_message
.loop:
    lodsb
    cmp al, 0
    je load_kernel
    mov ah, 0x0E
    int 0x10
    jmp .loop

;load the kernel using INT 13h extended read (LBA). one fast call that spans
;as many sectors as the kernel needs, unlike CHS floppy reads which choke at
;track boundaries.
load_kernel:
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

;option to switch the OS to 32-bit protected mode
switch_pm:
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    jmp CODE_SEG:init_pm

;specify that its 32 bit code
[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    jmp 0x10000

hang:
    cli
    hlt
    jmp hang

[BITS 16]
disk_error:
    mov si, disk_error_msg
.print:
    lodsb
    cmp al, 0
    je .stop
    mov ah, 0x0E
    int 0x10
    jmp .print
.stop:
    cli
    hlt

;disk address packet for the extended read: load KERNEL_SECTORS sectors
;starting at LBA 1 (right after this boot sector) into 0x0000:0x1000
dap:
    db 0x10                  ; packet size
    db 0x00                  ; reserved
    dw KERNEL_SECTORS        ; sectors to read
    dw 0x0000                ; destination offset
    dw 0x1000                ; destination segment (0x1000:0000 = phys 0x10000)
    dq 1                     ; starting LBA

boot_drive db 0

;messages
hello_message db "Welcome to JacobOS", 0
disk_error_msg db "Disk read error!", 0

;include gdt
%include "boot/gdt.asm"

;boot signature, $-$$ is $ for curr and $$ is start of file, so $-$$ is how many bytes written so far
times 510 - ($-$$) db 0
dw 0xAA55
