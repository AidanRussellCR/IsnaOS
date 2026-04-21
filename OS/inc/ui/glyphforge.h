#ifndef UI_GLYPHFORGE_H
#define UI_GLYPHFORGE_H

#include <stddef.h>
#include "ui/rune.h"

void glyphforge_draw_at(const rune_bitmap_t* bmp, size_t row, size_t col);
void glyphforge_draw_cursor_at(const rune_bitmap_t* bmp, size_t row, size_t col, size_t cx, size_t cy);
void glyphforge_clear_region(size_t row, size_t col, size_t width, size_t height);

#endif
