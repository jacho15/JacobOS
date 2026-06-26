#include "drivers/screen.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "drivers/ata.h"
#include "cpu/isr.h"
#include "cpu/gdt.h"
#include "mem/pmm.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "fs/bcache.h"
#include "fs/sfs.h"
#include "kernel/task.h"
#include "kernel/shell.h"
#include "lib/string.h"

void main() {
    screen_init();

    set_color(VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
    kprint("============================\n");
    kprint("   Welcome to JacobOS\n");
    kprint("============================\n");
    set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));

    gdt_install();
    isr_install();

    pmm_init();
    paging_init();
    kheap_init();
    kprint("[ok] paging + heap\n");

    ata_init();
    bcache_init();
    sfs_init();
    {
        //boot counter: proof that the filesystem persists across reboots
        char b[16];
        int f = sfs_lookup(SFS_ROOT, "bootcount");
        int count = 0;
        if (f < 0) f = sfs_create(SFS_ROOT, "bootcount", SFS_FILE);
        else { int n = sfs_read(f, b, 15); b[n] = 0; count = atoi(b); }
        itoa(++count, b);
        sfs_write(f, b, strlen(b));
        bcache_sync();
        kprint("[ok] filesystem mounted (boot #");
        kprint(b);
        kprint(")\n");
    }

    {
        //install the embedded ring-3 demo program as /hello so `exec hello` works
        extern u8 _binary_build_user_hello_elf_start[];
        extern u8 _binary_build_user_hello_elf_end[];
        u32 len = (u32)(_binary_build_user_hello_elf_end - _binary_build_user_hello_elf_start);
        int f = sfs_lookup(SFS_ROOT, "hello");
        if (f < 0) f = sfs_create(SFS_ROOT, "hello", SFS_FILE);
        if (f >= 0) sfs_write(f, (const char*)_binary_build_user_hello_elf_start, len);
        bcache_sync();
    }

    init_keyboard();

    tasking_init();
    task_create("worker", shell_worker);
    task_create("worker", shell_worker);
    init_timer(100);
    kprint("[ok] scheduler + ");
    kprint_dec(task_count());
    kprint(" tasks\n");

    __asm__ volatile ("sti");

    shell_init();

    for (;;) __asm__ volatile ("hlt");
}
