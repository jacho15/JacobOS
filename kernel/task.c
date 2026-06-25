#include "kernel/task.h"
#include "mem/kheap.h"
#include "lib/string.h"

#define STACK_SIZE 16384

extern void context_switch(u32 *save_old_esp, u32 new_esp);

static task_t *current;       //task running right now
static task_t *head;          //ring entry point (the initial kernel task)
static u32     next_id;
static int     scheduling_on;

//every task starts here so we can enable interrupts on its fresh stack (a brand
//new task is entered via a plain `ret`, not `iret`, so IF would otherwise stay
//cleared and the task could never be preempted)
static void task_bootstrap(void) {
    __asm__ volatile ("sti");
    current->entry();
    task_exit();
}

void tasking_init(void) {
    current = (task_t*)kmalloc(sizeof(task_t));
    current->id = next_id++;
    strncpy(current->name, "kernel", TASK_NAME_MAX - 1);
    current->name[TASK_NAME_MAX - 1] = 0;
    current->state = TASK_RUNNING;
    current->entry = 0;
    current->stack_base = 0;
    current->next = current;   //ring of one
    head = current;
    scheduling_on = 1;
}

task_t *task_create(const char *name, void (*entry)(void)) {
    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) return 0;
    t->stack_base = (u32)kmalloc(STACK_SIZE);
    if (!t->stack_base) { kfree(t); return 0; }

    t->id = next_id++;
    strncpy(t->name, name, TASK_NAME_MAX - 1);
    t->name[TASK_NAME_MAX - 1] = 0;
    t->state = TASK_READY;
    t->entry = entry;

    //synthesize a stack that context_switch can "return" into:
    //  [task_exit]       <- if task_bootstrap ever returns
    //  [task_bootstrap]  <- context_switch's ret jumps here
    //  [ebx][esi][edi][ebp] (popped by context_switch)
    u32 *sp = (u32*)(t->stack_base + STACK_SIZE);
    *--sp = (u32)task_exit;
    *--sp = (u32)task_bootstrap;
    *--sp = 0; //ebx
    *--sp = 0; //esi
    *--sp = 0; //edi
    *--sp = 0; //ebp
    t->esp = (u32)sp;

    //insert right after head so round-robin reaches it
    t->next = head->next;
    head->next = t;
    return t;
}

void schedule(void) {
    if (!scheduling_on) return;

    task_t *prev = current;
    task_t *next = current->next;
    while (next->state == TASK_DEAD && next != current)
        next = next->next;
    if (next == current) return;  //nothing else runnable

    if (prev->state == TASK_RUNNING) prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    current = next;
    context_switch(&prev->esp, next->esp);
}

void yield(void) {
    __asm__ volatile ("cli");
    schedule();
    __asm__ volatile ("sti");
}

void task_exit(void) {
    __asm__ volatile ("cli");
    current->state = TASK_DEAD;
    scheduling_on = 1;
    schedule();
    //unreachable: the scheduler never switches back to a dead task
    for (;;) __asm__ volatile ("hlt");
}

int task_count(void) {
    int n = 0;
    task_t *t = head;
    do { if (t->state != TASK_DEAD) n++; t = t->next; } while (t != head);
    return n;
}

task_t *task_current(void) { return current; }

void task_foreach(void (*cb)(task_t *t)) {
    task_t *t = head;
    do { cb(t); t = t->next; } while (t != head);
}
