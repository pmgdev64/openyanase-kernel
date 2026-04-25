#include <stdint.h>
#include "io.h"

// Khai báo các hàm Wrapper từ file interrupt.S
extern void mouse_isr_wrapper();
extern void keyboard_isr_wrapper();
extern void isr_default_handler(); // Hàm xử lý mặc định cho các ngắt chưa dùng

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;        // Thường là 0x08 (Kernel Code Segment)
    uint8_t  always0;
    uint8_t  flags;      // 0x8E (Present, Ring 0, 32-bit Interrupt Gate)
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // 1. Phủ kín 256 cổng bằng Default Handler để tránh Triple Fault 
    // khi có ngắt "lạ" (như Timer IRQ0) xảy ra ngay khi sti.
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)isr_default_handler, 0x08, 0x8E);
    }

    // 2. Đăng ký các ngắt phần cứng quan trọng qua Wrapper Assembly
    // IRQ 1: Keyboard (Vector 33 sau khi PIC Remap)
    idt_set_gate(33, (uint32_t)keyboard_isr_wrapper, 0x08, 0x8E);
    
    // IRQ 12: Mouse (Vector 44 sau khi PIC Remap)
    idt_set_gate(44, (uint32_t)mouse_isr_wrapper, 0x08, 0x8E);

    // 3. Nạp IDT vào CPU
    asm volatile("lidt (%0)" : : "r"(&idtp));
}