#ifndef DRIVERS_SERIAL_H
#define DRIVERS_SERIAL_H

//COM1 serial port, used for headless debug output (qemu -serial stdio).
//the screen driver mirrors everything here so boots can be verified without
//a video window.
void serial_init(void);
void serial_putc(char c);
void serial_print(const char *s);

#endif
