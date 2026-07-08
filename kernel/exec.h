#ifndef KERNEL_EXEC_H
#define KERNEL_EXEC_H

#include "cpu/types.h"

//load and run an ELF program (referenced by SFS inode) in ring 3.
//returns the program's exit code, or -1 on failure to load.
int  exec_user(int inode);

//called by the SYS_EXIT syscall: unwinds back into exec_user with the code
void exec_on_exit(int code);

//true while a ring-3 program is live; the timer uses this to suspend
//preemption (the user shares one kernel transition stack)
int  exec_user_running(void);

#endif
