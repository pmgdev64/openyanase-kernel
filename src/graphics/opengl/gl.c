#include "gl.h"
#include "types.h" // Đảm bảo types.h có định nghĩa matrix_t hoặc yanase_matrix_t

/* --- 1. Khai báo Struct & Context (Phải đặt ở trên cùng) --- */
typedef struct {
    float m[16];
} yanase_matrix_t;

typedef struct {
    uint32_t matrix_mode;
    yanase_matrix_t projection_matrix;
    yanase_matrix_t modelview_stack[32];
    uint32_t modelview_ptr;
    float current_color[4];
    uint32_t current_program;
    uint8_t depth_test_enabled;
    uint8_t lighting_enabled;
} GL_Core_Context;

static GL_Core_Context static_ctx;
static GL_Core_Context* ctx = &static_ctx;

/* Extern các hàm từ vesa.c và adl */
extern void vesa_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
extern void adl_graphics_begin(uint32_t mode);
extern void adl_vertex_push(float x, float y, float z, float* color, float* tex, float* norm);

// Thêm vào src/graphics/opengl/gl.c
// Thêm vào src/graphics/opengl/gl.c
void mat_multiply(yanase_matrix_t* res, yanase_matrix_t* a, yanase_matrix_t* b) {
    yanase_matrix_t out;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out.m[i*4+j] = a->m[i*4+0]*b->m[0*4+j] + a->m[i*4+1]*b->m[1*4+j] +
                           a->m[i*4+2]*b->m[2*4+j] + a->m[i*4+3]*b->m[3*4+j];
        }
    }
    *res = out;
}
// Thêm các extern adl_* khác ở đây...

/* --- 2. Math Core --- */
static void mat_identity(yanase_matrix_t* mat) {
    for (int i = 0; i < 16; i++) mat->m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

/* --- 3. OpenGL API Implementation --- */
void glMatrixMode(uint32_t mode) {
    if (ctx) ctx->matrix_mode = mode;
}

/* Thêm vào src/graphics/opengl/gl.c */
void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar) {
    yanase_matrix_t ortho;
    mat_identity(&ortho);

    ortho.m[0]  = 2.0f / (right - left);
    ortho.m[5]  = 2.0f / (top - bottom);
    ortho.m[10] = -2.0f / (zFar - zNear);
    ortho.m[12] = -(right + left) / (right - left);
    ortho.m[13] = -(top + bottom) / (top - bottom);
    ortho.m[14] = -(zFar + zNear) / (zFar - zNear);

    // Nhân ma trận hiện tại với ma trận Ortho này
    yanase_matrix_t* current = (ctx->matrix_mode == GL_PROJECTION) ? 
                                &ctx->projection_matrix : &ctx->modelview_stack[ctx->modelview_ptr];
    // Sử dụng hàm mat_multiply đã nói ở trên
    mat_multiply(current, current, &ortho);
}

/* --- Thêm vào src/graphics/opengl/gl.c --- */

void glColor3f(float r, float g, float b) {
    if (ctx) {
        ctx->current_color[0] = r;
        ctx->current_color[1] = g;
        ctx->current_color[2] = b;
        ctx->current_color[3] = 1.0f; // Alpha mặc định là 1.0
    }
}

void glColor4f(float r, float g, float b, float a) {
    if (ctx) {
        ctx->current_color[0] = r;
        ctx->current_color[1] = g;
        ctx->current_color[2] = b;
        ctx->current_color[3] = a;
    }
}

void glLoadIdentity(void) {
    if (ctx->matrix_mode == GL_PROJECTION) mat_identity(&ctx->projection_matrix);
    else mat_identity(&ctx->modelview_stack[ctx->modelview_ptr]);
}

void glTranslatef(float x, float y, float z) {
    yanase_matrix_t* mat = (ctx->matrix_mode == GL_PROJECTION) ? 
                            &ctx->projection_matrix : &ctx->modelview_stack[ctx->modelview_ptr];
    mat->m[12] += mat->m[0] * x + mat->m[4] * y + mat->m[8] * z;
    mat->m[13] += mat->m[1] * x + mat->m[5] * y + mat->m[9] * z;
    mat->m[14] += mat->m[2] * x + mat->m[6] * y + mat->m[10] * z;
    mat->m[15] += mat->m[3] * x + mat->m[7] * y + mat->m[11] * z;
}

void glBegin(uint32_t mode) {
    adl_graphics_begin(mode);
}

void glVertex3f(float x, float y, float z) {
    adl_vertex_push(x, y, z, ctx->current_color, NULL, NULL);
}

void glEnd(void) {
    // Gọi lệnh kết thúc từ driver adl
}

void glClearColor(float r, float g, float b, float a) {
    ctx->current_color[0] = r;
    ctx->current_color[1] = g;
    ctx->current_color[2] = b;
    ctx->current_color[3] = a;
}

void glClear(uint32_t mask) {
    // Triển khai adl_clear_buffers(mask);
}

void glEnable(uint32_t cap) {
    if (cap == GL_DEPTH_TEST) ctx->depth_test_enabled = 1;
}