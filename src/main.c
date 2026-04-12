#include "vga_buffer.h"
#include "io.h"
#include "types.h"

// Khai báo extern từ các file khác
extern void gdt_install(void);

// Buffer để lưu lệnh người dùng gõ vào
char cmd_buffer[256];
int cmd_ptr = 0;

int vga_strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

extern void app_echo(char *args); // Khai báo từ echo.c

void execute_command(char *input) {
    vga_puts("\n", 0);

    if (vga_strncmp(input, "help",1 ) == 0) {
        vga_puts("Commands: help, clear, hello, echo [msg], reboot\n", LIGHT_CYAN);
    } 
    else if (vga_strncmp(input, "clear", 1) == 0) {
        vga_clear();
    }
    // Kiểm tra xem 5 ký tự đầu có phải "echo " không
    else if (vga_strncmp(input, "echo ", 5) == 0) {
        app_echo(input + 5); // Nhảy qua 5 ký tự đầu để lấy phần message
    }
    else if (vga_strncmp(input, "hello", 1) == 0) {
        vga_puts("openYanase v0.0.1 build 0904 - April 12, 2026\n", LIGHT_GREEN);
    }
    else if (vga_strncmp(input, "reboot", 0) == 0) {
        vga_puts("Rebooting...", LIGHT_RED);
        outb(0x64, 0xFE);
    }
    else if (input[0] != '\0') {
        vga_puts("Unknown command: ", LIGHT_RED);
        vga_puts(input, LIGHT_RED);
        vga_puts("\n", 0);
    }

    vga_puts("> ", WHITE);
}

// Hàm xử lý logic phím bấm
void shell_input(char c) {
    if (c == '\n') {
        cmd_buffer[cmd_ptr] = '\0'; // Kết thúc chuỗi
        execute_command(cmd_buffer);
        cmd_ptr = 0; // Reset buffer
    } 
    else if (c == '\b') {
        if (cmd_ptr > 0) {
            cmd_ptr--;
            vga_putc('\b', WHITE); // Gọi hàm putc có xử lý backspace
        }
    } 
    else {
        if (cmd_ptr < 255) {
            cmd_buffer[cmd_ptr++] = c;
            vga_putc(c, WHITE);
        }
    }
}

// Driver keyboard kiểu Polling nhưng tích hợp Shell
extern unsigned char kbd_us[128];
void keyboard_poll() {
    if (inb(0x64) & 0x01) {
        uint8_t scancode = inb(0x60);
        if (!(scancode & 0x80)) { // Key down
            char c = kbd_us[scancode];
            if (c > 0) {
                shell_input(c);
            }
        }
    }
}

void kernel_main() {
    gdt_install();
    vga_init();
    
    vga_puts("openYanase kernel v0.0.1 build 0904\n", LIGHT_CYAN);
    vga_puts("> ", WHITE);

    while(1) {
        keyboard_poll();
    }
}
