#include "io.h"
#include "vga_buffer.h"

// Trong src/keyboard.c

// Bảng tra mã Scancode Set 1 (US QWERTY)
unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

#define MAX_COMMAND_LEN 256
char shell_buffer[MAX_COMMAND_LEN];
int shell_ptr = 0;
volatile int command_ready = 0; // Cờ báo hiệu lệnh đã sẵn sàng

void keyboard_handler() {
    if (inb(0x64) & 0x01) {
        uint8_t scancode = inb(0x60);
        
        if (!(scancode & 0x80)) {
            char c = kbd_us[scancode];
            
            if (c == '\n') { 
                shell_buffer[shell_ptr] = '\0';
                command_ready = 1;              
                vga_putc('\n', WHITE);     
            } 
            // CHỐT CHẶN: Chỉ xử lý Backspace nếu buffer đang có dữ liệu
            else if (c == '\b') { 
                if (shell_ptr > 0) {
                    shell_ptr--;
                    shell_buffer[shell_ptr] = '\0'; // Xóa ký tự cuối trong buffer
                    vga_backspace(); // Xóa ký tự cuối trên màn hình
                }
                // Nếu shell_ptr == 0, lệnh Backspace sẽ bị bỏ qua hoàn toàn
            }
            else if (c > 0 && shell_ptr < MAX_COMMAND_LEN - 1) {
                shell_buffer[shell_ptr++] = c;  
                vga_putc(c, LIGHT_GREY);       
            }
        }
    }
}