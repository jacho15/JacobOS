#include "kernel/exec.h"
#include "kernel/elf.h"
#include "kernel/task.h"
#include "cpu/gdt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "fs/sfs.h"

#define USER_BASE      0x800000u
#define USER_STACK_TOP 0xC00000u     //top of the user window; stack grows down
#define MAX_USER_FILE  (14 * 512)    //SFS direct-block file ceiling

//defined in usermode.asm
extern void enter_user(u32 entry, u32 user_esp, u32 *save_kesp);
extern void leave_user(u32 saved_kesp);

static u32 g_kreturn_esp;       //kernel context to resume on exit
static int g_exit_code;
static volatile int g_user_running;

void exec_on_exit(int code) {
    g_exit_code = code;
    g_user_running = 0;
    leave_user(g_kreturn_esp);   //never returns
}

int exec_user(int inode) {
    //one program at a time. a ring-3 program is now preemptible, so a second
    //exec (typed while the first is merely descheduled) would load over the
    //single user window at USER_BASE and clobber a program that is still live.
    if (g_user_running) return -1;

    task_t *t = task_current();
    if (!t->kstack_top) return -1;   //no ring-0 stack for its traps to land on

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
    //traps taken from ring 3 land on this task's own ring-0 stack, which is
    //separate from the stack exec_user is running on, so the frame the program
    //returns through is never overwritten. schedule() keeps TSS.esp0 pointed at
    //whichever task is current, so the program stays preemptible.
    tss_set_kernel_stack(t->kstack_top);

    g_exit_code = -1;
    g_user_running = 1;
    enter_user(entry, USER_STACK_TOP, &g_kreturn_esp);
    //resumes here via leave_user when the program calls SYS_EXIT
    g_user_running = 0;
    return g_exit_code;
}
