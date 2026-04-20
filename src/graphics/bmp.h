#ifndef BMP_H
#define BMP_H

#include "types.h"

typedef struct {
    uint16_t type;              // Phải là 0x4D42 ("BM")
    uint32_t size;              // Kích thước file
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;            // Vị trí bắt đầu dữ liệu pixel
} __attribute__((packed)) bmp_header_t;

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bpp;               // Bits per pixel (24 hoặc 32)
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_ppm;
    int32_t  y_ppm;
    uint32_t colors_used;
    uint32_t important_colors;
} __attribute__((packed)) bmp_info_header_t;

#endif