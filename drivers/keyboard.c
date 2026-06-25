#include "drivers/keyboard.h"
#include "drivers/screen.h"
#include "cpu/isr.h"
#include "cpu/ports.h"

#define KBD_DATA_PORT 0x60

#define SC_LSHIFT  0x2A
#define SC_RSHIFT  0x36
#define SC_CAPS    0x3A
#define SC_BACK    0x0E
#define SC_ENTER   0x1C
#define RELEASE    0x80

#define LINE_MAX 128
static char line_buf[LINE_MAX];
static int  line_len = 0;
static int  shift = 0;
static int  caps  = 0;

static line_handler_t line_handler = 0;

//US QWERTY scancode -> ascii, unshifted (index = scancode, set 1)
static const char map_lower[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ',
};

static const char map_upper[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ',
};

static int is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void submit_line(void) {
    line_buf[line_len] = 0;
    if (line_handler) line_handler(line_buf);
    line_len = 0;
}

static void keyboard_callback(registers_t *r) {
    (void)r;
    u8 sc = inb(KBD_DATA_PORT);

    //key release
    if (sc & RELEASE) {
        u8 code = sc & ~RELEASE;
        if (code == SC_LSHIFT || code == SC_RSHIFT) shift = 0;
        return;
    }

    //modifiers
    if (sc == SC_LSHIFT || sc == SC_RSHIFT) { shift = 1; return; }
    if (sc == SC_CAPS) { caps = !caps; return; }

    if (sc >= 128) return;
    char c = shift ? map_upper[sc] : map_lower[sc];
    if (!c) return;

    //caps lock only affects letters, and xors with shift
    if (caps && is_letter(c)) {
        if (c >= 'a' && c <= 'z') c -= 32;
        else if (c >= 'A' && c <= 'Z') c += 32;
    }

    if (c == '\n') {
        kputc('\n');
        submit_line();
    } else if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            kputc('\b');
        }
    } else {
        if (line_len < LINE_MAX - 1) {
            line_buf[line_len++] = c;
            kputc(c);
        }
    }
}

void keyboard_set_line_handler(line_handler_t h) {
    line_handler = h;
}

void init_keyboard(void) {
    register_interrupt_handler(IRQ1, keyboard_callback);
}
