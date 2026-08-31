#pragma once
#include <stdint.h>

#define GDT_KERNEL_CODE 0x08u
#define GDT_KERNEL_DATA 0x10u

#define GDT_USER_CODE   0x1Bu
#define GDT_USER_DATA   0x23u

void gdt_init(void);
