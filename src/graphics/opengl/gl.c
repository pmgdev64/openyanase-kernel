#include "gl.h"
#include "../types.h"

/* Extern các hàm từ vesa.c */
extern void vesa_put_pixel(uint16_t x, uint16_t y, uint32_t color);
extern void vesa_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
extern void vesa_clear(uint32_t color);

/* -------------------------------------------------------------------------- */
/* MATH CORE                                                                  */
/* -------------------------------------------------------------------------- */

#define PI 3.14159265f

static float yanase_sin(float x) {
    float res = x, term = x;
    for (int i = 1; i < 5; i++) {
        term *= -x * x / (2 * i * (2 * i + 1));
        res += term;
    }
    return res;
}
static float yanase_cos(float x) { return yanase_sin(x + (PI / 2.0f)); }

// Nhân ma trận 4x4 (A = A * B) - Column Major
static void mat_mul(float* A, const float* B) {
    float res[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            res[i * 4 + j] = A[i * 4 + 0] * B[0 * 4 + j] +
                             A[i * 4 + 1] * B[1 * 4 + j] +
                             A[i * 4 + 2] * B[2 * 4 + j] +
                             A[i * 4 + 3] * B[3 * 4 + j];
        }
    }
    for (int i = 0; i < 16; i++) A[i] = res[i];
}

/* -------------------------------------------------------------------------- */
/* RENDERING PIPELINE STORAGE                                                 */
/* -------------------------------------------------------------------------- */

typedef struct { float x, y, z; } vertex_t;
static vertex_t vertex_buffer[64];
static uint32_t vertex_count = 0;
static uint32_t current_mode = 0;

/* -------------------------------------------------------------------------- */
/* MATRIX OPERATIONS                                                          */
/* -------------------------------------------------------------------------- */

void glRotatef(float angle, float x, float y, float z) {
    float rad = angle * (PI / 180.0f);
    float s = yanase_sin(rad);
    float c = yanase_cos(rad);
    float m[16] = {0};
    load_identity_internal((yanase_matrix_t*)m);

    // Ma trận xoay cơ bản (Simplified)
    if (x > 0) { m[5]=c; m[6]=s; m[9]=-s; m[10]=c; }
    else if (y > 0) { m[0]=c; m[2]=-s; m[8]=s; m[10]=c; }
    else if (z > 0) { m[0]=c; m[1]=s; m[4]=-s; m[5]=c; }

    float* target = (ctx->matrix_mode == GL_PROJECTION) ? 
                     ctx->projection_matrix.m : ctx->modelview_stack[ctx->modelview_ptr].m;
    mat_mul(target, m);
}

/* -------------------------------------------------------------------------- */
/* CORE API                                                                   */
/* -------------------------------------------------------------------------- */

void glBegin(uint32_t mode) {
    current_mode = mode;
    vertex_count = 0;
}

void glVertex3f(float x, float y, float z) {
    if (vertex_count >= 64) return;

    // Nhân với Modelview Matrix hiện tại
    float* m = ctx->modelview_stack[ctx->modelview_ptr].m;
    vertex_buffer[vertex_count].x = x*m[0] + y*m[4] + z*m[8] + m[12];
    vertex_buffer[vertex_count].y = x*m[1] + y*m[5] + z*m[9] + m[13];
    vertex_buffer[vertex_count].z = x*m[2] + y*m[6] + z*m[10] + m[14];
    vertex_count++;
}

void glEnd(void) {
    uint32_t color = (uint32_t)(ctx->current_color[0]*255)<<16 | 
                     (uint32_t)(ctx->current_color[1]*255)<<8 | 
                     (uint32_t)(ctx->current_color[2]*255);

    int px[64], py[64];
    for (uint32_t i = 0; i < vertex_count; i++) {
        // Orthographic projection đơn giản xuống 1024x768
        px[i] = (int)((vertex_buffer[i].x + 1.0f) * 512.0f);
        py[i] = (int)((1.0f - vertex_buffer[i].y) * 384.0f);
    }

    // Rasterization
    if (current_mode == GL_TRIANGLES) {
        for (uint32_t i = 0; i < vertex_count - 2; i += 3) {
            vesa_draw_line(px[i], py[i], px[i+1], py[i+1], color);
            vesa_draw_line(px[i+1], py[i+1], px[i+2], py[i+2], color);
            vesa_draw_line(px[i+2], py[i+2], px[i], py[i], color);
        }
    } else if (current_mode == GL_QUADS) {
        for (uint32_t i = 0; i < vertex_count - 3; i += 4) {
            vesa_draw_line(px[i], py[i], px[i+1], py[i+1], color);
            vesa_draw_line(px[i+1], py[i+1], px[i+2], py[i+2], color);
            vesa_draw_line(px[i+2], py[i+2], px[i+3], py[i+3], color);
            vesa_draw_line(px[i+3], py[i+3], px[i], py[i], color);
        }
    }
}

void glClear(uint32_t mask) {
    if (mask & GL_COLOR_BUFFER_BIT) {
        uint32_t c = (uint32_t)(ctx->current_color[0]*255)<<16 | 
                     (uint32_t)(ctx->current_color[1]*255)<<8 | 
                     (uint32_t)(ctx->current_color[2]*255);
        vesa_clear(c);
    }
}
    float current_color[4];
    float current_tex_coord[2];
    uint32_t active_texture;
    
    // Flag trạng thái
    uint8_t depth_test;
    uint8_t lighting;
} GL_Core_Context;

// Khởi tạo Context tĩnh cho bản đầu tiên
static GL_Core_Context static_ctx;
static GL_Core_Context* ctx = &static_ctx;

/* Helper: Đưa ma trận về đơn vị */
static void load_identity_internal(yanase_matrix_t* mat) {
    for (int i = 0; i < 16; i++) {
        mat->m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

/* -------------------------------------------------------------------------- */
/* MATRIX OPERATIONS                                                          */
/* -------------------------------------------------------------------------- */

void glMatrixMode(uint32_t mode) {
    ctx->matrix_mode = mode;
}

void glLoadIdentity(void) {
    if (ctx->matrix_mode == GL_PROJECTION) {
        load_identity_internal(&ctx->projection_matrix);
    } else {
        load_identity_internal(&ctx->modelview_stack[ctx->modelview_ptr]);
    }
}

void glTranslatef(float x, float y, float z) {
    yanase_matrix_t* mat = (ctx->matrix_mode == GL_PROJECTION) ? 
                            &ctx->projection_matrix : 
                            &ctx->modelview_stack[ctx->modelview_ptr];
    
    // Tịnh tiến ma trận cột cuối (Column-major)
    mat->m[12] += mat->m[0] * x + mat->m[4] * y + mat->m[8] * z;
    mat->m[13] += mat->m[1] * x + mat->m[5] * y + mat->m[9] * z;
    mat->m[14] += mat->m[2] * x + mat->m[6] * y + mat->m[10] * z;
}

/* -------------------------------------------------------------------------- */
/* RENDERING STUBS (Đợi liên kết với .adl)                                    */
/* -------------------------------------------------------------------------- */

void glBegin(uint32_t mode) {
    // Bản đầu tiên: Ghi log hoặc gửi tín hiệu ra cổng I/O giả định
    // Sau này sẽ thay bằng: adl_graphics_begin(mode);
}

void glVertex3f(float x, float y, float z) {
    // Logic: Tạm thời chỉ in tọa độ hoặc lưu vào một buffer nội bộ nhỏ
    // glVertex sẽ nhân tọa độ (x,y,z) với modelview_matrix tại đây
}

void glEnd(void) {
    // Gửi lệnh flush dữ liệu ra màn hình
}

/* -------------------------------------------------------------------------- */
/* STATE & MEMORY STUBS                                                       */
/* -------------------------------------------------------------------------- */

void glEnable(uint32_t cap) {
    if (cap == GL_DEPTH_TEST) ctx->depth_test = 1;
    if (cap == GL_LIGHTING)   ctx->lighting = 1;
    // Tạm thời chưa có .adl nên chỉ set flag trong Context
}

void glClearColor(float r, float g, float b, float a) {
    ctx->current_color[0] = r;
    ctx->current_color[1] = g;
    ctx->current_color[2] = b;
    ctx->current_color[3] = a;
}

void glClear(uint32_t mask) {
    // Logic: Xóa bộ đệm khung (Framebuffer) tại địa chỉ 0xA0000 hoặc GOP
}

/* --- OpenGL 2.0 Shader Stubs --- */
uint32_t glCreateShader(uint32_t type) {
    return 1; // Trả về ID giả định
}

void glUseProgram(uint32_t program) {
    // Placeholder cho logic shader sau này
}    for (int i = 0; i < 16; i++) mat->m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

/* -------------------------------------------------------------------------- */
/* MATRIX OPERATIONS (FFP)                                                    */
/* -------------------------------------------------------------------------- */

void glMatrixMode(uint32_t mode) {
    if (ctx) ctx->matrix_mode = mode;
}

void glLoadIdentity(void) {
    if (ctx->matrix_mode == GL_PROJECTION) mat_identity(&ctx->projection_matrix);
    else mat_identity(&ctx->modelview_stack[ctx->modelview_ptr]);
}

void glTranslatef(float x, float y, float z) {
    matrix_t* mat = (ctx->matrix_mode == GL_PROJECTION) ? 
                     &ctx->projection_matrix : &ctx->modelview_stack[ctx->modelview_ptr];
    
    // Tối ưu hóa: Nhân trực tiếp vào cột cuối của ma trận hiện tại
    mat->m[12] += mat->m[0] * x + mat->m[4] * y + mat->m[8] * z;
    mat->m[13] += mat->m[1] * x + mat->m[5] * y + mat->m[9] * z;
    mat->m[14] += mat->m[2] * x + mat->m[6] * y + mat->m[10] * z;
    mat->m[15] += mat->m[3] * x + mat->m[7] * y + mat->m[11] * z;
}

void glPushMatrix(void) {
    if (ctx->matrix_mode == GL_MODELVIEW && ctx->modelview_ptr < 31) {
        ctx->modelview_stack[ctx->modelview_ptr + 1] = ctx->modelview_stack[ctx->modelview_ptr];
        ctx->modelview_ptr++;
    }
}

void glPopMatrix(void) {
    if (ctx->matrix_mode == GL_MODELVIEW && ctx->modelview_ptr > 0) {
        ctx->modelview_ptr--;
    }
}

/* -------------------------------------------------------------------------- */
/* IMMEDIATE MODE & VERTEX SPEC (FFP)                                         */
/* -------------------------------------------------------------------------- */

void glBegin(uint32_t mode) {
    // Thông báo cho Driver .adl chuẩn bị nhận một luồng dữ liệu vẽ (Primitive batch)
    adl_graphics_begin(mode);
}

void glVertex3f(float x, float y, float z) {
    // Đẩy dữ liệu đỉnh kèm theo trạng thái hiện tại (Color, TexCoord) xuống .adl
    // .adl sẽ thực thi logic driver để xử lý phần cứng
    adl_vertex_push(x, y, z, 
                   ctx->current_color, 
                   ctx->current_tex_coord, 
                   ctx->current_normal);
}

void glEnd(void) {
    // Kết thúc batch và ra lệnh GPU thực thi lệnh vẽ
    adl_graphics_end();
}

void glColor4f(float r, float g, float b, float a) {
    ctx->current_color[0] = r; ctx->current_color[1] = g;
    ctx->current_color[2] = b; ctx->current_color[3] = a;
}

/* -------------------------------------------------------------------------- */
/* TEXTURES & MEMORY OPTIMIZATION                                             */
/* -------------------------------------------------------------------------- */

void glTexImage2D(uint32_t target, int level, int internalformat, int width, int height, 
                  int border, uint32_t format, uint32_t type, const void *pixels) {
    
    // Tối ưu hóa 4GB RAM: Kiểm tra dung lượng texture
    size_t size = width * height * 4; // RGBA
    if (mem_get_free() < size) {
        // Kích hoạt logic driver .adl để nén hoặc swap chunk bộ nhớ
        adl_handle_low_memory(GL_ANYASE_RAM_Z_PRIORITY);
    }
    
    adl_upload_texture(target, level, width, height, pixels);
}

/* -------------------------------------------------------------------------- */
/* SHADERS (OpenGL 2.0 Logic)                                                 */
/* -------------------------------------------------------------------------- */

uint32_t glCreateProgram(void) {
    // Driver .adl thực thi tạo handle cho chương trình shader trong phần cứng
    return adl_create_handle(ADL_HANDLE_PROGRAM);
}

void glUseProgram(uint32_t program) {
    ctx->current_program = program;
    adl_bind_program(program);
}

/* -------------------------------------------------------------------------- */
/* MISCELLANEOUS                                                              */
/* -------------------------------------------------------------------------- */

void glEnable(uint32_t cap) {
    switch(cap) {
        case GL_DEPTH_TEST: ctx->depth_test_enabled = 1; break;
        case GL_LIGHTING:   ctx->lighting_enabled = 1; break;
        case GL_ANYASE_CHUNK_LOADER: adl_set_flag(ADL_FLAG_CHUNKLOAD, 1); break;
    }
    adl_set_state(cap, 1);
}

void glClear(uint32_t mask) {
    // Lệnh xóa buffer thường rất nặng, đẩy thẳng xuống .adl để dùng DMA clear
    adl_clear_buffers(mask);
}
