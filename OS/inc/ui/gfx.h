#pragma once

#include <stdint.h>

#define GFX_GLYPH_WIDTH 8
#define GFX_GLYPH_HEIGHT 16

void gfx_clip_reset(void);
void gfx_clip_set(int x, int y, int w, int h);
void gfx_pixel(int x, int y, uint32_t rgb);
void gfx_rect(int x, int y, int w, int h, uint32_t rgb);
void gfx_hline(int x, int y, int w, uint32_t rgb);
void gfx_vline(int x, int y, int h, uint32_t rgb);
void gfx_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void gfx_text(int x, int y, const char* text, uint32_t fg, uint32_t bg);
