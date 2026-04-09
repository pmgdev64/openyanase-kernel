#include "types.h"

#ifdef __INTELLISENSE__
    #define PACKED
#else
    #define PACKED __attribute__((packed))
#endif

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} PACKED;

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} PACKED;

struct gdt_entry gdt[3];
struct gdt_ptr gp;

extern void gdt_flush(uint32_t);

// Hàm phụ trợ để set giá trị cho từng dòng trong bảng GDT
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

// ĐÂY LÀ HÀM MÀ LINKER ĐANG TÌM KIẾM
void gdt_install() {
    // Thiết lập con trỏ GDT
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (uint32_t)&gdt;

    // 1. Null segment: Luôn luôn phải có
    gdt_set_gate(0, 0, 0, 0, 0);

    // 2. Code segment: Flat model (0 -> 4GB), Quyền thực thi/đọc
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 3. Data segment: Flat model (0 -> 4GB), Quyền đọc/ghi
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // Gọi hàm Assembly trong entry.S để nạp GDT vào CPU
    gdt_flush((uint32_t)&gp);
}