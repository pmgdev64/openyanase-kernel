#include "fs/iso9660.h"
#include "io.h"
#include "vga_buffer.h"

#define ATAPI_DATA       0x1F0
#define ATAPI_ERROR      0x1F1
#define ATAPI_FEATURES   0x1F1
#define ATAPI_IREASON    0x1F2
#define ATAPI_SAMTAG     0x1F2
#define ATAPI_COUNT_LOW  0x1F4
#define ATAPI_COUNT_HIGH 0x1F5
#define ATAPI_DRIVE_SEL  0x1F6
#define ATAPI_COMMAND    0x1F7
#define ATAPI_STATUS     0x1F7

// Sector size của ISO9660 luôn là 2048 bytes
#define ISO_SECTOR_SIZE 2048

// Trình điều khiển ATAPI đơn giản để đọc 1 sector (LBA)
void atapi_read_sector(uint32_t lba, uint8_t* buffer) {
    // 1. Chọn ổ đĩa (0xA0 là Master, 0xB0 là Slave)
    outb(ATAPI_DRIVE_SEL, 0xA0); 
    
    // 2. Thiết lập chuẩn ATAPI (không dùng DMA)
    outb(ATAPI_FEATURES, 0);
    outb(ATAPI_COUNT_LOW, (ISO_SECTOR_SIZE & 0xFF));
    outb(ATAPI_COUNT_HIGH, (ISO_SECTOR_SIZE >> 8));
    
    // 3. Gửi lệnh ATAPI Packet
    outb(ATAPI_COMMAND, 0xA0); 

    // Chờ ổ đĩa sẵn sàng nhận packet
    while (inb(ATAPI_STATUS) & 0x80); // Busy bit
    while (!(inb(ATAPI_STATUS) & 0x08)); // DRQ bit

    // 4. Gửi SCSI Packet: Lệnh READ (12) - mã 0xA8
    uint8_t packet[12] = {0};
    packet[0] = 0xA8; 
    packet[2] = (lba >> 24) & 0xFF;
    packet[3] = (lba >> 16) & 0xFF;
    packet[4] = (lba >> 8) & 0xFF;
    packet[5] = lba & 0xFF;
    packet[9] = 1; // Đọc 1 sector

    uint16_t* p = (uint16_t*)packet;
    for (int i = 0; i < 6; i++) {
        outw(ATAPI_DATA, p[i]);
    }

    // 5. Chờ dữ liệu trả về
    while (inb(ATAPI_STATUS) & 0x80);
    while (!(inb(ATAPI_STATUS) & 0x08));

    // 6. Đọc dữ liệu từ Data Port vào buffer
    uint16_t* b = (uint16_t*)buffer;
    for (int i = 0; i < (ISO_SECTOR_SIZE / 2); i++) {
        b[i] = inw(ATAPI_DATA);
    }
}

// Hàm so sánh tên file (ISO9660 thường có hậu tố ;1)
static int filename_match(const char* target, const char* iso_name, uint8_t len) {
    for (int i = 0; i < len; i++) {
        if (iso_name[i] == ';') break; // Bỏ qua version ;1
        if (target[i] != iso_name[i]) return 0;
    }
    return 1;
}

uint32_t iso9660_get_size(const char* path) {
    // Logic tìm size file
    return 0; 
}

void iso9660_read_file(const char* path, uint8_t* buffer) {
    // Logic đọc file
}

void iso_list_directory(const char* path) {
    uint8_t buf[ISO_SECTOR_SIZE];
    
    // Đọc PVD tại Sector 16
    atapi_read_sector(16, buf);
    iso_pvd_t* pvd = (iso_pvd_t*)buf;

    if (pvd->id[0] != 'C' || pvd->id[1] != 'D') {
        vga_puts("Error: Not a valid ISO9660 disk.\n", 0x0C);
        return;
    }

    // Lấy Root Directory Record
    iso_directory_record_t* root = (iso_directory_record_t*)pvd->root_directory_record;
    uint32_t root_lba = root->extent_lba;

    // Đọc sector của Root Directory
    atapi_read_sector(root_lba, buf);
    
    vga_puts("Listing Files in Root:\n", 0x0B);

    uint32_t offset = 0;
    while (offset < ISO_SECTOR_SIZE) {
        iso_directory_record_t* rec = (iso_directory_record_t*)&buf[offset];
        if (rec->length == 0) break;

        // In tên file (file_id)
        if (rec->file_id[0] == 0) vga_puts(".", 0x07);
        else if (rec->file_id[0] == 1) vga_puts("..", 0x07);
        else {
            for (int i = 0; i < rec->file_id_length; i++) {
                vga_putc(rec->file_id[i], 0x07);
            }
        }
        vga_puts("  ", 0x07);

        offset += rec->length;
    }
    vga_puts("\n", 0x07);
}

void* iso_load_file(const char* filename) {
    uint8_t buf[ISO_SECTOR_SIZE];
    atapi_read_sector(16, buf);
    iso_pvd_t* pvd = (iso_pvd_t*)buf;

    iso_directory_record_t* root = (iso_directory_record_t*)pvd->root_directory_record;
    uint32_t root_lba = root->extent_lba;

    atapi_read_sector(root_lba, buf);

    uint32_t offset = 0;
    while (offset < ISO_SECTOR_SIZE) {
        iso_directory_record_t* rec = (iso_directory_record_t*)&buf[offset];
        if (rec->length == 0) break;

        if (filename_match(filename, rec->file_id, rec->file_id_length)) {
            // Found it! Trả về LBA của file (để hàm bên ngoài tự load tiếp tùy dung lượng)
            return (void*)rec->extent_lba;
        }
        offset += rec->length;
    }
    return (void*)0;
}