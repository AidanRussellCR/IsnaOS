#include <stdint.h>
#include <stddef.h>
#include "ui/tty_render.h"
#include "ui/gfx.h"
#include "drivers/vga.h"
#include "drivers/framebuffer.h"

static const uint32_t vga_palette[16] = {
    0x101820, 0x254DAD, 0x18864A, 0x258E9B,
    0xA93845, 0x824BAB, 0xA77823, 0xC7D3DA,
    0x586575, 0x548AFF, 0x47CF7A, 0x62C8D9,
    0xF06F7D, 0xBA87E9, 0xFFD175, 0xF4F8FB
};

static void paint_cell(int x, int y, uint16_t cell) {
    uint8_t attr = (uint8_t)(cell >> 8);
    uint8_t ch = (uint8_t)cell;
    gfx_char(x, y, ch >= 32 ? (char)ch : ' ',
             vga_palette[attr & 15u], vga_palette[(attr >> 4) & 15u]);
}

void tty_render_region(int x, int y, int width, int height) {
    if (!framebuffer_available() || width <= 0 || height <= 0) return;
    int rows = height / GFX_GLYPH_HEIGHT;
    int cols = width / GFX_GLYPH_WIDTH;
    if (rows > TERM_HEIGHT) rows = TERM_HEIGHT;
    if (cols > TEXT_WIDTH) cols = TEXT_WIDTH;
    if (rows <= 0 || cols <= 0) return;

    int start = 0;
    int cursor_row = (int)terminal_get_row();
    if (cursor_row >= rows) start = cursor_row - rows + 1;
    if (start + rows > TERM_HEIGHT) start = TERM_HEIGHT - rows;

    gfx_rect(x, y, width, height, 0x101820);
    for (int row = 0; row < rows; row++)
        for (int col = 0; col < cols; col++)
            paint_cell(x + col * GFX_GLYPH_WIDTH, y + row * GFX_GLYPH_HEIGHT, terminal_visible_cell((size_t)(start + row), (size_t)col));

    int cursor_col = (int)terminal_get_col();
    if (cursor_row >= start && cursor_row < start + rows && cursor_col < cols) {
        gfx_rect(x + cursor_col * GFX_GLYPH_WIDTH, y + (cursor_row - start + 1) * GFX_GLYPH_HEIGHT - 2, GFX_GLYPH_WIDTH, 2, 0x93EDB9);
    }
}

void tty_render_fullscreen(void) {
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    int width = (int)fb->width;
    int height = (int)fb->height;
    gfx_clip_reset();
    gfx_rect(0, 0, width, height, 0x111C29);
    gfx_rect(0, 0, width, 32, 0x29435C);
    gfx_text(20, 8, "IsnaOS   /   CLI - VT2", 0xF4F8FB, 0x29435C);
    int x = width > 680 ? (width - 640) / 2 : 0;
    int y = height > 500 ? 56 : 36;
    if (height - y < 400) y = 32;
    gfx_clip_set(x, y, 640, height - y - 24);
    tty_render_region(x, y, 640, 23 * 16);
    gfx_clip_reset();

    for (int row = 0; row < 2; row++) {
        int py = height - 56 + row * 16;
        if (py < y + 23 * 16) py = y + 23 * 16 + row * 16;
        if (py + 16 > height) break;
        gfx_rect(0, py, width, 16, 0x29435C);
        for (int col = 0; col < VGA_WIDTH; col++)
            paint_cell(x + col * GFX_GLYPH_WIDTH, py,
                       terminal_visible_cell((size_t)(TERM_HEIGHT + row), (size_t)col));
    }

    gfx_rect(0, height - 20, width, 20, 0x17283A);
    gfx_text(16, height - 18, "Ctrl+Alt+F1 Desktop    Ctrl+Alt+F2 CLI", 0xCDDBE8, 0x17283A);
}
