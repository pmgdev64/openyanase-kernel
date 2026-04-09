#ifndef VGA_BUFFER_H
#define VGA_BUFFER_H

#include "types.h"

#define VGA_ADDRESS 0xB8000
#define BUF_WIDTH 80
#define BUF_HEIGHT 25

enum vga_color {
    BLACK = 0, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHT_GREY,
    DARK_GREY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, 
    LIGHT_MAGENTA, LIGHT_BROWN, WHITE
};

void vga_init();
void vga_putc(char c, uint8_t color); // THÊM DÒNG NÀY
void vga_puts(const char* str, uint8_t color);
void vga_clear();

#endif