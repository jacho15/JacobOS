#ifndef LIB_STRING_H
#define LIB_STRING_H

#include "cpu/types.h"

void  *memset(void *dst, int c, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
int    memcmp(const void *a, const void *b, size_t n);

size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);

//convert an unsigned int to a string in the given base (2..16). returns buf.
char  *utoa(u32 value, char *buf, int base);
//convert a signed int to a base-10 string. returns buf.
char  *itoa(i32 value, char *buf);
//parse a leading signed base-10 integer from a string.
int    atoi(const char *s);

#endif
