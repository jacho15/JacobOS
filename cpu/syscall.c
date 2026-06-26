#include "cpu/syscall.h"
#include "kernel/exec.h"
#include "kernel/task.h"
#include "drivers/screen.h"

//the user address window (matches exec.c); pointers from ring 3 must lie here
#define USER_BASE 0x800000u
#define USER_TOP  0xC00000u

static int sys_write(const char *buf, u32 len) {
    u32 p = (u32)buf;
    if (p < USER_BASE || p + len > USER_TOP) return -1;  //reject stray pointers
    for (u32 i = 0; i < len; i++) kputc(buf[i]);
    return (int)len;
}

void syscall_handler(registers_t *r) {
    switch (r->eax) {
        case SYS_WRITE: r->eax = (u32)sys_write((const char*)r->ebx, r->ecx); break;
        case SYS_EXIT:  exec_on_exit((int)r->ebx); break;   //does not return
        case SYS_YIELD: yield(); r->eax = 0; break;
        default:        r->eax = (u32)-1; break;
    }
}
