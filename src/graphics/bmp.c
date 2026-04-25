#include "vesa.h"
#include <stdint.h>

typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1, bfReserved2;
    uint32_t bfOffBits;
} __attribute__((packed)) bmp_file_header_t;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
} __attribute__((packed)) bmp_info_header_t;

void draw_bmp(uint8_t* data) {
    bmp_file_header_t* fh = (bmp_file_header_t*)data;
    bmp_info_header_t* ih = (bmp_info_header_t*)(data + sizeof(bmp_file_header_t));

    if (fh->bfType != 0x4D42) return;

    uint8_t* pixels = data + fh->bfOffBits;
    int width        = ih->biWidth;
    int height       = ih->biHeight;
    int bpp          = ih->biBitCount / 8;

    // ✅ Stride = mỗi hàng được pad lên bội số 4 byte
    int stride = (width * bpp + 3) & ~3;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // ✅ Dùng stride thay vì width * bpp
            int offset = y * stride + x * bpp;

            uint8_t b = pixels[offset];
            uint8_t g = pixels[offset + 1];
            uint8_t r = pixels[offset + 2];
            uint32_t color = (r << 16) | (g << 8) | b;

            // BMP lưu từ dưới lên → flip Y
            vesa_put_pixel((uint16_t)x, (uint16_t)(height - y - 1), color);
        }
    }
}

int bmp_parse_to_buffer(uint8_t *data, uint32_t size,
                        uint32_t *out_pixels, uint8_t *out_mask,
                        int expected_w, int expected_h) {
    if (!data || size < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t))
        return 0;

    bmp_file_header_t *fh = (bmp_file_header_t *)data;
    bmp_info_header_t *ih = (bmp_info_header_t *)(data + sizeof(bmp_file_header_t));

    if (fh->bfType != 0x4D42) return 0;

    int w   = ih->biWidth;
    int h   = ih->biHeight;
    int bpp = ih->biBitCount / 8;

    if (w != expected_w || h != expected_h || bpp < 3) return 0;

    uint8_t *pixels = data + fh->bfOffBits;
    int stride = (w * bpp + 3) & ~3;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int offset = (h - 1 - y) * stride + x * bpp;
            int idx    = y * w + x;

            uint8_t b = pixels[offset];
            uint8_t g = pixels[offset + 1];
            uint8_t r = pixels[offset + 2];

            if (r == 0xFF && g == 0x00 && b == 0xFF) {
                out_mask[idx]   = 0;
                out_pixels[idx] = 0;
            } else {
                out_mask[idx]   = 1;
                out_pixels[idx] = (r << 16) | (g << 8) | b;
            }
        }
    }

    return 1;
}