#pragma once
#include <stdint.h>

#define TRAP_DIVIDE_ERROR       0
#define TRAP_DEBUG              1
#define TRAP_NMI                2
#define TRAP_BREAKPOINT         3
#define TRAP_OVERFLOW           4
#define TRAP_BOUND              5
#define TRAP_INVALID_OPCODE     6
#define TRAP_DEVICE_NOT_AVAIL   7
#define TRAP_DOUBLE_FAULT       8
#define TRAP_INVALID_TSS       10
#define TRAP_SEGMENT_NOT_PRESENT 11
#define TRAP_STACK_FAULT       12
#define TRAP_GENERAL_PROTECTION 13
#define TRAP_PAGE_FAULT        14
#define TRAP_X87               16
#define TRAP_ALIGNMENT_CHECK   17
#define TRAP_MACHINE_CHECK     18
#define TRAP_SIMD              19

#define TRAP_IRQ_BASE          32
#define TRAP_IRQ_END           47

typedef struct trap_frame {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t vector;
    uint32_t error_code;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} trap_frame_t;


typedef void (*trap_handler_t)(
    trap_frame_t* frame
);

void trap_init(void);

void trap_dispatch(
    trap_frame_t* frame
);

void trap_register_handler(
    uint8_t vector,
    trap_handler_t handler
);
