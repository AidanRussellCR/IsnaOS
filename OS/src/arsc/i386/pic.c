#include <stdint.h>

#include "arsc/i386/pic.h"
#include "arsc/i386/ports.h"


#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

#define ICW1_ICW4    0x01
#define ICW1_INIT    0x10

#define ICW4_8086    0x01


static void pic_io_wait(void) {
    outb(0x80, 0);
}


static void pic_remap(
    uint8_t master_offset,
    uint8_t slave_offset
) {
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask  = inb(PIC2_DATA);

    outb(
        PIC1_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );
    pic_io_wait();

    outb(
        PIC2_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );
    pic_io_wait();

    outb(PIC1_DATA, master_offset);
    pic_io_wait();

    outb(PIC2_DATA, slave_offset);
    pic_io_wait();

    outb(PIC1_DATA, 0x04);
    pic_io_wait();

    outb(PIC2_DATA, 0x02);
    pic_io_wait();

    outb(PIC1_DATA, ICW4_8086);
    pic_io_wait();

    outb(PIC2_DATA, ICW4_8086);
    pic_io_wait();

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}


void pic_mask_all(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}


void pic_init(void) {
    pic_remap(
        PIC_MASTER_OFFSET,
        PIC_SLAVE_OFFSET
    );

    pic_mask_all();
}


void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}


void pic_set_mask(uint8_t irq) {
    if (irq >= PIC_IRQ_COUNT)
        return;

    uint16_t port;
    uint8_t bit;

    if (irq < 8) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = (uint8_t)(irq - 8);
    }

    uint8_t mask = inb(port);

    mask |= (uint8_t)(1u << bit);

    outb(port, mask);
}


void pic_clear_mask(uint8_t irq) {
    if (irq >= PIC_IRQ_COUNT)
        return;

    uint16_t port;
    uint8_t bit;

    if (irq < 8) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = (uint8_t)(irq - 8);
    }

    uint8_t mask = inb(port);

    mask &= (uint8_t)~(1u << bit);

    outb(port, mask);
}
