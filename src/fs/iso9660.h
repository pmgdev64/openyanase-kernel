#ifndef ISO9660_H
#define ISO9660_H

#include "types.h" // PHẢI CÓ DÒNG NÀY

// Thêm các define quan trọng
#define ISO_PVD_LBA 16
#define ISO_SECTOR_SIZE 2048

typedef struct {
    uint8_t type;                       // 1 = PVD
    char id[5];                         // "CD001"
    uint8_t version;                    // 1
    uint8_t unused1;
    char system_id[32];
    char volume_id[32];
    uint8_t unused2[8];
    uint32_t volume_space_size_lba;     // Little Endian
    uint32_t volume_space_size_msb;     // Big Endian (Bỏ qua)
    uint8_t unused3[32];
    uint16_t volume_set_size_lba;
    uint16_t volume_set_size_msb;
    uint16_t sequence_number_lba;
    uint16_t sequence_number_msb;
    uint16_t logical_block_size_lba;    // Thường là 2048 bytes
    uint16_t logical_block_size_msb;
    uint32_t path_table_size_lba;
    uint32_t path_table_size_msb;
    uint32_t path_table_lba;
    uint32_t path_table_optional_lba;
    uint32_t path_table_msb;
    uint32_t path_table_optional_msb;
    uint8_t root_directory_record[34];  // Root directory record
    // ... còn các trường mô tả thời gian phía sau nhưng chưa cần ngay
} __attribute__((packed)) iso_pvd_t;

typedef struct {
    uint8_t length;                     // Độ dài của bản ghi này
    uint8_t extended_attribute_length;
    uint32_t extent_lba;                // LBA của file/thư mục (Little Endian)
    uint32_t extent_lba_msb;            // (Big Endian)
    uint32_t data_length;               // Kích thước file (Little Endian)
    uint32_t data_length_msb;           // (Big Endian)
    uint8_t recording_date[7];
    uint8_t file_flags;
    uint8_t file_unit_size;
    uint8_t interleave_gap_size;
    uint16_t volume_sequence_number;
    uint16_t volume_sequence_number_msb;
    uint8_t file_id_length;             // Độ dài tên file
    char file_id[1];                    // Tên file thực tế (địa chỉ bắt đầu)
} __attribute__((packed)) iso_directory_record_t;

// THÊM CÁC DÒNG NÀY VÀO CUỐI FILE H
int iso9660_mount();
uint32_t iso9660_get_size(const char* path);
// Đảm bảo trong file .h khai báo như thế này:
void* iso9660_read_file(const char* path, uint8_t* buffer, uint32_t* out_size);
void iso_list_directory(const char* path);

#endif // PHẢI CÓ DÒNG NÀY Ở CUỐI FILE