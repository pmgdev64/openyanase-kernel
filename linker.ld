ENTRY(start)

SECTIONS
{
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Gom tất cả dữ liệu Read-Only và các section binary từ objcopy */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata*)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data*)  /* Thêm dấu * để gom cả .data.something nếu có */
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss*)
    }
    
    /* Ghi lại điểm kết thúc của Kernel để tránh ghi đè stack/heap */
    _kernel_end = .;
}