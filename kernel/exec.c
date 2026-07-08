#include "kernel/exec.h"
#include "kernel/elf.h"
#include "cpu/gdt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "fs/sfs.h"

#define USER_BASE      0x800000u
#define USER_STACK_TOP 0xC00000u     //top of the user window; stack grows down
#define MAX_USER_FILE  (14 * 512)    //SFS direct-block file ceiling

//ring0 stack the CPU switches to (via TSS) on a ring3->ring0 trap. dedicated so
//a syscall/IRQ from user mode never reuses an in-progress kernel stack.
static u8 user_kstack[8192] __attribute__((aligned(16)));

//defined in usermode.asm
extern void enter_user(u32 entry, u32 user_esp, u32 *save_kesp);
extern void leave_user(u32 saved_kesp);

static u32 g_kreturn_esp;       //kernel context to resume on exit
static int g_exit_code;
static volatile int g_user_running;

int exec_user_running(void) { return g_user_running; }

void exec_on_exit(int code) {
    g_exit_code = code;
    g_user_running = 0;
    leave_user(g_kreturn_esp);   //never returns
}

int exec_user(int inode) {
    if (sfs_type(inode) != SFS_FILE) return -1;
    u32 sz = sfs_size(inode);
    if (sz == 0 || sz > MAX_USER_FILE) return -1;

    u8 *buf = (u8*)kmalloc(sz);
    if (!buf) return -1;
    int n = sfs_read(inode, (char*)buf, sz);

    u32 entry;
    int rc = elf_load(buf, (u32)n, &entry);
    kfree(buf);
    if (rc != 0) return -1;

    paging_set_user(USER_BASE);
    tss_set_kernel_stack((u32)user_kstack + sizeof(user_kstack));

    g_exit_code = -1;
    g_user_running = 1;
    enter_user(entry, USER_STACK_TOP, &g_kreturn_esp);
    //resumes here via leave_user when the program calls SYS_EXIT
    g_user_running = 0;
    return g_exit_code;
}
