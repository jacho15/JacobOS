[BITS 32]

; void context_switch(u32 *save_old_esp, u32 new_esp)
; saves the callee-saved registers of the current task, stores its stack
; pointer through save_old_esp, then loads new_esp and restores that task's
; registers. returning lands in whatever code the new task last ran.
global context_switch
context_switch:
    push ebx
    push esi
    push edi
    push ebp

    mov eax, [esp + 20]   ; save_old_esp  (4 pushes*4 + return addr)
    mov [eax], esp        ; *save_old_esp = current esp
    mov esp, [esp + 24]   ; new_esp

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret
