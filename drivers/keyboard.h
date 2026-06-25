#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

//called with a completed (newline-terminated input) line, sans the newline
typedef void (*line_handler_t)(char *line);

void init_keyboard(void);
//register who receives a full line when the user presses Enter
void keyboard_set_line_handler(line_handler_t h);

#endif
