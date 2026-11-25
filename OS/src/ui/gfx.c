#include <stddef.h>
#include <stdint.h>
#include "ui/gfx.h"
#include "drivers/framebuffer.h"
#include "ui/font8x16.h"

// Future physical page allocator should replace this fixed-size buffer down the line
#define GFX_BACKBUFFER_PIXELS (1024u * 768u)

static uint32_t g_backbuffer[GFX_BACKBUFFER_PIXELS];
static int g_frame_active;
static uint32_t g_frame_width;

static int clip_left, clip_top, clip_right, clip_bottom;

void gfx_begin_frame(void) {
    const framebuffer_t* fb = framebuffer_info();
    g_frame_active = 0;
    if (!fb || fb->width == 0 || fb->height == 0) return;

    // Avoid multiplication overflow and fall back at larger modes
    if (fb->width > GFX_BACKBUFFER_PIXELS / fb->height) return;

    g_frame_width = fb->width;
    g_frame_active = 1;
}

void gfx_end_frame(void) {
    if (!g_frame_active) return;
    g_frame_active = 0;

    const framebuffer_t* fb = framebuffer_info();
    if (!fb || fb->width != g_frame_width) return;

    // Copy finished image
    for (uint32_t y = 0; y < fb->height; ++y) {
        const uint32_t* src = g_backbuffer + y * fb->width;
        volatile uint32_t* dst = (volatile uint32_t*)(fb->address + y * fb->pitch);
        for (uint32_t x = 0; x < fb->width; ++x) dst[x] = src[x];
    }
}

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

    if (g_frame_active) {
        g_backbuffer[(uint32_t)y * g_frame_width + (uint32_t)x] = framebuffer_pack_rgb(rgb);
    } else {
        framebuffer_pixel(x, y, rgb);
    }
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

    if (g_frame_active) {
        for (int row = y0; row < y1; ++row) {
            uint32_t* dst = g_backbuffer + (uint32_t)row * g_frame_width + (uint32_t)x0;
            for (int col = x0; col < x1; ++col) *dst++ = packed;
        }
    } else {
        for (int row = y0; row < y1; ++row) {
            volatile uint32_t* dst = (volatile uint32_t*)(fb->address + (uint32_t)row * fb->pitch + (uint32_t)x0 * 4u);
            for (int col = x0; col < x1; ++col) *dst++ = packed;
        }
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
