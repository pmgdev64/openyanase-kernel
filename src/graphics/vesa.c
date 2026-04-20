#include "../types.h"
#include "io.h"
#include "vesa.h"

typedef struct {
    uint32_t* framebuffer;
    uint16_t  width;
    uint16_t  height;
    uint32_t  pitch; // <--- Biến quan trọng: Số byte thực tế của một dòng
} vesa_info_t;

static vesa_info_t vesa_main;

extern multiboot_info_t* mbi;

/* --- Core Functions --- */

// Cập nhật vesa_init để nhận thêm pitch từ multiboot
void vesa_init(uint32_t fb_addr, uint16_t w, uint16_t h, uint32_t p) {
    vesa_main.framebuffer = (uint32_t*)fb_addr;
    vesa_main.width = w;
    vesa_main.height = h;
    vesa_main.pitch = p; 
}

void init_vesa_mode(multiboot_info_t* mbi) {
    // 1. Luôn set mode bất kể MBI có sống hay không
    set_vesa_mode(800, 600, 32);

    // 2. Giá trị mặc định an toàn cho VBoxVGA
    uint32_t fb_addr = 0xFD000000; 
    uint32_t width = 800;
    uint32_t height = 600;
    uint32_t pitch = 800 * 4;

    if (mbi && (mbi->flags & (1 << 12))) {
        global_mbi = mbi;
        // Đảm bảo lấy phần thấp của địa chỉ nếu là 64-bit
        fb_addr = (uint32_t)(mbi->framebuffer_addr & 0xFFFFFFFF);
        width = mbi->framebuffer_width;
        height = mbi->framebuffer_height;
        pitch = mbi->framebuffer_pitch; 
    }

    vesa_init(fb_addr, width, height, pitch);
    vesa_clear(0x1E1E1E);
    
    // Vẽ một khối màu đỏ thật lớn ngay góc 0,0 để test
    vesa_fill_rect(0, 0, 300, 300, 0xFF0000); 
}

void vesa_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= vesa_main.width || y >= vesa_main.height) return;

    // SỬ DỤNG PITCH: Ép kiểu sang uint8_t* để nhảy đúng số byte scanline
    // sau đó mới cast ngược lại uint32_t* để ghi màu.
    uint32_t* pixel_ptr = (uint32_t*)((uint8_t*)vesa_main.framebuffer + (y * vesa_main.pitch) + (x * 4));
    *pixel_ptr = color;
}

void vesa_clear(uint32_t color) {
    // Không dùng vòng lặp linear i++ vì sẽ bị lệch sọc nếu pitch > width*4
    for (uint16_t y = 0; y < vesa_main.height; y++) {
        for (uint16_t x = 0; x < vesa_main.width; x++) {
            vesa_put_pixel(x, y, color);
        }
    }
}

void set_vesa_mode(uint16_t width, uint16_t height, uint16_t bpp) {
    outw(0x01CE, 4); // VBE_DISPI_INDEX_ENABLE
    outw(0x01CF, 0); // Disable
    
    outw(0x01CE, 1); // XRES
    outw(0x01CF, width);
    
    outw(0x01CE, 2); // YRES
    outw(0x01CF, height);
    
    outw(0x01CE, 3); // BPP
    outw(0x01CF, bpp);
    
    outw(0x01CE, 4); // ENABLE
    outw(0x01CF, 0x01 | 0x40); // LFB_ENABLED | ENABLED
}

/* Thuật toán Bresenham (Giữ nguyên vì nó gọi vesa_put_pixel đã được fix) */
void vesa_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        vesa_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

/* --- Helper Shapes --- */

void vesa_draw_rect(int x, int y, int w, int h, uint32_t color) {
    vesa_draw_line(x, y, x + w, y, color);           
    vesa_draw_line(x, y + h, x + w, y + h, color);   
    vesa_draw_line(x, y, x, y + h, color);           
    vesa_draw_line(x + w, y, x + w, y + h, color);   
}

void vesa_fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        // Vẽ trực tiếp để tối ưu thay vì gọi draw_line (tùy chọn)
        for (int j = 0; j < w; j++) {
            vesa_put_pixel(x + j, y + i, color);
        }
    }
}