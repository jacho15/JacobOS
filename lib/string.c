#include "lib/string.h"

void *memset(void *dst, int c, size_t n) {
    u8 *d = (u8*)dst;
    while (n--) *d++ = (u8)c;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    if (d == s || n == 0) return dst;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        //copy backwards when the regions overlap and dst is ahead
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const u8 *x = (const u8*)a, *y = (const u8*)b;
    while (n--) {
        if (*x != *y) return (int)*x - (int)*y;
        x++; y++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (int)(u8)*a - (int)(u8)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return (int)(u8)*a - (int)(u8)*b;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++)) ;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}

char *utoa(u32 value, char *buf, int base) {
    static const char digits[] = "0123456789abcdef";
    char tmp[33];
    int i = 0;
    if (base < 2 || base > 16) { buf[0] = 0; return buf; }
    if (value == 0) tmp[i++] = '0';
    while (value) {
        tmp[i++] = digits[value % (u32)base];
        value /= (u32)base;
    }
    int j = 0;
    while (i) buf[j++] = tmp[--i];
    buf[j] = 0;
    return buf;
}

char *itoa(i32 value, char *buf) {
    char *p = buf;
    u32 mag;
    if (value < 0) { *p++ = '-'; mag = (u32)(-(i64)value); }
    else mag = (u32)value;
    utoa(mag, p, 10);
    return buf;
}

int atoi(const char *s) {
    int sign = 1, v = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return sign * v;
}
