#ifndef _STDINT_H_
#define _STDINT_H_

// Các kiểu số nguyên không dấu (Unsigned)
typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned __int64       uint64_t; // MSVC dùng __int64

// Các kiểu số nguyên có dấu (Signed)
typedef signed char            int8_t;
typedef signed short           int16_t;
typedef signed int             int32_t;
typedef signed __int64         int64_t;

// Các kiểu định dạng độ dài con trỏ (Pointer width)
// Trong chế độ 32-bit, con trỏ là 4 bytes
typedef unsigned int           uintptr_t;
typedef signed int             intptr_t;

// Các giá trị giới hạn (Optional nhưng nên có)
#define UINT8_MAX  0xff
#define UINT16_MAX 0xffff
#define UINT32_MAX 0xffffffff
#define UINT64_MAX 0xffffffffffffffffULL

#endif