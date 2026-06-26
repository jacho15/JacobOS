#ifndef MEM_PAGING_H
#define MEM_PAGING_H

#include "cpu/types.h"

//identity-maps physical RAM with 4MB pages (PSE) and turns on paging.
//also installs a page-fault handler that reports the faulting address.
void paging_init(void);

//mark the 4MB page containing vaddr user-accessible (ring 3), so a user
//program loaded into that window can execute and touch its own pages.
void paging_set_user(u32 vaddr);

#endif
