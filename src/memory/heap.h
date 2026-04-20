#ifndef HEAP_H
#define HEAP_H

#include "../types.h"

// Cấu trúc một khối bộ nhớ
typedef struct block {
    uint32_t size;       // Kích thước khối (không tính header)
    int free;            // 1 nếu đang trống, 0 nếu đã cấp phát
    struct block *next;  // Con trỏ đến khối tiếp theo
} block_t;

// Khởi tạo vùng Heap
void heap_init(uint32_t start_addr, uint32_t size);

// Cấp phát bộ nhớ
void* kmalloc(uint32_t size);

// Giải phóng bộ nhớ
void kfree(void* ptr);

#endif