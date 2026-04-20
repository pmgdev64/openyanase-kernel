#ifndef JAR_H
#define JAR_H

#include "../types.h"

// Định nghĩa signature cho ZIP
#define ZIP_LOCAL_HEADER_SIG 0x04034b50
#define ZIP_CENTRAL_DIR_SIG  0x02014b50

/**
 * Cấu trúc đại diện cho một file bên trong JAR sau khi quét
 */
typedef struct {
    char name[256];
    uint32_t offset;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t compression_method;
} jar_entry_t;

/**
 * Khởi động và thực thi một file JAR từ đường dẫn hệ thống
 * @param path Đường dẫn đến file .jar trên ISO9660
 * @return 0 nếu thành công, < 0 nếu thất bại
 */
int jar_execute(const char* path);

/**
 * Tìm kiếm class chính (Main-Class) và các file .class cần thiết
 * (Sẽ được gọi nội bộ bên trong jar.c)
 */
void jar_scan_entries(uint8_t* buffer, uint32_t size);

#endif