bits 32
section .multiboot
    align 4
    dd 0x1BADB002
    dd 0x03
    dd -(0x1BADB002 + 0x03)

section .text
global start
global gdt_flush       ; Xuất hàm này để file C gọi được
extern kernel_main

start:
    mov esp, stack_space
    call kernel_main
    hlt

; Hàm nạp GDT thực thụ
gdt_flush:
    mov eax, [esp + 4]  ; Lấy tham số (địa chỉ gp) từ stack
    lgdt [eax]          ; Nạp bảng GDT vào CPU

    ; Reset lại các thanh ghi phân đoạn (segment registers)
    mov ax, 0x10        ; 0x10 là offset của Data Segment (Entry 2 * 8)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump để nạp lại thanh ghi CS (Code Segment)
    jmp 0x08:.flush
.flush:
    ret

section .bss
resb 8192
stack_space: