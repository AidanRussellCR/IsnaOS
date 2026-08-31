#pragma once
#include <stdint.h>

#define IDT_ENTRY_COUNT 256
#define IDT_GATE_INTERRUPT_RING0 0x8Eu
#define IDT_GATE_INTERRUPT_RING3 0xEEu


void idt_init(void);

void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t flags);
