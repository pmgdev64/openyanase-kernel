# Định nghĩa các hàm C để Assembly có thể gọi
.extern mouse_handler
.extern keyboard_handler
.extern common_interrupt_handler # Nếu bạn có hàm xử lý chung

.global mouse_isr_wrapper
.global keyboard_isr_wrapper

.global isr_default_handler

# Wrapper cho Chuột (IRQ 12 -> INT 44)
mouse_isr_wrapper:
    pushal
    
    # Lưu DS cũ và nạp Kernel Data Segment (0x10)
    mov %ds, %ax
    push %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    
    cld
    call mouse_handler
    
    # Khôi phục DS cũ
    pop %eax
    mov %ax, %ds
    mov %ax, %es

    # Gửi EOI
    movb $0x20, %al
    outb %al, $0xA1
    outb %al, $0x20
    
    popal
    iret

# Wrapper cho Bàn phím (IRQ 1 -> INT 33)
keyboard_isr_wrapper:
    pushal
    cld
    call keyboard_handler
    
    movb $0x20, %al
    outb %al, $0x20     # Chỉ cần Master PIC cho IRQ 1
    
    popal
    iret

# Định nghĩa nội dung hàm (đặt ở cuối file)
isr_default_handler:
    pushal
    cld
    
    # Gửi EOI cho PIC để tránh treo máy khi có ngắt lạ
    movb $0x20, %al
    outb %al, $0x20    # Master PIC
    outb %al, $0xA1    # Slave PIC
    
    popal
    iret