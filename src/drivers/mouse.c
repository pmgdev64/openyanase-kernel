// mouse.c — Fixed for openYanase kernel
#include <stdint.h>
#include "io.h"

// ─── Trạng thái toàn cục ────────────────────────────────────────────────────
int32_t mouse_x = 400;
int32_t mouse_y = 300;
uint8_t mouse_cycle = 0;
int8_t  mouse_byte[3];
volatile int mouse_updated = 0;

// ─── Nút chuột (bitmask từ byte[0]) ────────────────────────────────────────
uint8_t mouse_buttons = 0; // bit0=trái, bit1=phải, bit2=giữa

// ─── Kích thước màn hình (chỉnh lại nếu khác) ──────────────────────────────
#define SCREEN_W 800
#define SCREEN_H 600

// ────────────────────────────────────────────────────────────────────────────
//  Nội bộ: chờ PS/2 controller
//    type == 0 → chờ output buffer có dữ liệu (để ĐỌC)
//    type == 1 → chờ input buffer trống        (để GHI)
// ────────────────────────────────────────────────────────────────────────────
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if (inb(0x64) & 0x01) return;
        }
    } else {
        while (timeout--) {
            if (!(inb(0x64) & 0x02)) return;
        }
    }
    // Timeout — không làm gì, caller phải tự chịu
}

/* Gửi lệnh đến Auxiliary Device (chuột) */
static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);   // Chuyển hướng sang port chuột
    mouse_wait(1);
    outb(0x60, data);
}

/* Đọc 1 byte từ data port */
static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

// ────────────────────────────────────────────────────────────────────────────
//  Khởi tạo chuột PS/2
// ────────────────────────────────────────────────────────────────────────────
void init_mouse(void) {
    uint8_t status;

    // 1. Flush output buffer trước khi bắt đầu
    //    (tránh đọc nhầm byte cũ còn kẹt trong buffer)
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    // 2. Bật Auxiliary Device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // 3. Bật IRQ12 trong Command Byte của controller
    mouse_wait(1);
    outb(0x64, 0x20);           // Đọc Command Byte
    status = mouse_read();
    status |=  0x02;            // Set bit 1 → bật IRQ12
    status &= ~0x20;            // Clear bit 5 → bỏ "disable mouse clock"
    mouse_wait(1);
    outb(0x64, 0x60);           // Ghi lại Command Byte
    mouse_wait(1);
    outb(0x60, status);

    // 4. Thiết lập mặc định
    mouse_write(0xF6);
    mouse_read();               // ACK (0xFA)

    // 5. Kích hoạt Data Reporting — BẮT BUỘC, không có thì chuột câm
    mouse_write(0xF4);
    mouse_read();               // ACK (0xFA)
}

// ────────────────────────────────────────────────────────────────────────────
//  Xử lý ngắt IRQ12 — gọi từ IDT handler của bro
//
//  FIX 1: Bỏ check status & 0x20 trong handler.
//         Khi IRQ12 đã fired thì byte trong 0x60 chắc chắn là của chuột.
//         Check thêm có thể drop byte hợp lệ, làm lệch mouse_cycle.
//
//  FIX 2: Gửi EOI cho cả Slave PIC (0xA0) và Master PIC (0x20).
//         Đây là bug nghiêm trọng nhất — thiếu EOI khiến IRQ12 chỉ
//         fire đúng 1 lần rồi controller bị mask luôn.
//
//  FIX 3: Xử lý nút chuột từ byte[0] bit 0-2.
// ────────────────────────────────────────────────────────────────────────────
void mouse_handler(void) {
    // FIX 1: Chỉ kiểm tra output buffer có dữ liệu (bit 0).
    //        Không kiểm tra bit 5 ở đây — tránh drop byte giữa packet.
    if (!(inb(0x64) & 0x01)) {
        goto send_eoi;          // Không có dữ liệu → vẫn phải EOI
    }

    mouse_byte[mouse_cycle] = (int8_t)inb(0x60);

    switch (mouse_cycle) {
    case 0:
        // Byte đầu tiên: bit 3 PHẢI là 1 (Always 1).
        // Nếu không thì đây là byte rác — bỏ qua, không tăng cycle.
        if (!(mouse_byte[0] & 0x08)) {
            // Không tăng mouse_cycle → tự động re-sync ở lần IRQ sau
            goto send_eoi;
        }
        mouse_cycle = 1;
        break;

    case 1:
        mouse_cycle = 2;
        break;

    case 2:
        mouse_cycle = 0;        // Reset trước khi xử lý

        {
            // Giải mã delta X / Y với sign extension 9-bit
            int32_t dx = (int32_t)(uint8_t)mouse_byte[1];
            int32_t dy = (int32_t)(uint8_t)mouse_byte[2];

            if (mouse_byte[0] & 0x10) dx -= 256;   // X âm
            if (mouse_byte[0] & 0x20) dy -= 256;   // Y âm

            mouse_x += dx;
            mouse_y -= dy;  // PS/2: Y+ = lên màn hình; screen: Y+ = xuống

            // Kẹp tọa độ trong màn hình
            if (mouse_x < 0)          mouse_x = 0;
            if (mouse_y < 0)          mouse_y = 0;
            if (mouse_x >= SCREEN_W)  mouse_x = SCREEN_W - 1;
            if (mouse_y >= SCREEN_H)  mouse_y = SCREEN_H - 1;

            // FIX 3: Lưu trạng thái nút
            mouse_buttons = mouse_byte[0] & 0x07;
            // bit0 = trái, bit1 = phải, bit2 = giữa

            mouse_updated = 1;
        }
        break;
    }

send_eoi:
    // FIX 2: Gửi EOI — KHÔNG CÓ CÁI NÀY CHUỘT CHỈ HOẠT ĐỘNG 1 LẦN
    outb(0xA0, 0x20);   // EOI → Slave PIC  (IRQ8–15)
    outb(0x20, 0x20);   // EOI → Master PIC (IRQ0–7)
}