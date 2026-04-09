#ifndef IO_H
#define IO_H

#include "types.h"

/* * Mẹo cho IntelliSense: Khi VS Code quét file, nó sẽ thấy các hàm trống 
 * và không báo lỗi cú pháp. Khi GCC biên dịch, nó sẽ dùng đoạn asm thật.
 */
#ifdef __INTELLISENSE__
static inline void outb(uint16_t port, uint8_t val) { (void)port; (void)val; }
static inline uint8_t inb(uint16_t port) { (void)port; return 0; }
#else

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ (
        "outb %0, %1" 
        : 
        : "a"(val), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(port)
    );
    return ret;
}

#endif

#endif