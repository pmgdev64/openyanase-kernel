// src/runtime/jar.c
#include "jar.h"
#include "bytecode.h"
#include "../fs/initrd.h"
#include "../memory/heap.h"
#include "../graphics/vesa.h"
#include "../vga_buffer.h"

// ─── Helpers: Big-Endian đọc (Java dùng big-endian) ──────────────────────────
static inline uint16_t be16(uint8_t* p) { return ((uint16_t)p[0]<<8)|p[1]; }
static inline uint32_t be32(uint8_t* p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }

// ─── String compare helper ────────────────────────────────────────────────────
static int str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}

static int strn_eq(const char* a, const uint8_t* b, uint16_t blen, const char* expect) {
    // So sánh UTF8 entry với chuỗi C
    uint16_t elen = 0;
    while (expect[elen]) elen++;
    if (blen != elen) return 0;
    for (uint16_t i = 0; i < blen; i++)
        if (b[i] != (uint8_t)expect[i]) return 0;
    return 1;
}

// ─── 1. Tìm entry trong JAR ───────────────────────────────────────────────────
uint8_t* jar_find_entry(uint8_t* buf, uint32_t jar_size,
                        const char* name, uint32_t* out_size) {
    uint32_t offset = 0;
    uint16_t name_len = 0;
    while (name[name_len]) name_len++;

    while (offset + sizeof(zip_local_header_t) < jar_size) {
        zip_local_header_t* h = (zip_local_header_t*)(buf + offset);
        if (h->signature != ZIP_LOCAL_HEADER_SIG) break;

        uint8_t* entry_name = buf + offset + sizeof(zip_local_header_t);
        uint32_t data_off   = offset + sizeof(zip_local_header_t)
                            + h->filename_len + h->extra_len;

        // So tên
        if (h->filename_len == name_len) {
            int match = 1;
            for (uint16_t i = 0; i < name_len; i++)
                if (entry_name[i] != (uint8_t)name[i]) { match=0; break; }
            if (match) {
                if (h->compression != 0) {
                    klog("JAR: Deflate not supported, use Store only", 2);
                    return 0;
                }
                *out_size = h->uncompressed_size;
                return buf + data_off;
            }
        }

        offset = data_off + h->compressed_size;
    }
    return 0;
}

// ─── 2. Parse Constant Pool ───────────────────────────────────────────────────
// Trả về số byte tiêu thụ, điền vào cp[]
static uint32_t parse_constant_pool(uint8_t* data, uint16_t count, cp_entry_t* cp) {
    uint32_t off = 0;
    // cp[0] không dùng trong Java spec
    for (uint16_t i = 1; i < count; i++) {
        uint8_t tag = data[off++];
        cp[i].tag = tag;
        switch (tag) {
        case CP_UTF8: {
            uint16_t len = be16(data+off); off+=2;
            cp[i].utf8.length = len;
            cp[i].utf8.bytes  = data+off;  // trỏ thẳng vào buffer
            off += len;
            break;
        }
        case CP_INTEGER:
            cp[i].integer.value = (int32_t)be32(data+off); off+=4; break;
        case CP_FLOAT:
            cp[i].integer.value = (int32_t)be32(data+off); off+=4; break;
        case CP_LONG: case CP_DOUBLE:
            off+=8; i++; // Long/Double chiếm 2 slot
            break;
        case CP_CLASS:
            cp[i].class_ref.index = be16(data+off); off+=2; break;
        case CP_STRING:
            cp[i].string_ref.index = be16(data+off); off+=2; break;
        case CP_FIELDREF:
        case CP_METHODREF:
        case CP_INTERFACE_MREF:
            cp[i].ref.class_i = be16(data+off); off+=2;
            cp[i].ref.nat_i   = be16(data+off); off+=2;
            break;
        case CP_NAME_AND_TYPE:
            cp[i].nat.name_i = be16(data+off); off+=2;
            cp[i].nat.desc_i = be16(data+off); off+=2;
            break;
        default:
            klog("JAR: Unknown CP tag", 2);
            return off;
        }
    }
    return off;
}

// ─── 3. Parse và execute .class ───────────────────────────────────────────────
int class_execute(uint8_t* data, uint32_t size) {
    if (size < 10) return -1;

    // Kiểm tra magic 0xCAFEBABE
    uint32_t magic = be32(data);
    if (magic != CLASS_MAGIC) {
        klog("JAR: Invalid class magic", 2);
        return -1;
    }

    // Minor(2) + Major(2) + cp_count(2)
    uint16_t cp_count = be16(data+8);
    if (cp_count > 512) {
        klog("JAR: CP too large", 2);
        return -1;
    }

    // Cấp phát CP trên heap
    cp_entry_t* cp = (cp_entry_t*)kmalloc(cp_count * sizeof(cp_entry_t));
    if (!cp) { klog("JAR: OOM for CP", 2); return -1; }

    // Parse CP bắt đầu từ offset 10
    uint32_t off = 10;
    off += parse_constant_pool(data+off, cp_count, cp);

    // Bỏ qua: access_flags(2) + this(2) + super(2) + interfaces_count(2)
    // + interfaces(2*n) + fields_count(2)
    if (off+8 > size) goto done;
    uint16_t access    = be16(data+off); off+=2;
    uint16_t this_cls  = be16(data+off); off+=2;
    uint16_t super_cls = be16(data+off); off+=2;
    uint16_t iface_cnt = be16(data+off); off+=2;
    off += iface_cnt * 2; // skip interfaces

    if (off+2 > size) goto done;
    uint16_t field_cnt = be16(data+off); off+=2;
    // Skip fields
    for (uint16_t f = 0; f < field_cnt; f++) {
        if (off+8 > size) goto done;
        off+=6; // access+name+desc
        uint16_t attr_cnt = be16(data+off); off+=2;
        for (uint16_t a = 0; a < attr_cnt; a++) {
            if (off+6 > size) goto done;
            off+=2; // name_index
            uint32_t alen = be32(data+off); off+=4+alen;
        }
    }

    // ── Parse methods ────────────────────────────────────────────────────────
    if (off+2 > size) goto done;
    uint16_t method_cnt = be16(data+off); off+=2;

    for (uint16_t m = 0; m < method_cnt; m++) {
        if (off+8 > size) goto done;
        uint16_t m_access = be16(data+off); off+=2;
        uint16_t m_name   = be16(data+off); off+=2;
        uint16_t m_desc   = be16(data+off); off+=2;
        uint16_t m_attrs  = be16(data+off); off+=2;

        // Kiểm tra tên method có phải "main" không
        int is_main = 0;
        if (m_name < cp_count && cp[m_name].tag == CP_UTF8)
            is_main = strn_eq(0, cp[m_name].utf8.bytes,
                              cp[m_name].utf8.length, "main");

        uint8_t* code_ptr = 0;
        uint32_t code_len = 0;
        uint16_t max_stack = 0, max_locals = 0;

        for (uint16_t a = 0; a < m_attrs; a++) {
            if (off+6 > size) goto done;
            uint16_t a_name = be16(data+off); off+=2;
            uint32_t a_len  = be32(data+off); off+=4;

            // Tìm attribute "Code"
            int is_code = 0;
            if (a_name < cp_count && cp[a_name].tag == CP_UTF8)
                is_code = strn_eq(0, cp[a_name].utf8.bytes,
                                  cp[a_name].utf8.length, "Code");

            if (is_code && a_len >= 8) {
                max_stack  = be16(data+off);
                max_locals = be16(data+off+2);
                code_len   = be32(data+off+4);
                code_ptr   = data+off+8;
            }
            off += a_len;
        }

        // Nếu là main() và có code → thực thi
        if (is_main && code_ptr && code_len > 0) {
            klog("JAR: Found main(), executing...", 1);
            execute_bytecode(code_ptr, code_len, cp, cp_count);
            kfree(cp);
            return 0;
        }
    }

    klog("JAR: No main() method found", 2);
done:
    kfree(cp);
    return -1;
}

// ─── 4. Scan entries + tìm main class ────────────────────────────────────────
void jar_scan_entries(uint8_t* buffer, uint32_t size) {
    char main_class[128] = {0};
    char tmp_name[128];

    // Tìm MANIFEST.MF trước
    uint32_t offset = 0;
    while (offset + sizeof(zip_local_header_t) < size) {
        zip_local_header_t* h = (zip_local_header_t*)(buffer+offset);
        if (h->signature != ZIP_LOCAL_HEADER_SIG) break;

        char* raw = (char*)(buffer+offset+sizeof(zip_local_header_t));
        uint32_t nlen = h->filename_len > 127 ? 127 : h->filename_len;
        for (uint32_t i = 0; i < nlen; i++) tmp_name[i] = raw[i];
        tmp_name[nlen] = '\0';

        uint32_t data_off = offset+sizeof(zip_local_header_t)+h->filename_len+h->extra_len;

        if (vga_strncmp(tmp_name, "META-INF/MANIFEST.MF", 20) == 0) {
            parse_manifest_for_main(buffer+data_off, h->uncompressed_size, main_class);
            klog("JAR: Main-Class found", 1);
            vga_puts(main_class, 0x0F);
            vga_puts("\n", 0x07);
            break;
        }

        offset = data_off + h->compressed_size;
    }

    if (main_class[0] == '\0') {
        klog("JAR: No Main-Class in manifest", 2);
        return;
    }

    // Chuyển "com.example.Main" → "com/example/Main.class"
    char class_path[128];
    uint32_t i = 0;
    for (; main_class[i]; i++)
        class_path[i] = (main_class[i] == '.') ? '/' : main_class[i];
    class_path[i++] = '.';
    class_path[i++] = 'c';
    class_path[i++] = 'l';
    class_path[i++] = 'a';
    class_path[i++] = 's';
    class_path[i++] = 's';
    class_path[i]   = '\0';

    // Tìm .class entry trong JAR
    uint32_t class_size = 0;
    uint8_t* class_data = jar_find_entry(buffer, size, class_path, &class_size);
    if (!class_data) {
        klog("JAR: .class file not found in JAR", 2);
        vga_puts(class_path, 0x0C);
        vga_puts("\n", 0x07);
        return;
    }

    klog("JAR: Loading class...", 1);
    class_execute(class_data, class_size);
}

// ─── 5. Entry point ───────────────────────────────────────────────────────────
int jar_execute(const char* path) {
    klog("JAR: Loading from initrd...", 1);

    uint32_t file_size = 0;
    void* jar_ptr = initrd_read_file(path, &file_size);
    if (!jar_ptr) {
        klog("JAR: File not found", 2);
        return -1;
    }

    zip_local_header_t* h = (zip_local_header_t*)jar_ptr;
    if (h->signature != ZIP_LOCAL_HEADER_SIG) {
        klog("JAR: Invalid ZIP signature", 2);
        return -1;
    }

    klog("JAR: ZIP OK, scanning entries...", 1);
    jar_scan_entries((uint8_t*)jar_ptr, file_size);
    return 0;
}

// ─── parse_manifest_for_main (giữ nguyên logic cũ) ───────────────────────────
void parse_manifest_for_main(uint8_t* data, uint32_t size, char* out_main) {
    char line[256];
    uint32_t p = 0;
    out_main[0] = '\0';

    for (uint32_t i = 0; i < size; i++) {
        char c = (char)data[i];
        if (c == '\n' || c == '\r' || i == size-1) {
            if (i == size-1 && c != '\n' && c != '\r') line[p++] = c;
            line[p] = '\0';
            if (vga_strncmp(line, "Main-Class:", 11) == 0) {
                char* val = line+11;
                while (*val==' ' || *val=='\t') val++;
                uint32_t j = 0;
                while (val[j] && val[j]!='\r' && j<127)
                    { out_main[j]=val[j]; j++; }
                out_main[j] = '\0';
                return;
            }
            p = 0;
        } else {
            if (p < 255) line[p++] = c;
        }
    }
}