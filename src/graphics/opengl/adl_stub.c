#include "gl.h"
#include "types.h"

// Đây là nơi logic thực sự sẽ diễn ra sau này
// Hiện tại ta để trống để Linker có thể tìm thấy hàm
void adl_graphics_begin(uint32_t mode) {
    // Logic: Khởi tạo buffer đỉnh, chuẩn bị state máy vẽ
}

/* Cập nhật src/graphics/opengl/adl_stub.c */

void adl_vertex_push(float x, float y, float z, float* color, float* tex, float* norm) {
    // 1. Giả lập Pipeline: v_out = Projection * Modelview * v_in
    // Để đơn giản hóa cho 2D, ta map trực tiếp từ dải -1..1 sang 800x600 nếu đã có Ortho
    
    // Tạm thời dùng logic map pixel trực tiếp để bạn thấy kết quả ngay:
    int screen_x = (int)x; 
    int screen_y = (int)y;

    // 2. Lấy màu từ context (Đã fix lỗi undefined glColor3f)
    uint32_t c = ((uint8_t)(color[0]*255) << 16) | 
                 ((uint8_t)(color[1]*255) << 8) | 
                 ((uint8_t)(color[2]*255));

    // 3. Gọi hàm vẽ của vesa.c
    extern void vesa_put_pixel(int x, int y, uint32_t color);
    vesa_put_pixel(screen_x, screen_y, c);
}

void adl_graphics_end(void) {
    // Logic: Flush buffer, kết thúc phiên vẽ
}