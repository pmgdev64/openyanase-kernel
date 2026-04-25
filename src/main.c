#include "vga_buffer.h"
#include "io.h"
#include "types.h"
#include "fs/iso9660.h"
#include "graphics/vesa.h"
#include "memory/heap.h"
#include "fs/initrd.h"

// Biến lưu trạng thái Initrd
int initrd_ready = 0;

/* Khai báo các biến từ keyboard.c */
extern volatile int command_ready;
extern char shell_buffer[];
extern int shell_ptr;

/* --- 1. BIẾN TOÀN CỤC & EXTERN --- */
multiboot_info_t *global_mbi;

// system_config.h hoặc đặt trong main.c
typedef struct
{
    char theme[32];
    char boot_jar[64];
    char default_user[32];
    int autostart_gui;
} yanase_config_t;

extern void gdt_install(void);
extern unsigned char kbd_us[128];

// Buffer cho Shell
char cmd_buffer[256];
int cmd_ptr = 0;
int fs_ready = 0;

/* --- 2. CÁC HÀM TIỆN ÍCH --- */

int vga_strncmp(const char *s1, const char *s2, size_t n)
{
    while (n--)
    {
        if (*s1 != *s2)
            return *(unsigned char *)s1 - *(unsigned char *)s2;
        if (*s1 == '\0')
            break;
        s1++;
        s2++;
    }
    return 0;
}

/**
 * Sao chép tối đa n ký tự từ nguồn sang đích.
 * Đảm bảo hệ thống không bị tràn bộ đệm khi đọc config từ init.yc.
 */
char *vga_strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0';
    }
    return dest;
}

volatile int is_in_gui = 0; // 0: Text Mode, 1: Image/GUI Mode

void mouse_wait(uint8_t type)
{
    uint32_t timeout = 100000;
    if (type == 0)
    { // Data
        while (timeout--)
        {
            if ((inb(0x64) & 1) == 1)
                return;
        }
    }
    else
    { // Signal
        while (timeout--)
        {
            if ((inb(0x64) & 2) == 0)
                return;
        }
    }
}

yanase_config_t sys_config = {"default", "openyanase-core.jar", "yanase", 0};

void mouse_write(uint8_t write)
{
    mouse_wait(1);
    outb(0x64, 0xD4); // Tell controller to send next byte to mouse
    mouse_wait(1);
    outb(0x60, write);
}

extern void init_mouse();

// Reset VGA về Text Mode chuẩn để đảm bảo console hiển thị đúng
void reset_to_text_mode()
{
    outw(0x01CE, 4); // Index Enable
    outw(0x01CF, 0); // Disable VBE
}
// Mảng lưu lại phần ảnh nền bị con trỏ che khuất
extern uint32_t mouse_save_buffer;

// Tọa độ cũ để biết chỗ nào cần khôi phục
extern int32_t last_mouse_x;
extern int32_t last_mouse_y;

void load_system_config()
{
    uint32_t size = 0;
    // Tìm file trong Ramdisk. Lưu ý: có thể là "./init.yc" tùy vào cách bạn đóng gói TAR
    char *config_data = (char *)initrd_read_file("init.yc", &size);

    if (config_data)
    {
        vga_puts("[ INFO ] Reading init.yc config...\n", 0x0B);

        // Tạo một buffer tạm để xử lý chuỗi (tránh sửa trực tiếp trên vùng nhớ Initrd)
        // Hoặc bạn có thể parse trực tiếp nếu config_data là chuỗi có kết thúc NULL
        parse_yanase_config(config_data, size);
    }
    else
    {
        vga_puts("[ WARN ] init.yc not found. Using default settings.\n", 0x0E);
    }
}

void parse_init_yc(char *buffer, uint32_t size)
{
    char line[128];
    uint32_t i = 0, lp = 0;

    while (i < size)
    {
        char c = buffer[i++];
        if (c == '\n' || c == '\r' || i == size)
        {
            line[lp] = '\0';
            if (lp > 0)
            {
                // Xử lý từng dòng cấu hình
                if (vga_strncmp(line, "theme=", 6) == 0)
                {
                    vga_strncpy(sys_config.theme, line + 6, 31);
                }
                else if (vga_strncmp(line, "core=", 5) == 0)
                {
                    vga_strncpy(sys_config.boot_jar, line + 5, 63);
                }
                else if (vga_strncmp(line, "autostart=true", 14) == 0)
                {
                    sys_config.autostart_gui = 1;
                }
            }
            lp = 0;
        }
        else if (lp < 127)
        {
            line[lp++] = c;
        }
    }
}

void parse_yanase_config(char *data, uint32_t size)
{
    char line[128];
    uint32_t i = 0;
    uint32_t line_ptr = 0;

    klog("YanaseConfig: Parsing init.yc...", 1); // 1 = INFO

    while (i < size)
    {
        char c = data[i++];

        // Xử lý cuối dòng hoặc cuối file
        if (c == '\n' || c == '\r' || i == size)
        {
            line[line_ptr] = '\0';

            if (line_ptr > 0)
            {
                char *key = line;
                char *value = 0;

                // Tách key và value tại dấu '='
                for (int j = 0; j < line_ptr; j++)
                {
                    if (line[j] == '=')
                    {
                        line[j] = '\0';
                        value = &line[j + 1];
                        break;
                    }
                }

                if (value)
                {
                    // Xử lý THEME
                    if (vga_strncmp(key, "theme", 5) == 0)
                    {
                        klog("Config: Theme set to:", 1);
                        klog(value, 0); // 0 = DEBUG/PLAIN
                    }
                    // Xử lý CORE JAR
                    else if (vga_strncmp(key, "core", 4) == 0)
                    {
                        klog("Config: Core JAR identified:", 1);
                        klog(value, 0);
                    }
                    // Các tham số khác (ví dụ autostart)
                    else if (vga_strncmp(key, "autostart", 9) == 0)
                    {
                        klog("Config: Autostart policy updated.", 1);
                    }
                }
            }
            line_ptr = 0;
        }
        else
        {
            if (line_ptr < 127)
            {
                line[line_ptr++] = c;
            }
        }
    }
}

/* --- 3. SHELL & COMMANDS --- */

void execute_command(char *input)
{
    vga_puts("\n", 0x07);

    // Xóa khoảng trắng đầu dòng (nếu cần)
    while (*input == ' ')
        input++;

    if (vga_strncmp(input, "help", 4) == 0)
    {
        vga_puts("Commands: help, ls, cat <file>, java <file>, startx, mem, clear, reboot\n", 0x0B);
    }
    else if (vga_strncmp(input, "ls", 2) == 0)
    {
        // --- Duyệt file từ RAM (Initrd) ---
        vga_puts("--- [ RAMDISK ] ---\n", 0x0B); // Cyan
        if (initrd_ready)
        {
            initrd_list(); // Hàm này liệt kê file từ file TAR trong RAM
        }
        else
        {
            vga_puts("Initrd not available.\n", 0x07);
        }

        // --- Duyệt file từ CD-ROM (ISO9660) ---
        vga_puts("\n--- [ CD-ROM ] ---\n", 0x0A); // Green
        if (fs_ready)
        {
            iso_list_directory("/");
        }
        else
        {
            vga_puts("ISO9660 not mounted (No ATAPI detected).\n", 0x0C);
        }
    }
    else if (vga_strncmp(input, "cat ", 4) == 0)
    {
        const char *filename = input + 4;
        while (*filename == ' ' && *filename != '\0')
            filename++;

        if (*filename == '\0')
        {
            klog("Usage: cat <filename>", 2);
        }
        else
        {
            uint32_t f_size = 0;
            void *f_ptr = initrd_read_file(filename, &f_size);

            if (!f_ptr)
            {
                // Tạo một buffer tĩnh để chứa nội dung file từ ISO
                // 8192 bytes là đủ cho các file config hoặc text nhỏ
                static uint8_t iso_buf[8192];

                // GỌI ĐÚNG 3 THAM SỐ: tên file, buffer đích, và biến nhận kích thước
                f_ptr = iso9660_read_file(filename, iso_buf, &f_size);
            }

            if (f_ptr)
            {
                klog("File content loaded:", 1);
                char *buffer = (char *)f_ptr;
                for (uint32_t i = 0; i < f_size; i++)
                {
                    if (buffer[i] == '\n')
                    {
                        vga_puts("\n", 0x07);
                    }
                    else if (buffer[i] >= 32 && buffer[i] <= 126)
                    {
                        vga_putc(buffer[i], 0x0F);
                    }
                    else if (buffer[i] == '\t')
                    {
                        vga_puts("    ", 0x07);
                    }
                }
                vga_puts("\n", 0x07);
            }
            else
            {
                klog("Error: File not found in Initrd or ISO.", 2);
            }
        }
    }
    else if (vga_strncmp(input, "java ", 5) == 0)
    {
        char *path = input + 5;
        while (*path == ' ')
            path++;

        // Kiểm tra đuôi file
        int len = 0;
        while (path[len])
            len++;

        if (len > 4 && vga_strncmp(path + len - 4, ".jar", 4) == 0)
        {
            klog("JVM: Executing JAR archive...", 1);
            jar_execute(path);
        }
        else if (len > 6 && vga_strncmp(path + len - 6, ".class", 6) == 0)
        {
            klog("JVM: Executing Class file...", 1);
            uint32_t size = 0;
            void *data = initrd_read_file(path, &size);
            if (data)
            {
                class_execute(data, size);
            }
            else
            {
                klog("JVM: Class not found", 2);
            }
        }
        else
        {
            klog("Usage: java <file.jar|file.class>", 0x0E);
        }
    }
    else if (vga_strncmp(input, "imgview ", 8) == 0)
    {
        const char *img_path = input + 8;
        while (*img_path == ' ')
            img_path++;

        uint32_t img_size = 0;
        void *img_data = initrd_read_file(img_path, &img_size);

        if (img_data)
        {
            extern uint32_t *vesa_mem;
            extern uint32_t *back_buffer;
            extern int32_t mouse_x, mouse_y;
            extern volatile int mouse_updated;

            init_vesa_mode(global_mbi);

            // ✅ Load cursor từ initrd, fallback về default nếu không có
            uint32_t cur_size = 0;
            uint8_t *cur_data = (uint8_t *)initrd_read_file("cursor.bmp", &cur_size);
            if (cur_data)
                cursor_load_bmp(cur_data, cur_size);
            else
                cursor_init();

            // Vẽ ảnh 1 lần duy nhất vào back_buffer rồi flush
            draw_bmp(img_data);
            vesa_draw_string(10, 10, "Image: ", 0xFFFFFF);
            vesa_draw_string(70, 10, img_path, 0x00FF00);
            vesa_draw_string(10, 580, "Press any key to exit (Not implemented)", 0xAAAAAA);
            memcpy(vesa_mem, back_buffer, 800 * 600 * 4);

            // Khởi tạo cursor tại vị trí hiện tại
            last_mouse_x = mouse_x;
            last_mouse_y = mouse_y;
            mouse_save_back(mouse_x, mouse_y);
            vesa_draw_cursor(mouse_x, mouse_y);
            vesa_update_region(mouse_x, mouse_y, 16, 16);

            while (1)
            {
                if (!mouse_updated)
                    continue;
                mouse_updated = 0;

                int ox = last_mouse_x;
                int oy = last_mouse_y;

                // Snapshot tọa độ mới — tránh IRQ chen giữa
                int32_t nx = mouse_x;
                int32_t ny = mouse_y;

                mouse_restore_back(ox, oy);

                last_mouse_x = nx;
                last_mouse_y = ny;

                mouse_save_back(nx, ny);
                vesa_draw_cursor(nx, ny);

                vesa_update_region(ox, oy, 16, 16);
                vesa_update_region(nx, ny, 16, 16);
            }
        }
        else
        {
            klog("Error: Image not found.", 2);
        }
    }
    else if (vga_strncmp(input, "mem", 3) == 0)
    {
        klog("Heap Base: 0x1000000 | Size: 4MB", 0);
    }
    else if (vga_strncmp(input, "startx", 6) == 0)
    {
        vga_puts("Entering Graphical Mode...\n", 0x0A);

        extern uint32_t *vesa_mem;
        extern uint32_t *back_buffer;
        extern int32_t mouse_x, mouse_y;
        extern volatile int mouse_updated;

        init_vesa_mode(global_mbi);

        // Load cursor từ initrd
        uint32_t cur_size = 0;
        uint8_t *cur_data = (uint8_t *)initrd_read_file("cursor.bmp", &cur_size);
        if (cur_data)
            cursor_load_bmp(cur_data, cur_size);
        else
            cursor_init();

        // Flush nền đen ra màn hình
        memcpy(vesa_mem, back_buffer, 800 * 600 * 4);

        // Init cursor
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        mouse_save_back(mouse_x, mouse_y);
        vesa_draw_cursor(mouse_x, mouse_y);
        vesa_update_region(mouse_x, mouse_y, 24, 24);

        while (1)
        {
            if (!mouse_updated)
                continue;
            mouse_updated = 0;

            int ox = last_mouse_x;
            int oy = last_mouse_y;
            int32_t nx = mouse_x;
            int32_t ny = mouse_y;

            mouse_restore_back(ox, oy);

            last_mouse_x = nx;
            last_mouse_y = ny;

            mouse_save_back(nx, ny);
            vesa_draw_cursor(nx, ny);

            vesa_update_region(ox, oy, 24, 24);
            vesa_update_region(nx, ny, 24, 24);
        }
    }
    else if (vga_strncmp(input, "clear", 5) == 0)
    {
        vga_clear();
    }
    else if (vga_strncmp(input, "reboot", 6) == 0)
    {
        vga_puts("Rebooting...", 0x0C);
        outb(0x64, 0xFE);
    }
    // 5. SHUTDOWN: Tắt máy (Mới)
    else if (vga_strncmp(input, "shutdown", 8) == 0)
    {
        sys_shutdown();
    }
    else if (*input == '\0')
    {
        // Nhấn Enter trống không làm gì cả
    }
    else
    {
        vga_puts("Unknown command: ", 0x0C);
        vga_puts(input, 0x0C);
    }
}

void vga_switch_to_text_mode()
{
    // 1. Mảng giá trị thanh ghi tiêu chuẩn cho Mode 3 (80x25, 16 màu)
    unsigned char mode_3_regs[] = {
        0x67, 0x03, 0x00, 0x03, 0x00, 0x02, 0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81,
        0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50, 0x9C, 0x8E,
        0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x10, 0x0E, 0x00, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00};

    // 2. Ghi vào các Port I/O của VGA
    // Ghi Miscellaneous Output
    outb(0x3C2, mode_3_regs[0]);

    // Ghi Sequencer Registers
    for (uint8_t i = 0; i < 5; i++)
    {
        outb(0x3C4, i);
        outb(0x3C5, mode_3_regs[1 + i]);
    }

    // Ghi CRTC Registers
    outb(0x3D4, 0x03);
    outb(0x3D5, inb(0x3D5) | 0x80); // Unlock CRTC
    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & ~0x80);
    for (uint8_t i = 0; i < 25; i++)
    {
        outb(0x3D4, i);
        outb(0x3D5, mode_3_regs[6 + i]);
    }

    // Ghi Graphics Controller Registers
    for (uint8_t i = 0; i < 9; i++)
    {
        outb(0x3CE, i);
        outb(0x3CF, mode_3_regs[31 + i]);
    }

    // Ghi Attribute Controller Registers
    for (uint8_t i = 0; i < 21; i++)
    {
        inb(0x3DA); // Reset flip-flop
        outb(0x3C0, i);
        outb(0x3C0, mode_3_regs[40 + i]);
    }
    outb(0x3C0, 0x20); // Bật hiển thị

    // 3. Cuối cùng mới xóa buffer văn bản
    vga_clear();
}

void delay(uint32_t count)
{
    // Với CPU Pentium Gold/Core i3, khoảng 100,000,000 vòng lặp
    // tương đương gần 1 giây tùy vào xung nhịp.
    volatile uint32_t i;
    for (i = 0; i < count; i++)
    {
        __asm__("nop"); // Tránh compiler xóa vòng lặp
    }
}

void sys_shutdown()
{
    // 1. In log thông báo dừng hệ thống
    vga_puts("\n[ SYSTEM ] Stopping all services...\n", 0x0E); // Màu vàng

    // Nếu bạn có hệ thống AutoSave như trong shion.sc
    vga_puts("[ KERNEL ] Saving configuration to ISO_ROOT/apps/...\n", 0x0B);

    vga_puts("[ STOP   ] System halt signal sent.\n", 0x0C); // Màu đỏ
    vga_puts("Safe to power off or wait for auto-shutdown.\n", 0x0F);

    // 2. Đợi 1.5 giây (Ước lượng khoảng 150 triệu vòng lặp cho CPU hiện đại)
    // Bạn có thể điều chỉnh con số này cho khớp với máy thật của bạn
    delay(150000000);

    // 3. Thực hiện Shutdown

    // Thử ACPI trước (cho máy thật - cần parse FADT trước đó)
    // acpi_shutdown();

    // QEMU/Bochs "Magic" Shutdown
    outw(0xB004, 0x2000);

    // VirtualBox / QEMU phiên bản mới hơn
    outw(0x604, 0x2000);

    // 4. Nếu vẫn không tắt được (trên máy thật chưa có ACPI)
    vga_puts("\n[ ERROR  ] ACPI Power-off failed.\n", 0x0C);
    vga_puts("[ HALT   ] CPU Halted. Press Power Button.", 0x0F);

    // Ngắt toàn bộ để tránh lỗi hệ thống
    asm volatile("cli; hlt");
}

void shell_input(char c)
{
    if (c == '\n')
    {
        cmd_buffer[cmd_ptr] = '\0';
        execute_command(cmd_buffer);
        cmd_ptr = 0;
    }
    else if (c == '\b')
    {
        if (cmd_ptr > 0) // CHỈ xóa khi đã gõ ít nhất 1 ký tự
        {
            cmd_ptr--;
            // Gọi hàm xóa ký tự trên màn hình
            vga_putc('\b', 0x07);
        }
        // Nếu cmd_ptr == 0, ta lờ đi, không gọi vga_putc nữa -> Dấu ">" an toàn!
    }
    else if (cmd_ptr < 255)
    {
        cmd_buffer[cmd_ptr++] = c;
        vga_putc(c, 0x0F);
    }
}

/* --- 2. HỆ THỐNG LOG ĐƠN GIẢN --- */
void klog(const char *msg, uint8_t type)
{
    switch (type)
    {
    case 0:
        vga_puts("[ INFO  ] ", 0x0B);
        break; // Cyan
    case 1:
        vga_puts("[  OK   ] ", 0x0A);
        break; // Green
    case 2:
        vga_puts("[ ERROR ] ", 0x0C);
        break; // Red
    default:
        vga_puts("[ LOG   ] ", 0x07);
        break;
    }
    vga_puts(msg, 0x0F);
    vga_puts("\n", 0x0F);
}

// Hàm này sẽ được gọi từ keyboard_isr_wrapper trong interrupt.S

void shell_ls()
{
    // Gọi trực tiếp hàm bạn đã viết trong iso9660.c
    iso_list_directory("/");
}

void pic_remap()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28); // Master offset 32, Slave 40
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi)
{
    reset_to_text_mode();
    vga_init();
    vga_puts("--- openYanase Kernel Core Init ---\n", 0x0E);

    if (magic != 0x2BADB002)
    {
        klog("Invalid Multiboot magic number!", 2);
    }
    else
    {
        klog("Multiboot magic verified.", 1);
    }

    global_mbi = mbi;

    klog("Installing GDT...", 0);
    gdt_install();

    klog("Installing IDT & Remapping PIC...", 0);
    idt_install();
    pic_remap();

    heap_init(0x1000000, 4 * 1024 * 1024);
    klog("Heap initialized at 0x1000000", 1);

    // --- BẮT ĐẦU NẠP INITRAMDISK ---
    klog("Checking for Initramdisk modules...", 0);

    // Kiểm tra xem Bootloader có nạp Module nào không (Bit 3 của flags)
    if (mbi->flags & (1 << 3) && mbi->mods_count > 0)
    {
        initrd_init(mbi); // Hàm này sẽ gán địa chỉ initrd từ mods_addr
        initrd_ready = 1;
        klog("Initrd system initialized from RAM.", 1);

        // Bạn có thể in thử danh sách file để chắc chắn .tar đã được nạp
        // initrd_list();
    }
    else
    {
        klog("ERROR: initrd-x86.tar not found! Bootloader failed to load modules.", 2);
        vga_puts("System may be unstable without ramdisk assets.\n", 0x0C);
        initrd_ready = 0;
    }

    uint32_t conf_sz = 0;
    // Thử tìm cả hai cách nếu vẫn lỗi
    char *conf = (char *)initrd_read_file("init.yc", &conf_sz);

    if (conf)
    {
        klog("Init: Found init.yc", 1);
        parse_yanase_config(conf, conf_sz);
    }
    else
    {
        klog("Init: init.yc NOT FOUND", 2);
        // In danh sách file đang có trong RAM để đối chiếu tên chính xác
        initrd_list();
    }

    klog("Initializing PS/2 Mouse...", 0);

    fs_ready = iso9660_mount();
    if (fs_ready)
    {
        vga_puts("ISO9660 File System Mounted.\n", 0x0A);
    }

    klog("Kernel Core ready. Interrupts enabling...", 1);

    // --- BƯỚC QUAN TRỌNG NHẤT ---
    asm volatile("sti"); // Kích hoạt ngắt toàn cục
    vga_puts("\nSystem stabilized. Type 'startx' or wait for GUI launch.\n> ", 0x0F);

    while (1)
    {
        if (command_ready)
        {
            // Khi có lệnh, xử lý ngay
            execute_command(shell_buffer);

            // Reset trạng thái để chờ lệnh tiếp theo
            shell_ptr = 0;
            command_ready = 0;

            vga_puts("\n> ", WHITE);
        }
        else
        {
            // Nếu không có việc gì làm, CPU nghỉ ngơi cho mát máy
            asm volatile("hlt");
        }
    }
}