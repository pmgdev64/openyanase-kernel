// src/runtime/bytecode.c
#include "bytecode.h"
#include "../graphics/vesa.h"
#include "../drivers/mouse.h"
#include "../vga_buffer.h"
#include <stdint.h>

// ─── JVM State ───────────────────────────────────────────────────────────────
int32_t stack[STACK_SIZE];
int      sp = -1;
int32_t locals[LOCALS_SIZE];

// ─── Macro helpers ───────────────────────────────────────────────────────────
#define U16(code, pc) ((uint16_t)((code[pc] << 8) | code[pc+1]))
#define I16(code, pc) ((int16_t)U16(code, pc))

// ─── Native method implementations ───────────────────────────────────────────

// System.out.println(int)
static void native_println_int(void) {
    int32_t v = pop();
    vesa_draw_number(10, 10, v, 0xFFFFFF);
    vesa_update();
}

// System.out.println(String) — string is index into CP, simplified
static void native_println_str(void) {
    // String ref trên stack — bỏ qua vì chưa có heap string
    pop();
}

// Mouse.getX() → push mouse_x
static void native_mouse_get_x(void) {
    extern int32_t mouse_x;
    push(mouse_x);
}

// Mouse.getY() → push mouse_y
static void native_mouse_get_y(void) {
    extern int32_t mouse_y;
    push(mouse_y);
}

// Screen.drawPixel(x, y, color)
static void native_draw_pixel(void) {
    int32_t color = pop();
    int32_t y     = pop();
    int32_t x     = pop();
    vesa_put_pixel((uint16_t)x, (uint16_t)y, (uint32_t)color);
}

// Screen.clear(color)
static void native_clear(void) {
    int32_t color = pop();
    vesa_clear((uint32_t)color);
}

// Screen.update()
static void native_update(void) {
    vesa_update();
}

// ─── Native dispatch table ────────────────────────────────────────────────────
static const native_entry_t native_table[] = {
    { "java/io/PrintStream", "println", "(I)V",                native_println_int  },
    { "java/io/PrintStream", "println", "(Ljava/lang/String;)V", native_println_str },
    // Cập nhật đường dẫn cho Mouse
    { "vn/pmgteam/yanase/input/Mouse",    "getX",    "()I",    native_mouse_get_x  },
    { "vn/pmgteam/yanase/input/Mouse",    "getY",    "()I",    native_mouse_get_y  },
    // Cập nhật đường dẫn cho Screen
    { "vn/pmgteam/yanase/graphics/Screen", "drawPixel","(III)V", native_draw_pixel  },
    { "vn/pmgteam/yanase/graphics/Screen", "clear",   "(I)V",    native_clear       },
    { "vn/pmgteam/yanase/graphics/Screen", "update",  "()V",    native_update      },
    { 0, 0, 0, 0 }
};

// Tìm và gọi native method
void handle_native_call(const char* class_name,
                        const char* method_name,
                        const char* descriptor) {
    for (int i = 0; native_table[i].class_name; i++) {
        if (vga_strncmp(native_table[i].class_name, class_name,   128) == 0 &&
            vga_strncmp(native_table[i].method_name, method_name, 128) == 0 &&
            vga_strncmp(native_table[i].descriptor,  descriptor,  128) == 0) {
            native_table[i].fn();
            return;
        }
    }
    klog("Native: unresolved method", 2);
}

// ─── Bytecode interpreter ─────────────────────────────────────────────────────
void execute_bytecode(uint8_t* code, uint32_t length,
                      cp_entry_t* cp, uint16_t cp_count) {
    uint32_t pc = 0;
    sp = -1;

    while (pc < length) {
        uint8_t op = code[pc++];

        switch (op) {

        // ── Constants ────────────────────────────────────────────────────────
        case 0x00: break;                           // nop
        case 0x01: push(0); break;                  // aconst_null
        case 0x02: push(-1); break;                 // iconst_m1
        case 0x03: push(0);  break;                 // iconst_0
        case 0x04: push(1);  break;                 // iconst_1
        case 0x05: push(2);  break;                 // iconst_2
        case 0x06: push(3);  break;                 // iconst_3
        case 0x07: push(4);  break;                 // iconst_4
        case 0x08: push(5);  break;                 // iconst_5
        case 0x10: push((int8_t)code[pc++]); break; // bipush
        case 0x11:                                  // sipush
            push(I16(code, pc)); pc += 2; break;
        case 0x12: {                                // ldc
            uint8_t idx = code[pc++];
            if (idx < cp_count && cp[idx].tag == CP_INTEGER)
                push(cp[idx].integer.value);
            else
                push(0);
            break;
        }

        // ── Loads ────────────────────────────────────────────────────────────
        case 0x1A: push(locals[0]); break;          // iload_0
        case 0x1B: push(locals[1]); break;          // iload_1
        case 0x1C: push(locals[2]); break;          // iload_2
        case 0x1D: push(locals[3]); break;          // iload_3
        case 0x15: push(locals[code[pc++]]); break; // iload

        // ── Stores ───────────────────────────────────────────────────────────
        case 0x3B: locals[0] = pop(); break;        // istore_0
        case 0x3C: locals[1] = pop(); break;        // istore_1
        case 0x3D: locals[2] = pop(); break;        // istore_2
        case 0x3E: locals[3] = pop(); break;        // istore_3
        case 0x36: locals[code[pc++]] = pop(); break; // istore

        // ── Stack ops ────────────────────────────────────────────────────────
        case 0x57: pop(); break;                    // pop
        case 0x59: push(stack[sp]); break;          // dup

        // ── Arithmetic ───────────────────────────────────────────────────────
        case 0x60: { int32_t b=pop(),a=pop(); push(a+b); break; } // iadd
        case 0x64: { int32_t b=pop(),a=pop(); push(a-b); break; } // isub
        case 0x68: { int32_t b=pop(),a=pop(); push(a*b); break; } // imul
        case 0x6C: { int32_t b=pop(),a=pop(); push(b?a/b:0); break; } // idiv
        case 0x70: { int32_t b=pop(),a=pop(); push(b?a%b:0); break; } // irem
        case 0x74: push(-pop()); break;             // ineg
        case 0x78: { int32_t b=pop(),a=pop(); push(a<<b);  break; } // ishl
        case 0x7A: { int32_t b=pop(),a=pop(); push(a>>b);  break; } // ishr
        case 0x80: { int32_t b=pop(),a=pop(); push(a|b);   break; } // ior
        case 0x7E: { int32_t b=pop(),a=pop(); push(a&b);   break; } // iand
        case 0x82: { int32_t b=pop(),a=pop(); push(a^b);   break; } // ixor
        case 0x84: {                                // iinc
            uint8_t idx = code[pc++];
            int8_t  val = (int8_t)code[pc++];
            locals[idx] += val;
            break;
        }

        // ── Comparisons & Branches ───────────────────────────────────────────
        case 0x99: { int16_t off=I16(code,pc); pc+=2; if(pop()==0)  pc+=off-3; break; } // ifeq
        case 0x9A: { int16_t off=I16(code,pc); pc+=2; if(pop()!=0)  pc+=off-3; break; } // ifne
        case 0x9B: { int16_t off=I16(code,pc); pc+=2; if(pop()<0)   pc+=off-3; break; } // iflt
        case 0x9C: { int16_t off=I16(code,pc); pc+=2; if(pop()>=0)  pc+=off-3; break; } // ifge
        case 0x9D: { int16_t off=I16(code,pc); pc+=2; if(pop()>0)   pc+=off-3; break; } // ifgt
        case 0x9E: { int16_t off=I16(code,pc); pc+=2; if(pop()<=0)  pc+=off-3; break; } // ifle
        case 0x9F: { int16_t off=I16(code,pc); pc+=2; int32_t b=pop(),a=pop(); if(a==b) pc+=off-3; break; } // if_icmpeq
        case 0xA0: { int16_t off=I16(code,pc); pc+=2; int32_t b=pop(),a=pop(); if(a!=b) pc+=off-3; break; } // if_icmpne
        case 0xA1: { int16_t off=I16(code,pc); pc+=2; int32_t b=pop(),a=pop(); if(a<b)  pc+=off-3; break; } // if_icmplt
        case 0xA2: { int16_t off=I16(code,pc); pc+=2; int32_t b=pop(),a=pop(); if(a>=b) pc+=off-3; break; } // if_icmpge
        case 0xA3: { int16_t off=I16(code,pc); pc+=2; int32_t b=pop(),a=pop(); if(a>b)  pc+=off-3; break; } // if_icmpgt
        case 0xA4: { int16_t off=I16(code,pc); pc+=2; int32_t b=pop(),a=pop(); if(a<=b) pc+=off-3; break; } // if_icmple
        case 0xA7: { int16_t off=I16(code,pc); pc+=2; pc+=off-3; break; }      // goto

        // ── Return ───────────────────────────────────────────────────────────
        case 0xAC: return; // ireturn
        case 0xB1: return; // return (void)

        // ── Method invocations ───────────────────────────────────────────────
        case 0xB8: {                               // invokestatic
            uint16_t idx = U16(code, pc); pc += 2;
            if (idx >= cp_count) break;
            cp_entry_t* mref = &cp[idx];
            if (mref->tag != CP_METHODREF) break;

            cp_entry_t* cls = &cp[mref->ref.class_i];
            cp_entry_t* nat = &cp[mref->ref.nat_i];
            if (cls->tag != CP_CLASS || nat->tag != CP_NAME_AND_TYPE) break;

            const char* class_name  = (const char*)cp[cls->class_ref.index].utf8.bytes;
            const char* method_name = (const char*)cp[nat->nat.name_i].utf8.bytes;
            const char* descriptor  = (const char*)cp[nat->nat.desc_i].utf8.bytes;

            handle_native_call(class_name, method_name, descriptor);
            break;
        }

        case 0xB6:                                 // invokevirtual — bỏ qua object ref
        case 0xB7: {                               // invokespecial
            pc += 2; // skip index
            break;
        }

        // ── Field access (stub) ──────────────────────────────────────────────
        case 0xB2: pc += 2; push(0); break;        // getstatic
        case 0xB3: pc += 2; pop();   break;        // putstatic

        default:
            // Opcode không biết — bỏ qua
            break;
        }
    }
}