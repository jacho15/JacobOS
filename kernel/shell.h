#ifndef KERNEL_SHELL_H
#define KERNEL_SHELL_H

//registers the shell as the keyboard line handler and prints the first prompt
void shell_init(void);

//body of a background worker task (used by the `spawn` command and the boot
//demo to show multiple processes running concurrently)
void shell_worker(void);

#endif
