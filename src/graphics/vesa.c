// src/graphics/vesa.c
#include "../types.h"

typedef struct {
    uint32_t* framebuffer;
    uint16_t  width;
    uint16_t  height;
} vesa_info_t;

static vesa_info_t vesa_main;

/* Khởi tạo địa chỉ Framebuffer */
void vesa_init(uint32_t fb_addr, uint16_t w, uint16_t h) {
    vesa_main.framebuffer = (uint32_t*)fb_addr;
    vesa_main.width = w;
    vesa_main.height = h;
}

/* Vẽ điểm (Pixel) */
void vesa_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= vesa_main.width || y >= vesa_main.height) return;
    vesa_main.framebuffer[y * vesa_main.width + x] = color;
}

/* Xóa màn hình (glClear) */
void vesa_clear(uint32_t color) {
    uint32_t size = vesa_main.width * vesa_main.height;
    for (uint32_t i = 0; i < size; i++) {
        vesa_main.framebuffer[i] = color;
    }
}

/* Thuật toán vẽ đường thẳng (Bresenham) - Nền tảng cho GL_LINES */
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
