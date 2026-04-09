#include "io.h"
#include "vga_buffer.h"

// Bảng tra mã Scancode Set 1 (US QWERTY)
unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler() {
    // Đọc trạng thái từ cổng 0x64 (Status Register)
    // Bit 0 = 1 nghĩa là có dữ liệu trong Output Buffer
    if (inb(0x64) & 0x01) {
        uint8_t scancode = inb(0x60); // Đọc mã phím từ cổng 0x60
        
        // Nếu bit 7 là 0: Key Pressed. Nếu bit 7 là 1: Key Released.
        if (!(scancode & 0x80)) {
            char c = kbd_us[scancode];
            if (c > 0) {
                vga_putc(c, LIGHT_GREEN); // In ký tự vừa gõ ra màn hình
            }
        }
    }
}