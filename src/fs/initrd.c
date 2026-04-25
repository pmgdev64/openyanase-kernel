#include "initrd.h"
#include "../memory/heap.h"
#include "../vga_buffer.h" // Để dùng vga_puts

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;    // Con trỏ tới tên module hoặc command line
    uint32_t reserved;
} multiboot_module_t;

static uint32_t initrd_addr = 0;
static uint32_t initrd_size = 0;

void initrd_init(multiboot_info_t* mbi) {
    // Kiểm tra bit 3 trong flags để biết có module hay không
    if (!(mbi->flags & (1 << 3))) {
        klog("Initrd: No modules found.", 2);
        return;
    }

    if (mbi->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbi->mods_addr;
        
        // Giả sử module đầu tiên là Ramdisk của chúng ta
        initrd_addr = mod[0].mod_start;
        initrd_size = mod[0].mod_end - mod[0].mod_start;

        klog("Initrd: Mounted at physical RAM.", 1);
        // Debug thông tin
        // klog((char*)mod[0].string, 0); 
    }
}
// Helper: Chuyển Octal (hệ 8) của TAR sang Integer
// Vì TAR header lưu kích thước file dưới dạng chuỗi "00000001234"
static uint32_t tar_get_size(const char *in) {
    uint32_t size = 0;
    for (int j = 0; j < 11; j++) {
        if (in[j] < '0' || in[j] > '7') break;
        size = size * 8 + (in[j] - '0');
    }
    return size;
}

// Hàm liệt kê toàn bộ file có trong Ramdisk
void initrd_list() {
    if (initrd_addr == 0 || initrd_size == 0) {
        vga_puts("  (Ramdisk empty or not initialized)\n", 0x07);
        return;
    }

    uint32_t address = initrd_addr;
    
    while (address < initrd_addr + initrd_size) {
        tar_header_t* header = (tar_header_t*)address;

        // Nếu gặp header rỗng (tên file trống) -> Hết archive
        if (header->name[0] == '\0') break;

        uint32_t size = tar_get_size(header->size);

        // In tên file
        vga_puts("  ", 0x07);
        vga_puts(header->name, 0x0F); // Trắng
        
        // In kích thước (Tạm thời in chữ DATA cho nhanh, hoặc hex nếu bạn có vga_put_hex)
        vga_puts("  [SIZE: ", 0x07);
        // Nếu bạn muốn in số thực, cần hàm vga_put_decimal, 
        // ở đây mình in dấu hiệu nhận biết file
        if (header->typeflag == '0' || header->typeflag == 0) vga_puts("FILE", 0x0A);
        else if (header->typeflag == '5') vga_puts("DIR", 0x0B);
        
        vga_puts("]\n", 0x07);

        // Nhảy đến header tiếp theo:
        // Header (512) + Dữ liệu (làm tròn lên bội số của 512)
        address += ((size + 511) & ~511) + 512;
    }
}

// Hàm so sánh tên file thông minh
static int tar_match_filename(const char* tar_name, const char* search_name) {
    // Trường hợp 1: Tên trong TAR là "./init.yc" và tìm "init.yc"
    if (tar_name[0] == '.' && tar_name[1] == '/') {
        const char* stripped_tar = tar_name + 2;
        if (vga_strncmp(stripped_tar, search_name, 100) == 0) return 1;
    }
    
    // Trường hợp 2: So sánh trực tiếp (khớp hoàn toàn)
    if (vga_strncmp(tar_name, search_name, 100) == 0) return 1;

    return 0;
}

void* initrd_read_file(const char* filename, uint32_t* out_size) {
    if (initrd_addr == 0) return (void*)0;

    uint32_t address = initrd_addr;
    while (address < initrd_addr + initrd_size) {
        tar_header_t* header = (tar_header_t*)address;
        
        // Kiểm tra Magic để đảm bảo là header TAR hợp lệ (UStar)
        if (vga_strncmp(header->magic, "ustar", 5) != 0 && header->name[0] == '\0') {
            break; 
        }

        uint32_t size = tar_get_size(header->size);

        // Sử dụng hàm so sánh thông minh
        if (tar_match_filename(header->name, filename)) {
            *out_size = size;
            return (void*)(address + 512); 
        }

        // Nhảy qua file: 512 (header) + dữ liệu (đã căn lề 512)
        address += ((size + 511) & ~511) + 512;
    }
    return (void*)0;
}