#include <stddef.h>
#include <stdint.h>
#include "ui/gfx.h"
#include "drivers/framebuffer.h"
#include "ui/font8x16.h"

static int clip_left, clip_top, clip_right, clip_bottom;

void gfx_clip_reset(void) {
    const framebuffer_t* fb = framebuffer_info();
    clip_left = clip_top = 0;
    clip_right = fb ? (int)fb->width : 0;
    clip_bottom = fb ? (int)fb->height : 0;
}

void gfx_clip_set(int x, int y, int w, int h) {
    const framebuffer_t* fb = framebuffer_info();

    if (!fb || w <= 0 || h <= 0) {
        clip_left = clip_right = clip_top = clip_bottom = 0;
        return;
    }

    clip_left = x > 0 ? x : 0;
    clip_top = y > 0 ? y : 0;
    clip_right = x + w < (int)fb->width ? x + w : (int)fb->width;
    clip_bottom = y + h < (int)fb->height ? y + h : (int)fb->height;

    if (clip_right < clip_left) clip_right = clip_left;
    if (clip_bottom < clip_top) clip_bottom = clip_top;
}

void gfx_pixel(int x, int y, uint32_t rgb) {
    if (x < clip_left || x >= clip_right || y < clip_top || y >= clip_bottom)
        return;

    framebuffer_pixel(x, y, rgb);
}

void gfx_rect(int x, int y, int w, int h, uint32_t rgb) {
    if (w <= 0 || h <= 0) return;
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    int x0 = x > clip_left ? x : clip_left;
    int y0 = y > clip_top ? y : clip_top;
    int x1 = x + w < clip_right ? x + w : clip_right;
    int y1 = y + h < clip_bottom ? y + h : clip_bottom;
    if (x0 >= x1 || y0 >= y1) return;
    uint32_t packed = framebuffer_pack_rgb(rgb);

    for (int row = y0; row < y1; row++) {
        volatile uint32_t* dst = (volatile uint32_t*)(fb->address + (uint32_t)row * fb->pitch + (uint32_t)x0 * 4u);
        for (int col = x0; col < x1; col++) *dst++ = packed;
    }
}

void gfx_hline(int x, int y, int w, uint32_t rgb) { gfx_rect(x, y, w, 1, rgb); }
void gfx_vline(int x, int y, int h, uint32_t rgb) { gfx_rect(x, y, 1, h, rgb); }

void gfx_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    unsigned char uc = (unsigned char)c;
    if (uc < 32 || uc > 126) uc = '?';
    const uint8_t* glyph = isna_font8x16[uc - 32];
    gfx_rect(x, y, GFX_GLYPH_WIDTH, GFX_GLYPH_HEIGHT, bg);

    for (int py = 0; py < GFX_GLYPH_HEIGHT; py++) {
        uint8_t bits = glyph[py];
        for (int px = 0; px < GFX_GLYPH_WIDTH; px++) {
            if (bits & (uint8_t)(0x80u >> px)) gfx_pixel(x + px, y + py, fg);
        }
    }
}

void gfx_text(int x, int y, const char* text, uint32_t fg, uint32_t bg) {
    if (!text) return;

    for (; *text; text++, x += GFX_GLYPH_WIDTH) {
        gfx_char(x, y, *text, fg, bg);
    }
}
