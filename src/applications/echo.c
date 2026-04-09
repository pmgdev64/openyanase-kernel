#include "../vga_buffer.h"

void app_echo(char *args) {
    if (args[0] == '\0') {
        vga_puts("\n", 0);
        return;
    }
    vga_puts(args, 0x07); // In tham số với màu xám trắng
    vga_puts("\n", 0);
}