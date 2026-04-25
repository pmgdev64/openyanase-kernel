// src/graphics/cursors.c
#include <stdint.h>
#include "cursors.h"
#include "bmp.h"
#include "vesa.h"

static uint32_t cursor_pixels[CURSOR_W * CURSOR_H];
static uint8_t  cursor_mask  [CURSOR_W * CURSOR_H];

// Fallback nếu không load được BMP
static const uint8_t default_cursor[CURSOR_H][CURSOR_W] = {
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,1,1,0,0,0,0,0,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static void load_default_cursor(void) {
    for (int y = 0; y < CURSOR_H; y++) {
        for (int x = 0; x < CURSOR_W; x++) {
            int idx = y * CURSOR_W + x;
            uint8_t v = default_cursor[y][x];
            if      (v == 1) { cursor_mask[idx] = 1; cursor_pixels[idx] = 0xFFFFFF; }
            else if (v == 2) { cursor_mask[idx] = 1; cursor_pixels[idx] = 0x1E90FF; }
            else             { cursor_mask[idx] = 0; cursor_pixels[idx] = 0; }
        }
    }
}

void cursor_init(void) {
    load_default_cursor();
}

void cursor_load_bmp(uint8_t *data, uint32_t size) {
    if (!bmp_parse_to_buffer(data, size, cursor_pixels, cursor_mask, CURSOR_W, CURSOR_H))
        load_default_cursor();
}

void cursor_draw(int x, int y) {
    for (int row = 0; row < CURSOR_H; row++) {
        for (int col = 0; col < CURSOR_W; col++) {
            int idx = row * CURSOR_W + col;
            if (cursor_mask[idx])
                vesa_put_pixel(x + col, y + row, cursor_pixels[idx]);
        }
    }
}