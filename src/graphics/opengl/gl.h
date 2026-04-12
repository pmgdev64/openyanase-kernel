// src/graphics/opengl/gl.h
/* * openYanase Graphics Kernel - Standard OpenGL 1.1 - 2.1 Header
 * Architecture: Abstract Driver Layer (ADL) Integrated
 * Author: PmgTeam (Non-Decompilable Logic)
 * Date: 2026-04-12
 */

#ifndef __YANASE_OPENGL_H__
#define __YANASE_OPENGL_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* CONSTANTS                                  */
/* -------------------------------------------------------------------------- */

/* Boolean */
#define GL_FALSE                          0
#define GL_TRUE                           1

/* Data types */
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_DOUBLE                         0x140A

/* Primitives */
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                          0x0007

/* Matrix Mode */
#define GL_MATRIX_MODE                    0x0BA0
#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701
#define GL_TEXTURE                        0x1702

/* Buffers */
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

/* Enable Caps */
#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_STENCIL_TEST                   0x0B90
#define GL_ALPHA_TEST                     0x0BC0
#define GL_BLEND                          0x0BE2
#define GL_LIGHTING                       0x0B50
#define GL_TEXTURE_2D                     0x0DE1
#define GL_SCISSOR_TEST                   0x0C11

/* Texture Filters & Wraps */
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_REPEAT                         0x2901
#define GL_CLAMP                          0x2900
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801

/* Client States */
#define GL_VERTEX_ARRAY                   0x8074
#define GL_NORMAL_ARRAY                   0x8075
#define GL_COLOR_ARRAY                    0x8076
#define GL_TEXTURE_COORD_ARRAY            0x8078

/* Shaders (OpenGL 2.0+) */
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82

/* openYanase Specific (Optimization Params) */
#define GL_ANYASE_CHUNK_LOADER            0x9001
#define GL_ANYASE_RAM_Z_PRIORITY          0x9002

/* -------------------------------------------------------------------------- */
/* FUNCTIONS                                 */
/* -------------------------------------------------------------------------- */

/* --- Miscellaneous --- */
void glClear(uint32_t mask);
void glClearColor(float r, float g, float b, float a);
void glViewport(int x, int y, int width, int height);
void glEnable(uint32_t cap);
void glDisable(uint32_t cap);
void glFlush(void);
void glFinish(void);

/* --- Depth & Stencil --- */
void glDepthFunc(uint32_t func);
void glDepthMask(uint8_t flag);

/* --- Matrix Operations (FFP) --- */
void glMatrixMode(uint32_t mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glRotatef(float angle, float x, float y, float z);
void glScalef(float x, float y, float z);
void glTranslatef(float x, float y, float z);
void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar);
void glFrustum(double left, double right, double bottom, double top, double zNear, double zFar);

/* --- Immediate Mode (FFP) --- */
void glBegin(uint32_t mode);
void glEnd(void);
void glVertex2f(float x, float y);
void glVertex3f(float x, float y, float z);
void glColor3f(float r, float g, float b);
void glColor4f(float r, float g, float b, float a);
void glTexCoord2f(float s, float t);
void glNormal3f(float nx, float ny, float nz);

/* --- Textures --- */
void glGenTextures(int n, uint32_t *textures);
void glBindTexture(uint32_t target, uint32_t texture);
void glTexImage2D(uint32_t target, int level, int internalformat, int width, int height, int border, uint32_t format, uint32_t type, const void *pixels);
void glTexParameteri(uint32_t target, uint32_t pname, int param);
void glDeleteTextures(int n, const uint32_t *textures);

/* --- Lighting --- */
void glLightfv(uint32_t light, uint32_t pname, const float *params);
void glMaterialfv(uint32_t face, uint32_t pname, const float *params);

/* --- Vertex Arrays (OpenGL 1.1) --- */
void glEnableClientState(uint32_t array);
void glDisableClientState(uint32_t array);
void glVertexPointer(int size, uint32_t type, int stride, const void *pointer);
void glColorPointer(int size, uint32_t type, int stride, const void *pointer);
void glTexCoordPointer(int size, uint32_t type, int stride, const void *pointer);
void glDrawArrays(uint32_t mode, int first, int count);
void glDrawElements(uint32_t mode, int count, uint32_t type, const void *indices);

/* --- Buffer Objects (OpenGL 1.5/2.1 VBOs) --- */
void glGenBuffers(int n, uint32_t *buffers);
void glBindBuffer(uint32_t target, uint32_t buffer);
void glBufferData(uint32_t target, ptrdiff_t size, const void *data, uint32_t usage);
void glDeleteBuffers(int n, const uint32_t *buffers);

/* --- Shaders (OpenGL 2.0) --- */
uint32_t glCreateShader(uint32_t type);
void glShaderSource(uint32_t shader, int count, const char **string, const int *length);
void glCompileShader(uint32_t shader);
uint32_t glCreateProgram(void);
void glAttachShader(uint32_t program, uint32_t shader);
void glLinkProgram(uint32_t program);
void glUseProgram(uint32_t program);
void glGetShaderiv(uint32_t shader, uint32_t pname, int *params);
void glGetProgramiv(uint32_t program, uint32_t pname, int *params);

#ifdef __cplusplus
}
#endif

#endif /* __YANASE_OPENGL_H__ */
