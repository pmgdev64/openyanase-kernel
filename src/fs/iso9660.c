#include "fs/iso9660.h"
#include "io.h"
#include "vga_buffer.h"

// Biến toàn cục để lưu bus port sau khi mount thành công
static uint16_t current_atapi_base = 0x1F0; 

#define ISO_SECTOR_SIZE 2048

/* --- ATAPI LOW LEVEL DRIVER --- */

// Đợi bit trạng thái của Controller
static int atapi_wait_status(uint16_t base, uint8_t mask, uint8_t value, uint32_t timeout) {
    while (timeout--) {
        uint8_t status = inb(base + 7);
        if ((status & mask) == value) return 1;
    }
    return 0;
}

// Đọc 1 sector (2048 bytes) từ đĩa quang
void atapi_read_sector(uint32_t lba, uint8_t* buffer) {
    uint16_t base = current_atapi_base;

    // 1. Chọn Drive và đợi hết BSY
    outb(base + 6, 0xA0); 
    atapi_wait_status(base, 0x80, 0x00, 1000000);

    // 2. Thiết lập kích thước nhận dữ liệu (2048 bytes)
    outb(base + 1, 0); // Features
    outb(base + 4, (ISO_SECTOR_SIZE & 0xFF));
    outb(base + 5, (ISO_SECTOR_SIZE >> 8));
    outb(base + 7, 0xA0); // Lệnh ATAPI Packet

    // 3. Đợi DRQ để gửi SCSI Packet
    atapi_wait_status(base, 0x08, 0x08, 1000000);

    uint8_t scsi_packet[12] = {0};
    scsi_packet[0] = 0xA8;               // READ(12)
    scsi_packet[2] = (lba >> 24) & 0xFF; // MSB của LBA
    scsi_packet[3] = (lba >> 16) & 0xFF;
    scsi_packet[4] = (lba >> 8) & 0xFF;
    scsi_packet[5] = lba & 0xFF;         // LSB của LBA
    scsi_packet[9] = 1;                  // Đọc 1 sector

    // Gửi 12 bytes packet (6 words)
    uint16_t* ptr = (uint16_t*)scsi_packet;
    for (int i = 0; i < 6; i++) outw(base, ptr[i]);

    // 4. Đợi Controller xử lý xong và có dữ liệu (DRQ)
    atapi_wait_status(base, 0x80, 0x00, 1000000);
    atapi_wait_status(base, 0x08, 0x08, 1000000);

    // 5. Đọc dữ liệu từ Data Port (ISO_SECTOR_SIZE / 2 = 1024 words)
    uint16_t* b = (uint16_t*)buffer;
    for (int i = 0; i < 1024; i++) {
        b[i] = inw(base);
    }
}

/* --- ISO9660 HELPERS --- */

static int filename_match(const char* target, const char* iso_name, uint8_t len) {
    for (int i = 0; i < len; i++) {
        if (iso_name[i] == ';') break; // Bỏ qua version số của ISO9660
        if (target[i] != iso_name[i]) return 0;
    }
    return 1;
}

static int find_file_record(const char* path, iso_directory_record_t* out_rec) {
    static uint8_t buf[ISO_SECTOR_SIZE];
    const char* filename = (path[0] == '/') ? path + 1 : path;

    // Đọc Primary Volume Descriptor (Sector 16)
    atapi_read_sector(16, buf);
    iso_pvd_t* pvd = (iso_pvd_t*)buf;
    
    // Lấy bản ghi thư mục gốc
    iso_directory_record_t* root = (iso_directory_record_t*)pvd->root_directory_record;
    uint32_t root_lba = root->extent_lba;

    // Đọc sector chứa Root Directory
    atapi_read_sector(root_lba, buf);

    uint32_t offset = 0;
    while (offset < ISO_SECTOR_SIZE) {
        iso_directory_record_t* rec = (iso_directory_record_t*)&buf[offset];
        if (rec->length == 0) break;

        if (filename_match(filename, rec->file_id, rec->file_id_length)) {
            // Sao chép bản ghi tìm thấy
            for(int i=0; i < rec->length; i++) ((uint8_t*)out_rec)[i] = buf[offset + i];
            return 1;
        }
        offset += rec->length;
    }
    return 0;
}

/* --- PUBLIC API --- */
int iso9660_mount() {
    uint16_t bus_ports[] = {0x1F0, 0x170};
    uint8_t drives[] = {0xA0, 0xB0};

    vga_puts("FS: Scanning ATAPI devices...\n", 0x07);

    for (int b = 0; b < 2; b++) {
        uint16_t base = bus_ports[b];
        for (int d = 0; d < 2; d++) {
            // 1. Chọn Drive Master/Slave
            outb(base + 6, drives[d]);
            for(int i=0; i<1000; i++) inb(base + 7); // Delay

            // 2. Gửi lệnh Soft Reset (ATAPI cần reset để báo Signature chính xác)
            outb(base + 7, 0x08); 
            for(int i=0; i<2000; i++) inb(base + 7);

            // 3. Đợi Busy xóa (BSY=0, DRQ=0)
            uint32_t timeout = 100000;
            while ((inb(base + 7) & 0x80) && --timeout);

            // 4. Đọc Signature từ LBA Mid và LBA High
            uint8_t cl = inb(base + 4); 
            uint8_t ch = inb(base + 5); 

            // ATAPI Signature: CL=0x14, CH=0xEB
            if (cl == 0x14 && ch == 0xEB) {
                current_atapi_base = base;
                vga_puts("FS: ATAPI Found on ", 0x0A);
                vga_puts(base == 0x1F0 ? "Primary " : "Secondary ", 0x0A);
                vga_puts(drives[d] == 0xA0 ? "Master\n" : "Slave\n", 0x0A);
                return 1;
            }
        }
    }
    vga_puts("FS Error: No ATAPI CD-ROM detected.\n", 0x0C);
    return 0;
}

void iso_list_directory(const char* path) {
    static uint8_t buf[ISO_SECTOR_SIZE];
    
    // Đọc PVD để tìm Root Directory
    atapi_read_sector(16, buf);
    iso_pvd_t* pvd = (iso_pvd_t*)buf;

    if (pvd->type != 1 || pvd->id[0] != 'C' || pvd->id[1] != 'D') {
        vga_puts("FS Error: Invalid ISO9660 PVD\n", 0x0C);
        return;
    }

    iso_directory_record_t* root = (iso_directory_record_t*)pvd->root_directory_record;
    uint32_t current_lba = root->extent_lba;
    uint32_t total_len = root->data_length;
    
    vga_puts("Index of /:\n", 0x0B);

    // Duyệt qua từng sector nếu thư mục lớn
    for (uint32_t s = 0; s < (total_len + 2047) / 2048; s++) {
        atapi_read_sector(current_lba + s, buf);
        uint32_t offset = 0;

        while (offset < ISO_SECTOR_SIZE) {
            iso_directory_record_t* rec = (iso_directory_record_t*)&buf[offset];
            if (rec->length == 0) break; // Hết bản ghi trong sector này

            vga_puts("  ", 0x07);
            if (rec->file_id[0] == 0) vga_puts(".", 0x07);
            else if (rec->file_id[0] == 1) vga_puts("..", 0x07);
            else {
                for (int i = 0; i < rec->file_id_length; i++) {
                    if (rec->file_id[i] == ';') break; // Bỏ versioning
                    vga_putc(rec->file_id[i], 0x0F);
                }
            }

            // Phân biệt Folder/File bằng flags (bit 1)
            if (rec->file_flags & 0x02) vga_puts(" [DIR]\n", 0x0B);
            else vga_puts("\n", 0x07);

            offset += rec->length;
        }
    }
}

/* --- PUBLIC API --- */

uint32_t iso9660_get_size(const char* path) {
    iso_directory_record_t rec;
    if (find_file_record(path, &rec)) {
        return rec.data_length;
    }
    return 0;
}

void* iso9660_read_file(const char* path, uint8_t* buffer, uint32_t* out_size) {
    iso_directory_record_t rec;
    
    // SỬA: return NULL (hoặc 0) thay vì return trống
    if (!find_file_record(path, &rec)) return 0;

    uint32_t lba = rec.extent_lba;
    uint32_t size = rec.data_length;
    uint32_t sectors = (size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;

    // Gán size ra ngoài để hàm cat biết đường mà in
    if (out_size) *out_size = size;

    for (uint32_t i = 0; i < sectors; i++) {
        atapi_read_sector(lba + i, buffer + (i * ISO_SECTOR_SIZE));
    }

    // SỬA: Trả về buffer chứa dữ liệu để f_ptr nhận được giá trị
    return (void*)buffer;
}