#include <stdint.h>
#include "arsc/i386/gdt.h"

typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} gdt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} gdt_ptr_t;


/*
 * 0x00  null
 * 0x08  kernel code
 * 0x10  kernel data
 * 0x18  user code
 * 0x20  user data
 */
static gdt_entry_t g_gdt[5];
static gdt_ptr_t g_gdt_ptr;

extern void gdt_load(const gdt_ptr_t* ptr);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    gdt_entry_t* e = &g_gdt[index];

    e->base_low =
        (uint16_t)(base & 0xFFFF);

    e->base_middle =
        (uint8_t)((base >> 16) & 0xFF);

    e->base_high =
        (uint8_t)((base >> 24) & 0xFF);

    e->limit_low =
        (uint16_t)(limit & 0xFFFF);

    e->granularity =
        (uint8_t)((limit >> 16) & 0x0F);

    e->granularity |=
        (uint8_t)(granularity & 0xF0);

    e->access = access;
}


void gdt_init(void) {
    // Null
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel code
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);

    // Kernel data
    gdt_set_entry( 2, 0, 0xFFFFF, 0x92, 0xCF);

    // User code
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);

    // User data
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);

    g_gdt_ptr.limit = (uint16_t)(sizeof(g_gdt) - 1);

    g_gdt_ptr.base = (uint32_t)&g_gdt[0];

    gdt_load(&g_gdt_ptr);
}
