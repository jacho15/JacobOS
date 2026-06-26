#ifndef KERNEL_GREEN_H
#define KERNEL_GREEN_H

#include "cpu/types.h"

//cooperative "green threads" (fibers) multiplexed onto a single kernel thread.
//they switch only at gt_yield(), so they never race each other. gt_run() drives
//the set to completion, then returns to the caller (the M:1 carrier).

void gt_init(void);                 //reset before spawning a fresh batch
int  gt_spawn(void (*fn)(void));    //add a fiber; -> 0 ok / -1 fail
void gt_yield(void);                //give up the CPU to the green scheduler
void gt_run(void);                  //run all fibers until every one exits

#endif
