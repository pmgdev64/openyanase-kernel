#include "gl.h"
#include "types.h"

// Đây là nơi logic thực sự sẽ diễn ra sau này
// Hiện tại ta để trống để Linker có thể tìm thấy hàm
void adl_graphics_begin(uint32_t mode) {
    // Logic: Khởi tạo buffer đỉnh, chuẩn bị state máy vẽ
}

void adl_vertex_push(float x, float y, float z, float* color, float* tex, float* norm) {
    // Logic: Nhân tọa độ với Matrix hiện tại, thực hiện Clipping, 
    // sau đó đẩy vào VESA draw_line hoặc rasterizer.
}

void adl_graphics_end(void) {
    // Logic: Flush buffer, kết thúc phiên vẽ
}