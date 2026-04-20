#include "heap.h"

static block_t *free_list = (block_t*)0;
static uint32_t heap_start = 0;
static uint32_t heap_size = 0;

void heap_init(uint32_t start_addr, uint32_t size) {
    heap_start = start_addr;
    heap_size = size;

    // Khởi tạo khối trống đầu tiên bao phủ toàn bộ Heap
    free_list = (block_t*)heap_start;
    free_list->size = size - sizeof(block_t);
    free_list->free = 1;
    free_list->next = (block_t*)0;
}

void* kmalloc(uint32_t size) {
    block_t *curr = free_list;

    // Tìm khối trống đầu tiên đủ lớn (First Fit)
    while (curr) {
        if (curr->free && curr->size >= size) {
            // Nếu khối lớn hơn mức cần thiết, ta chia nó ra (Split)
            if (curr->size > size + sizeof(block_t) + 4) {
                block_t *new_block = (block_t*)((uint8_t*)curr + sizeof(block_t) + size);
                new_block->size = curr->size - size - sizeof(block_t);
                new_block->free = 1;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }
            curr->free = 0;
            return (void*)((uint8_t*)curr + sizeof(block_t));
        }
        curr = curr->next;
    }
    return (void*)0; // Không đủ bộ nhớ
}

void kfree(void* ptr) {
    if (!ptr) return;

    // Lấy header từ con trỏ dữ liệu
    block_t *block = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    block->free = 1;

    // (Tùy chọn) Coalesce: Gộp các khối trống liền kề để tránh phân mảnh
    block_t *curr = free_list;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}