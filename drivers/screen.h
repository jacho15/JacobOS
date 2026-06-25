#ifndef DRIVERS_SCREEN_H
#define DRIVERS_SCREEN_H

#include "cpu/types.h"

//VGA text-mode colors
#define VGA_BLACK         0
#define VGA_BLUE          1
#define VGA_GREEN         2
#define VGA_CYAN          3
#define VGA_RED           4
#define VGA_MAGENTA       5
#define VGA_BROWN         6
#define VGA_LIGHT_GREY    7
#define VGA_DARK_GREY     8
#define VGA_LIGHT_BLUE    9
#define VGA_LIGHT_GREEN   10
#define VGA_LIGHT_CYAN    11
#define VGA_LIGHT_RED     12
#define VGA_LIGHT_MAGENTA 13
#define VGA_YELLOW        14
#define VGA_WHITE         15

#define VGA_COLOR(fg, bg) ((u8)((bg) << 4 | (fg)))

void screen_init(void);
void clear_screen(void);
void set_color(u8 color);
void kputc(char c);
void kprint(const char *s);
void kprint_color(const char *s, u8 color);

//formatted helpers (no full printf; just the common cases)
void kprint_dec(i32 value);
void kprint_hex(u32 value);

#endif
