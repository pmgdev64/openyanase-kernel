#ifndef TYPES_H
#define TYPES_H

/* * openYanase Kernel Types
 * Ngăn chặn xung đột với stddef.h của GCC khi dùng -ffreestanding
 */

// Định nghĩa NULL an toàn
#undef NULL
#define NULL ((void*)0)

// Kiểm tra và định nghĩa các kiểu số nguyên
#ifndef _UINT8_T
#define _UINT8_T
typedef unsigned char      uint8_t;
#endif

#ifndef _UINT16_T
#define _UINT16_T
typedef unsigned short     uint16_t;
#endif

#ifndef _UINT32_T
#define _UINT32_T
typedef unsigned int       uint32_t;
#endif

#ifndef _UINT64_T
#define _UINT64_T
typedef unsigned long long uint64_t;
#endif

// Size_t cho kiến trúc x86 (32-bit)
#ifndef _SIZE_T
#define _SIZE_T
typedef uint32_t           size_t;
#endif

// Các kiểu có dấu nếu cần
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef unsigned int uintptr_t;

#endif