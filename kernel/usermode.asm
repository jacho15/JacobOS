[BITS 32]

; void enter_user(u32 entry, u32 user_esp, u32 *save_kesp)
; saves the kernel's callee-saved context + esp (through save_kesp), then builds
; an iret frame and drops to ring 3. control comes back not by returning here but
; via leave_user (called from the SYS_EXIT syscall), which restores that context.
global enter_user
enter_user:
    push ebx
    push esi
    push edi
    push ebp
    ; after 4 pushes, args are at [esp+20]=entry, [esp+24]=user_esp, [esp+28]=save_kesp
    mov eax, [esp + 28]     ; save_kesp
    mov [eax], esp          ; *save_kesp = kernel esp (resume point for leave_user)
    mov ecx, [esp + 20]     ; entry
    mov edx, [esp + 24]     ; user_esp

    mov ax, 0x23            ; user data selector (RPL 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23               ; user SS
    push edx                ; user ESP
    pushf
    pop ebx
    or  ebx, 0x200          ; set IF so the program is preemptible
    push ebx                ; EFLAGS
    push 0x1B               ; user CS (RPL 3)
    push ecx                ; EIP = entry
    iret                    ; -> ring 3

; void leave_user(u32 saved_kesp) -- no return
; restore the kernel context enter_user saved and return into exec_user, just
; past the enter_user call. abandons the syscall stack we were called on.
global leave_user
leave_user:
    mov esp, [esp + 4]      ; saved_kesp
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret
