#include "vga_buffer.h"
#include "io.h"
#include "types.h"
#include "fs/iso9660.h"
#include "graphics/vesa.h"
#include "memory/heap.h"

/* --- 1. BIẾN TOÀN CỤC & EXTERN --- */
multiboot_info_t* global_mbi; 

extern void gdt_install(void);
extern unsigned char kbd_us[128];

// Buffer cho Shell
char cmd_buffer[256];
int cmd_ptr = 0;

/* --- 2. CÁC HÀM TIỆN ÍCH --- */

int vga_strncmp(const char *s1, const char *s2, size_t n) {
    while (n--) {
        if (*s1 != *s2) return *(unsigned char *)s1 - *(unsigned char *)s2;
        if (*s1 == '\0') break;
        s1++; s2++;
    }
    return 0;
}

// Reset VGA về Text Mode chuẩn để đảm bảo console hiển thị đúng
void reset_to_text_mode() {
    outw(0x01CE, 4); // Index Enable
    outw(0x01CF, 0); // Disable VBE
}

/* --- 3. SHELL & COMMANDS --- */

void execute_command(char *input) {
    vga_puts("\n", 0x07);

    if (vga_strncmp(input, "help", 4) == 0) {
        vga_puts("Commands: help, clear, hello, startx, reboot\n", 0x0B);
    } 
    else if (vga_strncmp(input, "startx", 6) == 0) {
        vga_puts("Entering Graphical Mode...\n", 0x0A);
        init_vesa_mode(global_mbi); // Sử dụng hàm khởi tạo an toàn đã sửa
    }
    else if (vga_strncmp(input, "hello", 5) == 0) {
        vga_puts("openYanase OS - Kernel Active\n", 0x0A);
    }
    else if (vga_strncmp(input, "clear", 5) == 0) {
        vga_clear();
    }
    else if (vga_strncmp(input, "reboot", 6) == 0) {
        vga_puts("Rebooting...", 0x0C);
        outb(0x64, 0xFE);
    }
    else {
        vga_puts("Unknown command: ", 0x0C);
        vga_puts(input, 0x0C);
    }
    vga_puts("\n> ", 0x0F);
}

void shell_input(char c) {
    if (c == '\n') {
        cmd_buffer[cmd_ptr] = '\0';
        execute_command(cmd_buffer);
        cmd_ptr = 0;
    } else if (c == '\b') {
        if (cmd_ptr > 0) { 
            cmd_ptr--; 
            vga_putc('\b', 0x07); 
        }
    } else if (cmd_ptr < 255) {
        cmd_buffer[cmd_ptr++] = c;
        vga_putc(c, 0x0F);
    }
}

/* --- 2. HỆ THỐNG LOG ĐƠN GIẢN --- */
void klog(const char* msg, uint8_t type) {
    switch (type) {
        case 0: vga_puts("[ INFO  ] ", 0x0B); break; // Cyan
        case 1: vga_puts("[  OK   ] ", 0x0A); break; // Green
        case 2: vga_puts("[ ERROR ] ", 0x0C); break; // Red
        default: vga_puts("[ LOG   ] ", 0x07); break;
    }
    vga_puts(msg, 0x0F);
    vga_puts("\n", 0x0F);
}

void keyboard_poll() {
    // Kiểm tra xem có phím nào được nhấn không (Status Register bit 0)
    if (inb(0x64) & 0x01) {
        uint8_t scancode = inb(0x60);
        if (!(scancode & 0x80)) { // Key press (không phải key release)
            char c = kbd_us[scancode];
            if (c > 0) shell_input(c);
        }
    }
}

/* --- 4. MAIN KERNEL ENTRY --- */
void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
    // 1. Reset VGA ngay lập tức để dọn dẹp rác từ bootloader
    reset_to_text_mode();
    vga_init();

    // 2. Bắt đầu log quá trình init
    vga_puts("--- openYanase Kernel Booting ---\n", 0x0E); // Màu vàng

    // Kiểm tra Multiboot Magic
    if (magic != 0x2BADB002) {
        klog("Invalid Multiboot magic number!", 2);
    } else {
        klog("Multiboot magic verified.", 1);
    }

    // Lưu và kiểm tra MBI
    global_mbi = mbi;
    if (global_mbi) {
        klog("Multiboot Information Structure loaded.", 1);
    } else {
        klog("MBI pointer is NULL!", 2);
    }

    // Khởi tạo GDT
    klog("Installing GDT...", 0);
    gdt_install();
    klog("GDT initialized successfully.", 1);
	
	// Khởi tạo Heap 4MB bắt đầu từ địa chỉ 0x1000000 (16MB mark)
    // Hoặc dùng thông tin mem_upper từ MBI để xác định vùng trống
    heap_init(0x1000000, 4 * 1024 * 1024);
    
    klog("Heap initialized at 0x1000000", 1);

    // Khởi tạo bàn phím (nếu bạn có hàm kbd_init, nếu không thì bỏ qua)
    klog("Polling keyboard controller...", 0);
    
    // Kết thúc quá trình init
    vga_puts("\n", 0x07);
    klog("Kernel Stage 1 ready.", 1);
    vga_puts("\nType 'help' for available commands.\n> ", 0x0F);

    // Vòng lặp chính
    while(1) {
        keyboard_poll();
        // Giải phóng CPU khi không có tác vụ
        // __asm__ volatile("hlt");
    }
}