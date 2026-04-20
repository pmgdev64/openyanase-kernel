.intel_syntax noprefix

# --- 1. CÁC HẰNG SỐ MULTIBOOT ---
.set MAGIC,    0x1BADB002
.set ALIGN,    1 << 0
.set MEMINFO,  1 << 1
.set VIDEO,    1 << 2  
# FLAGS bao gồm VIDEO để GRUB chuẩn bị cấu trúc Framebuffer trong MBI
.set FLAGS,    ALIGN | MEMINFO | VIDEO 
.set CHECKSUM, -(MAGIC + FLAGS)

# --- 2. MULTIBOOT HEADER ---
.section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
    
    # 5 trường tiếp theo (để 0 cho ELF)
    .long 0, 0, 0, 0, 0 
    
    # --- PHẦN QUAN TRỌNG ĐỂ GIỮ TEXT MODE ---
    .long 1         # 1 = Request Text Mode (thay vì 0 cho Linear Graphics)
    .long 80        # Width (Columns)
    .long 25        # Height (Rows)
    .long 0         # Depth (0 cho Text Mode)

# --- 3. PHẦN MÃ LỆNH (TEXT SECTION) ---
.section .text
.globl start
.globl gdt_flush
.extern kernel_main

start:
    cli                         # Tắt ngắt

    # Thiết lập stack (16KB đã khai báo ở dưới)
    mov esp, offset stack_space
    
    # Đẩy tham số vào stack cho hàm kernel_main(magic, mbi_addr)
    push ebx                    # EBX chứa địa chỉ Multiboot Info
    push eax                    # EAX chứa Magic Number

    call kernel_main
    
halt_loop:
    hlt
    jmp halt_loop

# --- 4. GDT FLUSHING ---
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    # Far jump để cập nhật CS register (0x08 là Code Segment selector)
    ljmp 0x08, offset .flush

.flush:
    ret

# --- 5. BỘ NHỚ STACK ---
.section .bss
.align 16
    .skip 16384                 # Stack 16KB
stack_space:

.section .note.GNU-stack,"",@progbits