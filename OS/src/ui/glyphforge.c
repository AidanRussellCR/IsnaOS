#include <stddef.h>
#include <stdint.h>
#include "ui/glyphforge.h"
#include "drivers/vga.h"

static uint8_t rune_color_to_vga(uint8_t color) {
    switch (color) {
        case RUNE_COLOR_BLACK: return 0x00;
        case RUNE_COLOR_WHITE: return 0xF0;
        case RUNE_COLOR_RED:   return 0x40;
        case RUNE_COLOR_GREEN: return 0x20;
        case RUNE_COLOR_BLUE:  return 0x10;
        default: return 0x00;
    }
}

void glyphforge_clear_region(size_t row, size_t col, size_t width, size_t height) {
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            if (row + y >= TERM_HEIGHT) continue;
            if (col + x >= TEXT_WIDTH) continue;
            terminal_putentry_at(row + y, col + x, ' ', 0x00);
        }
    }
}

void glyphforge_draw_at(const rune_bitmap_t* bmp, size_t row, size_t col) {
    if (!bmp || !bmp->pixels) return;

    for (size_t y = 0; y < bmp->height; y++) {
        for (size_t x = 0; x < bmp->width; x++) {
            if (row + y >= TERM_HEIGHT) continue;
            if (col + x >= TEXT_WIDTH) continue;

            uint8_t color = rune_get_pixel(bmp, (uint8_t)x, (uint8_t)y);
            terminal_putentry_at(row + y, col + x, ' ', rune_color_to_vga(color));
        }
    }
}

void glyphforge_draw_cursor_at(const rune_bitmap_t* bmp, size_t row, size_t col, size_t cx, size_t cy) {
    if (!bmp || !bmp->pixels) return;

    glyphforge_draw_at(bmp, row, col);

    if (cx < bmp->width && cy < bmp->height) {
        if (row + cy < TERM_HEIGHT && col + cx < TEXT_WIDTH) {
            terminal_putentry_at(row + cy, col + cx, 'X', 0x0F);
        }
    }
}
