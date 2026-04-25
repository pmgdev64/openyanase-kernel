#ifndef VESA_H
#define VESA_H

#include "../types.h"

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    // --- OFFSET 88 (0x58) BẮT ĐẦU TỪ ĐÂY ---
    uint64_t framebuffer_addr;   
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;

// Chỉ khai báo extern, không khởi tạo giá trị ở đây
extern multiboot_info_t* global_mbi;

// Trong vesa.h hoặc đầu file main.c
uint32_t vesa_get_pixel(int x, int y);

// --- PROTOTYPES ---
void init_vesa_mode(multiboot_info_t* mbi);
void vesa_init(uint32_t fb_addr, uint16_t w, uint16_t h, uint32_t p);
void vesa_clear(uint32_t color);
void set_vesa_mode(uint16_t width, uint16_t height, uint16_t bpp);
void vesa_fill_rect(int x, int y, int w, int h, uint32_t color);

#endif // Kết thúc VESA_H