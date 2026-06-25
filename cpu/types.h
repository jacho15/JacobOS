#ifndef CPU_TYPES_H
#define CPU_TYPES_H

//fixed-width integer aliases so the kernel doesnt depend on libc
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

typedef u32                size_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define true  1
#define false 0
typedef u8 bool;

#endif
