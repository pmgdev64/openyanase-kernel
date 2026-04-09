#include "io.h"
#include "vga_buffer.h"

// Bảng tra mã Scancode cơ bản (Set 1)
unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler() {
    // Đọc trạng thái từ cổng 0x64
    // Bit 0 của Status Register báo hiệu có dữ liệu đang đợi
    if (inb(0x64) & 0x01) {
        uint8_t scancode = inb(0x60);
        
        // Nếu bit thứ 7 là 0, đó là sự kiện nhấn phím (Key Pressed)
        if (!(scancode & 0x80)) {
            char c = kbd_us[scancode];
            if (c > 0) {
                vga_putc(c, LIGHT_GREEN);
            }
        }
    }
}