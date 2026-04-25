#ifndef INITRD_H
#define INITRD_H

#include "../types.h"
#include "../graphics/vesa.h"

// Định nghĩa cấu trúc Header của file TAR (UStar)
// Định dạng này cực kỳ hữu ích vì nó không nén, dễ dàng truy xuất trực tiếp trong RAM
typedef struct {
    char name[100];     // Tên file
    char mode[8];       // Quyền truy cập
    char uid[8];        // User ID
    char gid[8];        // Group ID
    char size[12];      // Kích thước file (Hệ bát phân - Octal)
    char mtime[12];     // Thời gian sửa đổi
    char chksum[8];     // Checksum
    char typeflag;      // Loại file (0 hoặc '0' là file thường)
    char linkname[100];
    char magic[6];      // Phải là "ustar"
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
} __attribute__((packed)) tar_header_t;

/**
 * Khởi tạo Initramdisk từ Multiboot Information
 * @param mbi Con trỏ đến cấu trúc multiboot nạp bởi bootloader
 */
void initrd_init(multiboot_info_t* mbi);

/**
 * Tìm kiếm và trả về nội dung file trong Ramdisk
 * @param filename Tên file cần tìm (bao gồm đường dẫn nếu có trong tar)
 * @param out_size Con trỏ để nhận kích thước file tìm được
 * @return Con trỏ đến vùng dữ liệu trong RAM, hoặc NULL nếu không thấy
 */
void* initrd_read_file(const char* filename, uint32_t* out_size);

/**
 * Liệt kê toàn bộ file có trong Initrd (Dùng cho lệnh ls_rd)
 */
void initrd_list();

#endif // INITRD_H