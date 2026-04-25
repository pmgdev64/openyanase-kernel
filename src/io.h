#ifndef IO_H
#define IO_H

#include "types.h"

#include <stdint.h>

/* Ép GCC luôn luôn inline hàm này kể cả khi không bật tối ưu hóa (-O0) */
#define FINLINE static inline __attribute__((always_inline))

#ifdef __INTELLISENSE__
FINLINE void outb(uint16_t port, uint8_t val) { (void)port; (void)val; }
FINLINE void outw(uint16_t port, uint16_t val) { (void)port; (void)val; }
FINLINE uint8_t inb(uint16_t port) { (void)port; return 0; }
FINLINE uint16_t inw(uint16_t port) { (void)port; return 0; }
#else

/* Output 8-bit (Byte) */
FINLINE void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ( "outb %b0, %w1" : : "a"(val), "Nd"(port) );
}

/* Output 16-bit (Word) */
FINLINE void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__ ( "outw %w0, %w1" : : "a"(val), "Nd"(port) );
}
/* Input 8-bit (Byte) */
FINLINE uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

/* Input 16-bit (Word) - Quan trọng cho ATAPI */
FINLINE uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__ ( "inw %w1, %w0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#endif

#endif