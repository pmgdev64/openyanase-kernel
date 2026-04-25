#include "vga_buffer.h"
#include "io.h"  // Cần outb để điều khiển cursor

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint16_t* const vga_buffer = (uint16_t*)VGA_ADDRESS;

// Hàm điều khiển vạch nhấp nháy phần cứng
void update_cursor() {
    uint16_t pos = cursor_y * BUF_WIDTH + cursor_x;

    // VGA port 0x3D4: Index Register
    // VGA port 0x3D5: Data Register
    
    outb(0x3D4, 0x0F); // Register 15: Cursor Location Low
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); // Register 14: Cursor Location High
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

void vga_backspace() {
    // Giả sử biến của bạn là vga_x và vga_y (hãy đổi tên cho khớp với file của bạn)
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_y = 79; 
    }

    // Ghi đè khoảng trắng để xóa ký tự cũ
    // Số 7 là mã màu Light Grey mặc định
    vga_putc(' ', 7); 

    // Lùi lại một lần nữa vì vga_putc vừa làm con trỏ tiến lên
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_y = 79; 
    }
    
    // Cập nhật con trỏ phần cứng nếu bạn có hàm update_cursor
    // update_cursor(); 
}

void vga_clear() {
    for (uint32_t i = 0; i < BUF_WIDTH * BUF_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', LIGHT_GREY);
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor(); // Đưa cursor về 0,0
}

// Thêm hàm scroll vào trước vga_putc
void vga_scroll() {
    // Di chuyển dữ liệu từ dòng 1->24 lên dòng 0->23
    for (uint32_t y = 0; y < BUF_HEIGHT - 1; y++) {
        for (uint32_t x = 0; x < BUF_WIDTH; x++) {
            vga_buffer[y * BUF_WIDTH + x] = vga_buffer[(y + 1) * BUF_WIDTH + x];
        }
    }

    // Xóa dòng cuối cùng (dòng 24)
    for (uint32_t x = 0; x < BUF_WIDTH; x++) {
        vga_buffer[(BUF_HEIGHT - 1) * BUF_WIDTH + x] = vga_entry(' ', LIGHT_GREY);
    }
    
    // Đưa cursor về đầu dòng cuối
    cursor_y = BUF_HEIGHT - 1;
}

void vga_putc(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } 
    else if (c == '\b') { // Xử lý BACKSPACE
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = BUF_WIDTH - 1;
        }
        
        // Ghi đè khoảng trắng vào vị trí đó để "xóa" chữ cũ
        const uint32_t index = cursor_y * BUF_WIDTH + cursor_x;
        vga_buffer[index] = vga_entry(' ', color);
    } 
    else {
        const uint32_t index = cursor_y * BUF_WIDTH + cursor_x;
        vga_buffer[index] = vga_entry(c, color);
        cursor_x++;
    }

    // Xử lý tràn biên ngang
    if (cursor_x >= BUF_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    // Xử lý cuộn màn hình
    if (cursor_y >= BUF_HEIGHT) {
        vga_scroll();
    }
    
    update_cursor();
}

void vga_puts(const char* str, uint8_t color) {
    for (uint32_t i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i], color);
    }
}

void vga_init() {
    vga_clear();
}