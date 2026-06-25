#ifndef MEM_PAGING_H
#define MEM_PAGING_H

//identity-maps physical RAM with 4MB pages (PSE) and turns on paging.
//also installs a page-fault handler that reports the faulting address.
void paging_init(void);

#endif
