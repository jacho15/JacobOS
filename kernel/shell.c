#include "kernel/shell.h"
#include "kernel/task.h"
#include "drivers/screen.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "fs/sfs.h"
#include "fs/bcache.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "lib/string.h"

static int cwd = SFS_ROOT;          //current directory inode
volatile u32 worker_ticks;          //shared work counter for demo tasks

//--- path handling ---------------------------------------------------------
//resolve an absolute or relative path to an inode (-1 if any part is missing).
//"." and ".." resolve naturally because they are real directory entries.
static int resolve(const char *path) {
    int cur = (path[0] == '/') ? SFS_ROOT : cwd;
    char comp[32];
    int ci = 0;
    for (const char *p = path; ; p++) {
        if (*p == '/' || *p == 0) {
            if (ci > 0) {
                comp[ci] = 0;
                cur = sfs_lookup(cur, comp);
                if (cur < 0) return -1;
                ci = 0;
            }
            if (*p == 0) break;
        } else if (ci < 31) {
            comp[ci++] = *p;
        }
    }
    return cur;
}

//find the name of `child` inside `parent` (skips "." and "..")
static int name_of(int parent, int child, char *out) {
    char nm[32];
    for (int i = 0; ; i++) {
        int ino = sfs_readdir(parent, i, nm);
        if (ino < 0) break;
        if (ino == child && strcmp(nm, ".") && strcmp(nm, "..")) {
            strcpy(out, nm);
            return 0;
        }
    }
    return -1;
}

static void print_cwd(void) {
    if (cwd == SFS_ROOT) { kprint("/"); return; }
    char parts[16][32];
    int n = 0, cur = cwd;
    while (cur != SFS_ROOT && n < 16) {
        int parent = sfs_lookup(cur, "..");
        if (parent < 0 || name_of(parent, cur, parts[n]) != 0) break;
        n++;
        cur = parent;
    }
    for (int i = n - 1; i >= 0; i--) { kprint("/"); kprint(parts[i]); }
}

//--- commands --------------------------------------------------------------
static void cmd_help(void) {
    kprint("commands:\n");
    kprint("  help            this list\n");
    kprint("  clear           clear the screen\n");
    kprint("  pwd             print working directory\n");
    kprint("  ls [path]       list a directory\n");
    kprint("  cd <dir>        change directory\n");
    kprint("  mkdir <name>    create a directory\n");
    kprint("  touch <name>    create an empty file\n");
    kprint("  cat <file>      print a file\n");
    kprint("  echo <txt> > f  write text to file f\n");
    kprint("  rm <name>       remove a file or empty dir\n");
    kprint("  ps              list running tasks\n");
    kprint("  spawn           start a background task\n");
    kprint("  stress          hammer the heap\n");
    kprint("  meminfo         memory + cache stats\n");
    kprint("  uptime          seconds since boot\n");
}

static void cmd_ls(const char *path) {
    int dir = (path && path[0]) ? resolve(path) : cwd;
    if (dir < 0) { kprint("ls: no such path\n"); return; }
    if (sfs_type(dir) != SFS_DIR) { kprint("ls: not a directory\n"); return; }
    char nm[32];
    for (int i = 0; ; i++) {
        int ino = sfs_readdir(dir, i, nm);
        if (ino < 0) break;
        if (strcmp(nm, ".") == 0 || strcmp(nm, "..") == 0) continue;
        if (sfs_type(ino) == SFS_DIR) { set_color(VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK)); kprint(nm); kprint("/"); set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK)); }
        else { kprint(nm); }
        kprint("\n");
    }
}

static void cmd_cd(const char *path) {
    if (!path || !path[0]) { cwd = SFS_ROOT; return; }
    int dir = resolve(path);
    if (dir < 0) { kprint("cd: no such directory\n"); return; }
    if (sfs_type(dir) != SFS_DIR) { kprint("cd: not a directory\n"); return; }
    cwd = dir;
}

static void cmd_cat(const char *name) {
    if (!name || !name[0]) { kprint("cat: need a file\n"); return; }
    int ino = resolve(name);
    if (ino < 0) { kprint("cat: no such file\n"); return; }
    if (sfs_type(ino) != SFS_FILE) { kprint("cat: not a file\n"); return; }
    u32 sz = sfs_size(ino);
    char *buf = (char*)kmalloc(sz + 1);
    if (!buf) { kprint("cat: out of memory\n"); return; }
    int n = sfs_read(ino, buf, sz);
    buf[n] = 0;
    kprint(buf);
    if (n > 0 && buf[n - 1] != '\n') kprint("\n");
    kfree(buf);
}

static void cmd_ps(void);   //forward

static void cmd_meminfo(void) {
    kprint("frames: ");
    kprint_dec(pmm_used_frames()); kprint(" / "); kprint_dec(pmm_total_frames());
    kprint("   heap: "); kprint_dec(kheap_used()); kprint("B used, ");
    kprint_dec(kheap_free()); kprint("B free\n");
    kprint("cache:  "); kprint_dec(bcache_hits()); kprint(" hits / ");
    kprint_dec(bcache_misses()); kprint(" miss\n");
}

static void cmd_stress(void) {
    void *p[128];
    kprint("allocating 128 blocks... ");
    int ok = 0;
    for (int i = 0; i < 128; i++) { p[i] = kmalloc(512); if (p[i]) ok++; }
    kprint_dec(ok); kprint(" ok, freeing... ");
    for (int i = 0; i < 128; i++) kfree(p[i]);
    kprint("heap used now "); kprint_dec(kheap_used()); kprint("B\n");
}

//split a line into argv in place; returns argc
static int tokenize(char *line, char **argv, int max) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max) {
        while (*p == ' ') *p++ = 0;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
    }
    return argc;
}

static void run_command(char *line) {
    char *argv[16];
    int argc = tokenize(line, argv, 16);
    if (argc == 0) return;
    char *cmd = argv[0];

    if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strcmp(cmd, "clear") == 0) clear_screen();
    else if (strcmp(cmd, "pwd") == 0) { print_cwd(); kprint("\n"); }
    else if (strcmp(cmd, "ls") == 0) cmd_ls(argc > 1 ? argv[1] : 0);
    else if (strcmp(cmd, "cd") == 0) cmd_cd(argc > 1 ? argv[1] : 0);
    else if (strcmp(cmd, "mkdir") == 0) {
        if (argc < 2) kprint("mkdir: need a name\n");
        else if (sfs_create(cwd, argv[1], SFS_DIR) < 0) kprint("mkdir: failed\n");
    }
    else if (strcmp(cmd, "touch") == 0) {
        if (argc < 2) kprint("touch: need a name\n");
        else if (sfs_lookup(cwd, argv[1]) < 0 && sfs_create(cwd, argv[1], SFS_FILE) < 0)
            kprint("touch: failed\n");
    }
    else if (strcmp(cmd, "cat") == 0) cmd_cat(argc > 1 ? argv[1] : 0);
    else if (strcmp(cmd, "echo") == 0) {
        //echo words...  OR  echo words... > file
        int redir = -1;
        for (int i = 1; i < argc; i++) if (strcmp(argv[i], ">") == 0) { redir = i; break; }
        char text[256]; text[0] = 0;
        int end = (redir < 0) ? argc : redir;
        for (int i = 1; i < end; i++) {
            if (i > 1) { int l = strlen(text); if (l < 255) { text[l] = ' '; text[l+1] = 0; } }
            int l = strlen(text); strncpy(text + l, argv[i], 255 - l); text[255] = 0;
        }
        if (redir < 0) { kprint(text); kprint("\n"); }
        else if (redir + 1 >= argc) kprint("echo: need a filename after >\n");
        else {
            const char *fname = argv[redir + 1];
            int ino = sfs_lookup(cwd, fname);
            if (ino < 0) ino = sfs_create(cwd, fname, SFS_FILE);
            if (ino < 0) kprint("echo: cannot create file\n");
            else sfs_write(ino, text, strlen(text));
        }
    }
    else if (strcmp(cmd, "rm") == 0) {
        if (argc < 2) kprint("rm: need a name\n");
        else if (sfs_unlink(cwd, argv[1]) < 0) kprint("rm: failed (missing or non-empty dir)\n");
    }
    else if (strcmp(cmd, "ps") == 0) cmd_ps();
    else if (strcmp(cmd, "spawn") == 0) {
        task_t *t = task_create("worker", shell_worker);
        if (t) { kprint("spawned task #"); kprint_dec((int)t->id); kprint("\n"); }
        else kprint("spawn: failed\n");
    }
    else if (strcmp(cmd, "stress") == 0) cmd_stress();
    else if (strcmp(cmd, "meminfo") == 0) cmd_meminfo();
    else if (strcmp(cmd, "uptime") == 0) {
        kprint_dec((int)(timer_ticks() / 100)); kprint(" s ("); kprint_dec((int)timer_ticks()); kprint(" ticks)\n");
    }
    else { kprint("unknown command: "); kprint(cmd); kprint("  (try 'help')\n"); }

    //persist any filesystem changes from this command
    bcache_sync();
}

//--- ps needs the task list; defined here so it can see task internals ------
static void ps_print(task_t *t) {
    kprint("  #"); kprint_dec((int)t->id);
    kprint(" "); kprint(t->name);
    const char *st = t->state == TASK_RUNNING ? " (running)" :
                     t->state == TASK_DEAD ? " (dead)" : " (ready)";
    kprint(st); kprint("\n");
}
static void cmd_ps(void) {
    kprint("tasks ("); kprint_dec(task_count()); kprint("):\n");
    task_foreach(ps_print);
    kprint("background work counter: "); kprint_dec((int)worker_ticks); kprint("\n");
}

//--- entry points ----------------------------------------------------------
static void prompt(void) {
    set_color(VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
    print_cwd();
    kprint(" $ ");
    set_color(VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
}

static void shell_handle_line(char *line) {
    run_command(line);
    prompt();
}

void shell_init(void) {
    keyboard_set_line_handler(shell_handle_line);
    kprint("\ntype 'help' for commands.\n");
    prompt();
}

void shell_worker(void) {
    //low-CPU background task: tick a shared counter and sleep until the next
    //timer interrupt, so it shows up in `ps` without hogging the cpu
    for (;;) {
        worker_ticks++;
        __asm__ volatile ("hlt");
    }
}
