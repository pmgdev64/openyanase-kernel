#ifndef JAR_H
#define JAR_H

#include "../types.h"
#include "bytecode.h"

// ─── ZIP signatures ───────────────────────────────────────────────────────────
#define ZIP_LOCAL_HEADER_SIG  0x04034b50
#define ZIP_CENTRAL_DIR_SIG   0x02014b50
#define ZIP_END_OF_CENTRAL    0x06054b50

// ─── ZIP Local File Header ────────────────────────────────────────────────────
typedef struct {
    uint32_t signature;
    uint16_t version;
    uint16_t flags;
    uint16_t compression;       // 0=Store, 8=Deflate (chỉ hỗ trợ 0)
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
} __attribute__((packed)) zip_local_header_t;

// ─── Java .class magic ────────────────────────────────────────────────────────
#define CLASS_MAGIC 0xCAFEBABE

// ─── .class File Header ───────────────────────────────────────────────────────
typedef struct {
    uint32_t magic;
    uint16_t minor_version;
    uint16_t major_version;
    uint16_t constant_pool_count;
} __attribute__((packed)) class_file_header_t;

// ─── API ──────────────────────────────────────────────────────────────────────
int  jar_execute(const char* path);
void jar_scan_entries(uint8_t* buffer, uint32_t size);
void parse_manifest_for_main(uint8_t* data, uint32_t size, char* out_main);

// Tìm entry trong JAR theo tên, trả về pointer đến data và size
uint8_t* jar_find_entry(uint8_t* buffer, uint32_t jar_size,
                        const char* name, uint32_t* out_size);

// Parse .class và chạy method main()
int class_execute(uint8_t* class_data, uint32_t class_size);

#endif