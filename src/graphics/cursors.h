#ifndef CURSORS_H
#define CURSORS_H

#include <stdint.h>

#define CURSOR_W 16   // ← phải có ở đây
#define CURSOR_H 16

void cursor_init(void);
void cursor_load_bmp(uint8_t *data, uint32_t size);
void cursor_draw(int x, int y);

#endif