#include "jar.h"
#include "../fs/iso9660.h"
#include "../memory/heap.h"
#include "../graphics/vesa.h"

// Định nghĩa header của định dạng ZIP (Local File Header)
typedef struct {
    uint32_t signature;      // 0x04034b50 ("PK\3\4")
    uint16_t version;
    uint16_t flags;
    uint16_t compression;    // 0 = Store, 8 = Deflate
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
} __attribute__((packed)) zip_header_t;

extern multiboot_info_t* global_mbi;

/**
 * Hàm nạp và thực thi file JAR
 */
int jar_execute(const char* path) {
    klog("JAR Runtime: Loading file...", 0);
    
    // 1. Đọc file JAR từ ISO9660 vào RAM
    uint32_t file_size = iso9660_get_size(path);
    if (file_size == 0) {
        klog("JAR Runtime: File not found!", 2);
        return -1;
    }

    uint8_t* jar_buffer = (uint8_t*)kmalloc(file_size);
    iso9660_read_file(path, jar_buffer);

    // 2. Kiểm tra tính hợp lệ (Header PK)
    zip_header_t* header = (zip_header_t*)jar_buffer;
    if (header->signature != 0x04034b50) {
        klog("JAR Runtime: Invalid JAR signature!", 2);
        kfree(jar_buffer);
        return -1;
    }

    // 3. Quét các entry bên trong JAR
    // Ở giai đoạn này, bạn cần tìm file "META-INF/MANIFEST.MF" 
    // hoặc quét các file ".class"
    jar_scan_entries(jar_buffer, file_size);

    return 0;
}

// Thêm vào cuối src/runtime/jar.c
void jar_scan_entries(uint8_t* buffer, uint32_t size) {
    (void)buffer; // Tránh báo lỗi unused variable
    (void)size;
    klog("JAR: Entry scanner initialized.", 0x0B);
}