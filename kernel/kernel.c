#include "drivers/screen.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "drivers/ata.h"
#include "cpu/isr.h"
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
