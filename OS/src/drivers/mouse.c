#include <stdint.h>
#include "arsc/i386/ports.h"
#include "drivers/mouse.h"

static int enabled, screen_width, screen_height;
static uint8_t packet[3];
static int packet_pos;
static mouse_state_t state;

static int wait_for_input(void) {
    for (unsigned int i = 0; i < 100000; i++) {
        if (!(inb(0x64) & 2)) return 1;
    }

    return 0;
}

static int wait_for_output(void) {
    for (unsigned int i = 0; i < 100000; i++) {
        if (inb(0x64) & 1) return 1;
    }

    return 0;
}

static int controller_command(uint8_t value) {
    if (!wait_for_input()) return 0;
    outb(0x64, value);

    return 1;
}

static int controller_data(uint8_t value) {
    if (!wait_for_input()) return 0;
    outb(0x60, value);

    return 1;
}

static int mouse_command(uint8_t value) {
    return controller_command(0xD4) && controller_data(value);
}

void mouse_init(int width, int height) {
    enabled = 0;
    if (width < 1 || height < 1) return;
    screen_width = width;
    screen_height = height;
    state.x = width / 2;
    state.y = height / 2;
    state.buttons = 0;
    state.generation = 1;
    packet_pos = 0;

    if (!controller_command(0xA8)) return;
    if (!controller_command(0x20) || !wait_for_output()) return;
    uint8_t config = inb(0x60);
    /* Enable auxiliary PS/2 clock but keep IRQ12 masked for polling */
    config &= (uint8_t)~0x20;
    config &= (uint8_t)~0x02;

    if (!controller_command(0x60) || !controller_data(config)) return;
    if (!mouse_command(0xF6)) return; /* set defaults */
    if (!mouse_command(0xF4)) return; /* enable data reporting */
    enabled = 1;
}

int mouse_enabled(void) { return enabled; }

void mouse_consume_byte(uint8_t data) {
    if (!enabled) return;
    if (data == 0xFA && packet_pos == 0)   return; /* command ACK */
    if (packet_pos == 0 && !(data & 0x08)) return; /* sync bit */
    packet[packet_pos++] = data;
    if (packet_pos < 3) return;
    packet_pos = 0;
    if (packet[0] & 0xC0) return; /* overflow */

    int dx = (int)packet[1] - ((packet[0] & 0x10) ? 256 : 0);
    int dy = (int)packet[2] - ((packet[0] & 0x20) ? 256 : 0);
    int nx = state.x + dx;
    int ny = state.y - dy;
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if (nx >= screen_width) nx = screen_width - 1;
    if (ny >= screen_height) ny = screen_height - 1;
    uint8_t buttons = (uint8_t)(packet[0] & 7u);

    if (nx != state.x || ny != state.y || buttons != state.buttons) {
        state.x = nx;
        state.y = ny;
        state.buttons = buttons;
        state.generation++;
    }
}

mouse_state_t mouse_state(void) { return state; }
