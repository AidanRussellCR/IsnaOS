#include <stddef.h>
#include <stdint.h>
#include "kernel/glyph.h"
#include "kernel/janus.h"
#include "ui/rune.h"
#include "ui/glyphforge.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "kernel/sched.h"

static void glyph_wait_for_key(void) {
    for (;;) {
        key_event_t ev;
        if (!janus_try_get_key(&ev)) {
            yield();
            continue;
        }
        return;
    }
}

void glyph_view(const char* filename) {
    if (!filename || !filename[0]) {
        terminal_write("Usage: glyph <file.rune>\n");
        return;
    }

    rune_bitmap_t bmp;
    if (!rune_load(filename, &bmp)) {
        terminal_write("Could not load rune.\n");
        return;
    }

    terminal_clear_text_area();

    size_t row = 1;
    size_t col = 0;

    if (bmp.height < TERM_HEIGHT - 2) {
        row = (TERM_HEIGHT - 2 - bmp.height) / 2;
    }
    if (bmp.width < TEXT_WIDTH) {
        col = (TEXT_WIDTH - bmp.width) / 2;
    }

    glyphforge_draw_at(&bmp, row, col);

    terminal_clear_row(TERM_HEIGHT - 1);
    terminal_write_at(TERM_HEIGHT - 1, 0, "glyph: press any key to return");

    glyph_wait_for_key();

    terminal_clear_text_area();
    rune_free(&bmp);
}
