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
    mov ah, 0X0E;
    int 0x10;
    jmp .loop

;stops CPU
.done:
hang:
    cli;
    hlt;
    jmp hang;

;hello message
hello_message db "Welcome to JacobOS", 0

;boot signature, $-$$ is $ for curr and $$ is start of file, so $-$$ is how many bytes written so far
times 510 - ($-$$) db 0
dw 0xAA55