#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

// ─── JVM Stack ───────────────────────────────────────────────────────────────
#define STACK_SIZE   256
#define LOCALS_SIZE  64

extern int32_t  stack[];
extern int       sp;
extern int32_t  locals[];

// ─── Constant Pool Entry ─────────────────────────────────────────────────────
#define CP_UTF8             1
#define CP_INTEGER          3
#define CP_FLOAT            4
#define CP_LONG             5
#define CP_DOUBLE           6
#define CP_CLASS            7
#define CP_STRING           8
#define CP_FIELDREF         9
#define CP_METHODREF        10
#define CP_INTERFACE_MREF   11
#define CP_NAME_AND_TYPE    12

typedef struct {
    uint8_t  tag;
    union {
        struct { uint8_t* bytes; uint16_t length; } utf8;
        struct { uint32_t value; }                  integer;
        struct { uint16_t index; }                  class_ref;
        struct { uint16_t index; }                  string_ref;
        struct { uint16_t class_i; uint16_t nat_i; } ref;      // field/method
        struct { uint16_t name_i; uint16_t desc_i; } nat;      // name_and_type
    };
} cp_entry_t;

// ─── Class File ──────────────────────────────────────────────────────────────
typedef struct {
    cp_entry_t* pool;
    uint16_t    pool_count;
    uint16_t    this_class;
    uint16_t    super_class;
} class_t;

// ─── Method ──────────────────────────────────────────────────────────────────
typedef struct {
    uint16_t  access_flags;
    char*     name;
    char*     descriptor;
    uint8_t*  code;
    uint32_t  code_len;
    uint16_t  max_stack;
    uint16_t  max_locals;
} method_t;

// ─── Native method table ─────────────────────────────────────────────────────
typedef void (*native_fn_t)(void);

typedef struct {
    const char*  class_name;
    const char*  method_name;
    const char*  descriptor;
    native_fn_t  fn;
} native_entry_t;

// ─── API ─────────────────────────────────────────────────────────────────────
void execute_bytecode(uint8_t* code, uint32_t length,
                      cp_entry_t* cp, uint16_t cp_count);

void handle_native_call(const char* class_name,
                        const char* method_name,
                        const char* descriptor);

// Stack helpers
static inline void  push(int32_t v) { stack[++sp] = v; }
static inline int32_t pop(void)     { return stack[sp--]; }

#endif