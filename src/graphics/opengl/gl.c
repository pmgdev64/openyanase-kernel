// src/graphics/opengl/gl.c
#include "gl.h"
#include <math.h>
#include "../../drivers/gpu/adl.h" // Interface điều khiển driver .adl
#include "../../kernel/mm/memory.h" // Quản lý 4GB RAM

/* -------------------------------------------------------------------------- */
/* INTERNAL STATE MACHINE                                                     */
/* -------------------------------------------------------------------------- */

typedef struct {
    float m[16];
} matrix_t;

typedef struct {
    uint32_t matrix_mode;
    matrix_t projection_matrix;
    matrix_t modelview_stack[32]; // Stack ma trận độ sâu 32
    uint32_t modelview_ptr;

    uint32_t active_texture;
    uint8_t depth_test_enabled;
    uint8_t lighting_enabled;

    float current_color[4];
    float current_tex_coord[2];
    float current_normal[3];

    uint32_t current_program;
} GL_Context;

// Mỗi tiến trình .abp sẽ có một Context riêng (Thread-local hoặc Context-switch handle)
static GL_Context* ctx = NULL;

/* Helper: Reset ma trận về đơn vị (Identity) */
static void mat_identity(matrix_t* mat) {
    for (int i = 0; i < 16; i++) mat->m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
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
