#pragma once

#include <stdint.h>

typedef struct {
    int x, y;
    uint8_t buttons;
    uint32_t generation;
} mouse_state_t;

/* PS/2 mouse use polled controller data */
void mouse_init(int screen_width, int screen_height);
void mouse_consume_byte(uint8_t data);
mouse_state_t mouse_state(void);
int mouse_enabled(void);
