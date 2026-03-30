;specify 16 bit code, load at the bootloader address
[BITS 16]
[ORG 0x7C00]

;output a hello message
start:
    mov si, hello_message

;loop to output the hello message
.loop:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0X0E
    int 0x10
    jmp .loop

;stops CPU
.done:

;load the kernel
load_kernel:
    mov ah, 0x02

    mov al, 15

    mov ch, 0
    mov cl, 0x02

    mov dh, 0
    mov dl, 0x00

    mov bx, 0x1000

    int 0x13

    jc disk_error

    jmp switch_pm

;option to switch the OS to 32-bit protected mode
switch_pm:
    cli;
    lgdt [gdt_descriptor];

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
 
    jmp 0x1000

hang:
    cli
    hlt
    jmp hang

disk_error:
    mov si, disk_error_msg
    cli
    hlt

;messages
hello_message db "Welcome to JacobOS", 0
disk_error_msg db "Disk read error!", 0

;include gdt
%include "boot/gdt.asm"

;boot signature, $-$$ is $ for curr and $$ is start of file, so $-$$ is how many bytes written so far
times 510 - ($-$$) db 0
dw 0xAA55