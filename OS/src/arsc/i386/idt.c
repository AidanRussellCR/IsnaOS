#include <stdint.h>

#include "arsc/i386/idt.h"
#include "arsc/i386/gdt.h"

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t offset_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idt_ptr_t;


static idt_entry_t g_idt[IDT_ENTRY_COUNT];
static idt_ptr_t g_idt_ptr;

extern uint32_t trap_stub_table[];

void idt_set_gate(
    uint8_t vector,
    uint32_t handler,
    uint16_t selector,
    uint8_t flags
) {
    idt_entry_t* e = &g_idt[vector];

    e->offset_low =
        (uint16_t)(handler & 0xFFFF);

    e->selector = selector;
    e->zero = 0;
    e->flags = flags;

    e->offset_high =
        (uint16_t)((handler >> 16) & 0xFFFF);
}

void idt_init(void) {
    for (int i = 0; i < IDT_ENTRY_COUNT; i++) {
        g_idt[i].offset_low = 0;
        g_idt[i].selector = 0;
        g_idt[i].zero = 0;
        g_idt[i].flags = 0;
        g_idt[i].offset_high = 0;
    }

    for (int i = 0; i < 48; i++) {
        idt_set_gate(
            (uint8_t)i,
            trap_stub_table[i],
            GDT_KERNEL_CODE,
            IDT_GATE_INTERRUPT_RING0
        );
    }

    g_idt_ptr.limit =
        (uint16_t)(sizeof(g_idt) - 1);

    g_idt_ptr.base =
        (uint32_t)&g_idt[0];

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(g_idt_ptr)
    );
}
