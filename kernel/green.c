#include "kernel/green.h"
#include "mem/kheap.h"

#define GT_STACK_SIZE 4096

enum gt_state { GT_READY, GT_RUNNING, GT_DEAD };

typedef struct gthread {
    u32  esp;          //saved stack pointer (whole context lives on the stack)
    u32  stack_base;   //kmalloc'd fiber stack, for cleanup
    int  state;
    void (*entry)(void);
    struct gthread *next;  //circular ring
} gthread_t;

//reuses the kernel task context switch: save current esp through the pointer,
//load the new esp, restore callee-saved regs and ret into that context.
extern void context_switch(u32 *save_old_esp, u32 new_esp);

static gthread_t *g_ring;      //ring of fibers (NULL when empty)
static gthread_t *g_current;   //fiber running right now
static u32        g_sched_esp; //the scheduler's saved context

void gt_init(void) {
    g_ring = 0;
    g_current = 0;
}

//every fiber starts here so a finished fiber cleanly yields back forever
static void gt_trampoline(void) {
    g_current->entry();
    g_current->state = GT_DEAD;
    gt_yield();   //scheduler never switches back to a dead fiber, so no return
}

int gt_spawn(void (*fn)(void)) {
    gthread_t *t = (gthread_t*)kmalloc(sizeof(gthread_t));
    if (!t) return -1;
    t->stack_base = (u32)kmalloc(GT_STACK_SIZE);
    if (!t->stack_base) { kfree(t); return -1; }
    t->state = GT_READY;
    t->entry = fn;

    //synthesize a stack context_switch can "return" into (same layout as
    //task_create): the ret target, then ebx/esi/edi/ebp popped by the switch
    u32 *sp = (u32*)(t->stack_base + GT_STACK_SIZE);
    *--sp = (u32)gt_trampoline;
    *--sp = 0; //ebx
    *--sp = 0; //esi
    *--sp = 0; //edi
    *--sp = 0; //ebp
    t->esp = (u32)sp;

    if (!g_ring) { t->next = t; g_ring = t; }
    else { t->next = g_ring->next; g_ring->next = t; }
    return 0;
}

void gt_yield(void) {
    context_switch(&g_current->esp, g_sched_esp);
}

//round-robin: scan the ring for a runnable fiber, advancing the start point
static gthread_t *pick_next(void) {
    if (!g_ring) return 0;
    gthread_t *start = g_ring, *t = start;
    do {
        if (t->state == GT_READY) { g_ring = t->next; return t; }
        t = t->next;
    } while (t != start);
    return 0;   //none runnable -> all fibers have exited
}

static void free_all(void) {
    if (!g_ring) return;
    gthread_t *t = g_ring;
    do {
        gthread_t *nx = t->next;
        kfree((void*)t->stack_base);
        kfree(t);
        t = nx;
    } while (t != g_ring);
    g_ring = 0;
    g_current = 0;
}

void gt_run(void) {
    for (;;) {
        gthread_t *next = pick_next();
        if (!next) break;
        g_current = next;
        next->state = GT_RUNNING;
        context_switch(&g_sched_esp, next->esp);
        //back in the scheduler: the fiber either yielded or exited
        if (g_current->state == GT_RUNNING) g_current->state = GT_READY;
    }
    free_all();
}
