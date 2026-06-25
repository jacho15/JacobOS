#include "drivers/screen.h"
#include "drivers/serial.h"
#include "cpu/ports.h"
#include "lib/string.h"

#define VGA_MEMORY 0xB8000
#define VGA_COLS   80
#define VGA_ROWS   25

//CRT controller ports, used to move the hardware cursor
#define CRTC_INDEX 0x3D4
#define CRTC_DATA  0x3D5

static volatile u16 *const vga = (volatile u16 *)VGA_MEMORY;
static u8 cur_color = VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK);
static int cursor_row = 0;
static int cursor_col = 0;

static u16 cell(char c) {
    return (u16)c | ((u16)cur_color << 8);
}

static void move_cursor(void) {
    u16 pos = (u16)(cursor_row * VGA_COLS + cursor_col);
    outb(CRTC_INDEX, 14);
    outb(CRTC_DATA, (u8)(pos >> 8));
    outb(CRTC_INDEX, 15);
    outb(CRTC_DATA, (u8)(pos & 0xFF));
}

static void scroll_if_needed(void) {
    if (cursor_row < VGA_ROWS) return;
    //shift every row up by one
    for (int r = 1; r < VGA_ROWS; r++)
        for (int c = 0; c < VGA_COLS; c++)
            vga[(r - 1) * VGA_COLS + c] = vga[r * VGA_COLS + c];
    //clear the last row
    for (int c = 0; c < VGA_COLS; c++)
        vga[(VGA_ROWS - 1) * VGA_COLS + c] = cell(' ');
    cursor_row = VGA_ROWS - 1;
}

void set_color(u8 color) { cur_color = color; }

void clear_screen(void) {
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) vga[i] = cell(' ');
    cursor_row = 0;
    cursor_col = 0;
    move_cursor();
}

void screen_init(void) {
    serial_init();
    clear_screen();
}

void kputc(char c) {
    //mirror to serial so headless runs are observable
    serial_putc(c);

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 4) & ~3;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            vga[cursor_row * VGA_COLS + cursor_col] = cell(' ');
        }
    } else {
        vga[cursor_row * VGA_COLS + cursor_col] = cell(c);
        cursor_col++;
    }

    if (cursor_col >= VGA_COLS) {
        cursor_col = 0;
        cursor_row++;
    }
    scroll_if_needed();
    move_cursor();
}

void kprint(const char *s) {
    while (*s) kputc(*s++);
}

void kprint_color(const char *s, u8 color) {
    u8 saved = cur_color;
    cur_color = color;
    kprint(s);
    cur_color = saved;
}

void kprint_dec(i32 value) {
    char buf[16];
    kprint(itoa(value, buf));
}

void kprint_hex(u32 value) {
    char buf[16];
    kprint("0x");
    kprint(utoa(value, buf, 16));
}
