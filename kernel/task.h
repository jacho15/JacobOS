#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include "cpu/types.h"

#define TASK_NAME_MAX 32

enum task_state { TASK_RUNNING, TASK_READY, TASK_DEAD };

typedef struct task {
    u32  esp;          //saved stack pointer (the whole context lives on the stack)
    u32  stack_base;   //allocated kernel stack, for reference/cleanup
    u32  id;
    char name[TASK_NAME_MAX];
    int  state;
    void (*entry)(void);
    struct task *next; //circular ready ring
} task_t;

//promote the current execution context into the first task, enabling switching
void    tasking_init(void);
task_t *task_create(const char *name, void (*entry)(void));
void    schedule(void);     //preemptive: pick and switch to the next ready task
void    yield(void);        //cooperative: voluntarily give up the CPU
void    task_exit(void);    //end the current task

int     task_count(void);
task_t *task_current(void);
//visit every task once (for `ps`)
void    task_foreach(void (*cb)(task_t *t));

#endif
