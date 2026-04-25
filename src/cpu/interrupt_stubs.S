.extern mouse_handler
.global irq12_handler
irq12_handler:
    pusha              # Lưu toàn bộ thanh ghi
    call mouse_handler # Gọi hàm xử lý trong mouse.c
    movb $0x20, %al
    outb %al, $0xA0    # Gửi EOI cho Slave PIC
    outb %al, $0x20    # Gửi EOI cho Master PIC
    popa               # Khôi phục thanh ghi
    iret               # Kết thúc ngắt