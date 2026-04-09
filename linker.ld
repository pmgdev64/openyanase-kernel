ENTRY(start)                /* Điểm bắt đầu là label 'start' trong file asm */

SECTIONS
{
    /* Kernel sẽ được load tại 1MB, đây là tiêu chuẩn cho Multiboot */
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)       /* Đảm bảo header multiboot nằm đầu file */
        *(.text)            /* Code thực thi */
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)          /* Dữ liệu chỉ đọc (string, hằng số) */
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)            /* Biến toàn cục đã khởi tạo */
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)             /* Biến toàn cục chưa khởi tạo (và stack) */
    }
}