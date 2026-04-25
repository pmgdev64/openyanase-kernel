#include "../types.h"
#include "io.h"
#include "vesa.h"
#include "opengl/gl.h"
#include "cursors.h"

// Trong src/graphics/vesa.c
extern int32_t mouse_x;
extern int32_t mouse_y;

// Tại src/graphics/vesa.c
//static uint32_t* back_buffer = (void*)0; // Dùng con trỏ để cấp phát sau
uint32_t* vesa_mem;                      // Địa chỉ LFB từ Multiboot

void vesa_draw_cursor(int x, int y) {
    cursor_draw(x, y);
}

/* --- PSF v2 HEADER STRUCTURE --- */
#define PSF2_MAGIC 0x864ab572

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t length;
    uint32_t charsize;
    uint32_t height;
    uint32_t width;
} psf2_t;

// Tọa độ con trỏ ảo để in text
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

// (Tùy chọn) Lưu tọa độ cũ để xóa nếu cần
uint32_t last_cursor_x = 0;
uint32_t last_cursor_y = 0;

/* Ký hiệu được tạo ra bởi objcopy từ src/assets/font.psf */
/* Lưu ý: Nếu lệnh nm hiện tên khác, hãy cập nhật lại ở đây */
extern char _binary_src_assets_font_psf_start;

typedef struct {
    uint32_t* framebuffer;
    uint16_t  width;
    uint16_t  height;
    uint32_t  pitch; 
} vesa_info_t;

typedef struct {
    psf2_t* header;
    uint8_t* glyphs;
    uint32_t width;
    uint32_t height;
    uint32_t charsize;
} vesa_font_t;

vesa_font_t current_font;

static vesa_info_t vesa_main;
uint32_t* back_buffer;

extern multiboot_info_t* global_mbi;
extern int32_t mouse_x, mouse_y; 

/* --- CORE FUNCTIONS --- */

void vesa_scroll_up(uint32_t scanlines) {
    extern uint32_t *back_buffer;
    uint32_t screen_width = 800;
    uint32_t screen_height = 600;

    // 1. Tính toán kích thước 1 dòng quét (bytes)
    uint32_t row_size = screen_width * 4;
    
    // 2. Copy từ dòng [scanlines] lên dòng 0
    // Tổng số byte cần giữ lại: (tổng dòng - dòng cuộn) * row_size
    uint32_t copy_size = (screen_height - scanlines) * row_size;
    memcpy(back_buffer, (uint8_t*)back_buffer + (scanlines * row_size), copy_size);

    // 3. Xóa dòng cuối cùng (điền màu đen)
    memset((uint8_t*)back_buffer + copy_size, 0, scanlines * row_size);
}

void vesa_init(uint32_t fb_addr, uint16_t w, uint16_t h, uint32_t p) {
    vesa_mem = (uint32_t*)fb_addr;
    vesa_main.framebuffer = (uint32_t*)fb_addr;
    vesa_main.width = w;
    vesa_main.height = h;
    vesa_main.pitch = p;
    // Cấp phát back_buffer trong RAM (~1.8MB cho 800x600)
    back_buffer = (uint32_t*)0x2000000; 
}

int vesa_load_font(const char* filename) {
    uint32_t size = 0;
    void* data = initrd_read_file(filename, &size); // Hàm initrd của bạn
    
    if (!data) {
        klog("FONT: Could not find font file in ramdisk!", 2);
        return -1;
    }

    psf2_t* header = (psf2_t*)data;
    if (header->magic != PSF2_MAGIC) {
        klog("FONT: Invalid PSF2 magic", 2);
        return -1;
    }

    current_font.header = header;
    current_font.width = header->width;
    current_font.height = header->height;
    current_font.charsize = header->charsize;
    // Glyph bắt đầu sau Header
    current_font.glyphs = (uint8_t*)data + header->header_size;

    klog("FONT: Loaded successfully from ramdisk", 1);
    return 0;
}

void vesa_put_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= vesa_main.width || y >= vesa_main.height) return;
    back_buffer[y * vesa_main.width + x] = color;
}

void vesa_clear(uint32_t color) {
    uint32_t size = vesa_main.width * vesa_main.height;
    for (uint32_t i = 0; i < size; i++) back_buffer[i] = color;
}

// ✅ Bỏ draw_cursor khỏi vesa_update — để caller tự quản lý
void vesa_update() {
    memcpy(vesa_mem, back_buffer, 800 * 600 * 4);
}

// ✅ Nếu muốn giảm lag: chỉ flush vùng cursor thay đổi (2 vùng 16x16)
void vesa_update_region(int x, int y, int w, int h) {
    int pitch = 800 * 4;
    for (int row = y; row < y + h; row++) {
        if (row < 0 || row >= 600) continue;
        int cx = (x < 0) ? 0 : x;
        int cw = (x + w > 800) ? (800 - cx) : (x + w - cx);
        if (cw <= 0) continue;
        uint8_t *src = (uint8_t*)back_buffer + row * pitch + cx * 4;
        uint8_t *dst = (uint8_t*)vesa_mem    + row * pitch + cx * 4;
        memcpy(dst, src, cw * 4);
    }
}

/* --- PSF TEXT RENDERING --- */

void vesa_draw_char(int x, int y, char c, uint32_t color) {
    if (!current_font.glyphs) return;

    // Lấy bitmap của ký tự c
    uint8_t* glyph = current_font.glyphs + (uint8_t)c * current_font.charsize;
    uint32_t bytes_per_line = (current_font.width + 7) / 8;

    for (uint32_t cy = 0; cy < current_font.height; cy++) {
        for (uint32_t cx = 0; cx < current_font.width; cx++) {
            // Kiểm tra bit trong bitmap
            if (glyph[cy * bytes_per_line + (cx / 8)] & (0x80 >> (cx % 8))) {
                vesa_put_pixel(x + cx, y + cy, color);
            } else {
                // Vẽ màu nền để không bị "bóng ma" ký tự cũ
                vesa_put_pixel(x + cx, y + cy, 0x000000); 
            }
        }
    }
}

void vesa_puts(const char* s, uint32_t color) {
    psf2_t* font = (psf2_t*)&_binary_src_assets_font_psf_start;
    
    while (*s) {
        if (*s == '\n') {
            cursor_x = 0;
            cursor_y += font->height;
        } else {
            vesa_draw_char(cursor_x, cursor_y, *s, color);
            cursor_x += font->width;
        }

        // Tự động xuống dòng nếu chạm biên phải (800px)
        if (cursor_x + font->width > 800) {
            cursor_x = 0;
            cursor_y += font->height;
        }

        // Kiểm tra cuộn trang nếu chạm biên dưới (600px)
        if (cursor_y + font->height > 600) {
            vesa_scroll_up(font->height); // Cần viết thêm hàm này
            cursor_y -= font->height;
        }
        s++;
    }
    vesa_update(); // Đẩy từ Backbuffer ra màn hình
}

void vesa_draw_string(int x, int y, const char* str, uint32_t color) {
    psf2_t* font = (psf2_t*)&_binary_src_assets_font_psf_start;
    while (*str) {
        vesa_draw_char(x, y, *str, color);
        x += font->width; // Tự động dãn cách theo chiều rộng font
        str++;
    }
}

void vesa_draw_number(int x, int y, int num, uint32_t color) {
    char buf[16];
    int i = 0;
    if (num == 0) { vesa_draw_string(x, y, "0", color); return; }
    
    // Xử lý số âm
    if (num < 0) { 
        vesa_draw_char(x, y, '-', color); 
        psf2_t* font = (psf2_t*)&_binary_src_assets_font_psf_start;
        x += font->width; 
        num = -num; 
    }
    
    char t[16]; int j = 0;
    while (num > 0) { t[j++] = (num % 10) + '0'; num /= 10; }
    while (j > 0) buf[i++] = t[--j];
    buf[i] = '\0';
    vesa_draw_string(x, y, buf, color);
}

/* --- LOGIC VẼ --- */

void test_opengl_ui() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1); 

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef((float)mouse_x, (float)mouse_y, 0);

    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(0,  0,  0);
        glVertex3f(0,  15, 0);
        glVertex3f(10, 10, 0);
    glEnd();
}

void set_vesa_mode(uint16_t width, uint16_t height, uint16_t bpp) {
    outw(0x01CE, 4); outw(0x01CF, 0);
    outw(0x01CE, 1); outw(0x01CF, width);
    outw(0x01CE, 2); outw(0x01CF, height);
    outw(0x01CE, 3); outw(0x01CF, bpp);
    outw(0x01CE, 4); outw(0x01CF, 0x01 | 0x40);
}

void init_vesa_mode(multiboot_info_t* mbi) {
    // 1. Thiết lập Mode VESA qua BIOS/Multiboot
    set_vesa_mode(800, 600, 32);

    uint32_t fb_addr = 0xFD000000; 
    uint16_t width = 800;
    uint16_t height = 600;
    uint32_t pitch = 800 * 4;

    // 2. Lấy thông tin Framebuffer từ Multiboot
    if (mbi && (mbi->flags & (1 << 12))) {
        fb_addr = (uint32_t)mbi->framebuffer_addr;
        width = mbi->framebuffer_width;
        height = mbi->framebuffer_height;
        pitch = mbi->framebuffer_pitch; 
    }

    // 3. Khởi tạo Buffer và Driver
    vesa_init(fb_addr, width, height, pitch);
    
    // 4. Xóa màn hình về màu nền đặc trưng (Hex code từ Blue Archive theme)
    vesa_clear(0x1E1E1E); 
    
    // 5. Kích hoạt chuột
    init_mouse(); 
    
    klog("VESA: GUI Subsystem initialized.", 1);
}

#define CURSOR_W 16
#define CURSOR_H 16

// Mảng lưu lại phần ảnh nền bị con trỏ che khuất
uint32_t mouse_save_buffer[CURSOR_W * CURSOR_H];

// Tọa độ cũ để biết chỗ nào cần khôi phục
int32_t last_mouse_x = 0;
int32_t last_mouse_y = 0;

// Giả sử mảng backbuffer của bạn tên là vesa_buffer
extern uint32_t* vesa_buffer; 

void mouse_save_back(int x, int y) {
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            // Dùng hàm get_pixel mới tạo để "nhặt" màu từ ảnh BMP
            mouse_save_buffer[i * 16 + j] = vesa_get_pixel(x + j, y + i);
        }
    }
}

// Phải dùng đúng tên biến mà bạn đã định nghĩa để lưu trữ Buffer
extern uint32_t* vesa_mem; // Thay vesa_mem bằng tên biến thực tế của bạn

uint32_t vesa_get_pixel(int x, int y) {
    if (x < 0 || x >= 800 || y < 0 || y >= 600) return 0;
    // ✅ Đọc từ back_buffer, không phải vesa_mem
    return back_buffer[y * 800 + x];
}

void mouse_restore_back(int x, int y) {
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            // Dùng chính vesa_put_pixel để "dán" lại màu cũ
            vesa_put_pixel(x + j, y + i, mouse_save_buffer[i * 16 + j]);
        }
    }
}

// Hàm này sẽ được gọi liên tục trong vòng lặp chính của Kernel
void vesa_render_frame() {
    // A. Xóa Back Buffer (Để không bị lưu vết chuột cũ)
    vesa_clear(0x1E1E1E);

    // B. Vẽ các thành phần tĩnh (Wallpaper/UI)
    vesa_draw_string(10, 10, "openYanase OS v0.0.2", 0x00BFFF); // DeepSkyBlue
    
    // C. Vẽ thông số Debug (Để kiểm tra driver chuột có nhảy số không)
    // vesa_draw_number(10, 30, mouse_x, 0x00FF00);
    // vesa_draw_number(60, 30, mouse_y, 0x00FF00);

    // D. VẼ CURSOR (Luôn vẽ cuối cùng để nó nằm trên cùng)
    vesa_draw_cursor(mouse_x, mouse_y); 

    // E. FLIP: Đẩy toàn bộ Back Buffer ra màn hình thật
    vesa_update();
}