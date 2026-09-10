#include <stddef.h>
#include <stdint.h>

#include "arsc/i386/trap.h"
#include "arsc/i386/idt.h"
#include "arsc/i386/pic.h"

#include "drivers/vga.h"
#include "drivers/framebuffer.h"
#include "ui/gfx.h"

_Static_assert(
    offsetof(trap_frame_t, gs) == 0,
    "trap_frame_t gs offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, edi) == 16,
    "trap_frame_t edi offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, eax) == 44,
    "trap_frame_t eax offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, vector) == 48,
    "trap_frame_t vector offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, error_code) == 52,
    "trap_frame_t error offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, eip) == 56,
    "trap_frame_t eip offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, cs) == 60,
    "trap_frame_t cs offset mismatch"
);

_Static_assert(
    offsetof(trap_frame_t, eflags) == 64,
    "trap_frame_t eflags offset mismatch"
);

_Static_assert(
    sizeof(trap_frame_t) == 68,
    "trap_frame_t size mismatch"
);

static trap_handler_t g_handlers[256];

static const char* g_exception_names[32] = {
    "Divide Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",

    "x87 Floating-Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security Exception",
    "Reserved"
};

static void trap_print_hex32(uint32_t value) {
    static const char digits[] =
        "0123456789ABCDEF";

    terminal_write("0x");

    for (int shift = 28;
         shift >= 0;
         shift -= 4) {
        terminal_putc(
            digits[(value >> shift) & 0xF]
        );
    }
}

static void trap_print_register(
    const char* name,
    uint32_t value
) {
    terminal_write(name);
    terminal_write("=");

    trap_print_hex32(value);

    terminal_write(" ");
}

static void trap_halt(void) {
    for (;;) {
        __asm__ volatile (
            "cli; hlt"
        );
    }
}

static void trap_fb_hex(int x, int y, const char* label, uint32_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char text[11];
    text[0] = '0'; text[1] = 'x';

    for (int i = 0; i < 8; i++)
        text[2 + i] = hex[(value >> (28 - 4 * i)) & 0xFu];

    text[10] = '\0';
    gfx_text(x, y, label, 0xFFFFFFu, 0x4C1523u);
    gfx_text(x + 96, y, text, 0xFFF1B0u, 0x4C1523u);
}

static void trap_fb_panic(const trap_frame_t* frame) {
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    gfx_clip_reset();
    gfx_rect(0, 0, (int)fb->width, (int)fb->height, 0x4C1523u);
    gfx_text(16, 16, "ISNAOS KERNEL TRAP", 0xFFFFFFu, 0x4C1523u);

    if (frame->vector < 32)
        gfx_text(16, 48, g_exception_names[frame->vector], 0xFFD0D0u, 0x4C1523u);

    trap_fb_hex(16, 80, "Vector:", frame->vector);
    trap_fb_hex(16, 100, "Error:", frame->error_code);
    trap_fb_hex(16, 120, "EIP:", frame->eip);
}

static void trap_fatal(
    trap_frame_t* frame
) {
    terminal_write(
        "\n\n=== CPU TRAP ===\n"
    );

    terminal_write("Exception: ");

    if (frame->vector < 32) {
        terminal_write(
            g_exception_names[
                frame->vector
            ]
        );
    } else {
        terminal_write("Unknown");
    }

    terminal_write("\nVector: ");
    trap_print_hex32(frame->vector);

    terminal_write("\nError:  ");
    trap_print_hex32(frame->error_code);

    terminal_write("\nEIP:    ");
    trap_print_hex32(frame->eip);

    terminal_write("\nCS:     ");
    trap_print_hex32(frame->cs);

    terminal_write("\nEFLAGS: ");
    trap_print_hex32(frame->eflags);

    terminal_write("\n\n");

    trap_print_register(
        "EAX",
        frame->eax
    );

    trap_print_register(
        "EBX",
        frame->ebx
    );

    trap_print_register(
        "ECX",
        frame->ecx
    );

    trap_print_register(
        "EDX",
        frame->edx
    );

    terminal_write("\n");

    trap_print_register(
        "ESI",
        frame->esi
    );

    trap_print_register(
        "EDI",
        frame->edi
    );

    trap_print_register(
        "EBP",
        frame->ebp
    );

    terminal_write(
        "\n\nSystem halted.\n"
    );

    trap_fb_panic(frame);
    trap_halt();
}

void trap_register_handler(
    uint8_t vector,
    trap_handler_t handler
) {
    g_handlers[vector] = handler;
}

void trap_init(void) {
    for (int i = 0; i < 256; i++) {
        g_handlers[i] = 0;
    }

    idt_init();

    pic_init();
}

void trap_dispatch(
    trap_frame_t* frame
) {
    if (!frame) {
        trap_halt();
    }

    uint32_t vector =
        frame->vector;

    if (vector >= 256) {
        trap_fatal(frame);
    }

    trap_handler_t handler =
        g_handlers[vector];

    if (vector >= TRAP_IRQ_BASE &&
        vector <= TRAP_IRQ_END) {

        if (handler) {
            handler(frame);
        }

        pic_send_eoi(
            (uint8_t)(
                vector -
                TRAP_IRQ_BASE
            )
        );

        return;
    }

    if (handler) {
        handler(frame);
        return;
    }

    if (vector == TRAP_BREAKPOINT) {
        terminal_write(
            "\n[trap] breakpoint at "
        );

        trap_print_hex32(frame->eip);

        terminal_write("\n");

        return;
    }

    if (vector < 32) {
        trap_fatal(frame);
    }

    trap_fatal(frame);
}
