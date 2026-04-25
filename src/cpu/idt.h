// src/cpu/idt.h
struct idt_entry_struct {
    uint16_t base_low;
    uint16_t sel;        // Đoạn mã (Kernel Code Segment)
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));