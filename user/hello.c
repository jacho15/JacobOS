//a tiny freestanding ring-3 program. it has no libc and no crt0: execution
//begins at _start, and it talks to the kernel only through int 0x80 syscalls.

#define SYS_WRITE 1
#define SYS_EXIT  2

static int sys_write(const char *s, int len) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret)
                      : "a"(SYS_WRITE), "b"(s), "c"(len));
    return ret;
}

static void sys_exit(int code) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_EXIT), "b"(code));
}

static unsigned slen(const char *s) {
    unsigned n = 0;
    while (s[n]) n++;
    return n;
}

void _start(void) {
    const char *msg = "hello from ring 3\n";
    sys_write(msg, (int)slen(msg));
    sys_exit(0);
    for (;;) { }   //unreachable: sys_exit does not return
}
