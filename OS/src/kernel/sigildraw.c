#include <stddef.h>
#include <stdint.h>
#include "kernel/sigildraw.h"
#include "ui/rune.h"
#include "ui/glyphforge.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "kernel/sched.h"
#include "fs/vfs.h"
#include "lib/str.h"

#define SIGILDRAW_STATUS_ROW (TERM_HEIGHT - 2)
#define SIGILDRAW_CMD_ROW (TERM_HEIGHT - 1)
#define SIGILDRAW_DEFAULT_W 8
#define SIGILDRAW_DEFAULT_H 8

typedef struct {
    rune_bitmap_t bmp;
    uint8_t cur_x;
    uint8_t cur_y;
    uint8_t cur_color;
    int modified;
    char filename[32];
    char message[80];
} sigildraw_t;

static void sigildraw_set_message(sigildraw_t* ed, const char* msg) {
    if (!ed) return;
    kstrncpy0(ed->message, msg ? msg : "", sizeof(ed->message));
}

static const char* sigildraw_color_name(uint8_t color) {
    switch (color) {
        case RUNE_COLOR_BLACK: return "BLACK";
        case RUNE_COLOR_WHITE: return "WHITE";
        case RUNE_COLOR_RED:   return "RED";
        case RUNE_COLOR_GREEN: return "GREEN";
        case RUNE_COLOR_BLUE:  return "BLUE";
        default: return "UNKNOWN";
    }
}

static size_t sigildraw_canvas_row(const sigildraw_t* ed) {
    (void)ed;
    return 1;
}

static size_t sigildraw_canvas_col(const sigildraw_t* ed) {
    if (ed->bmp.width >= TEXT_WIDTH) return 0;
    return (TEXT_WIDTH - ed->bmp.width) / 2;
}

static void sigildraw_draw_status(sigildraw_t* ed) {
    terminal_clear_row(SIGILDRAW_STATUS_ROW);
    terminal_write_at(SIGILDRAW_STATUS_ROW, 0, "[SIGILDRAW] ");
    terminal_write_at(SIGILDRAW_STATUS_ROW, 12, ed->filename);

    if (ed->modified) {
        terminal_write_at(SIGILDRAW_STATUS_ROW, 12 + kstrlen(ed->filename) + 1, "*");
    }

    terminal_write_at(SIGILDRAW_STATUS_ROW, 32, "X ");
    {
        char buf[8];
        size_t v = (size_t)ed->cur_x;
        int p = 0;
        if (v == 0) buf[p++] = '0';
        else {
            char tmp[8];
            int tp = 0;
            while (v > 0) { tmp[tp++] = (char)('0' + (v % 10)); v /= 10; }
            while (tp > 0) buf[p++] = tmp[--tp];
        }
        buf[p] = '\0';
        terminal_write_at(SIGILDRAW_STATUS_ROW, 34, buf);
    }

    terminal_write_at(SIGILDRAW_STATUS_ROW, 40, "Y ");
    {
        char buf[8];
        size_t v = (size_t)ed->cur_y;
        int p = 0;
        if (v == 0) buf[p++] = '0';
        else {
            char tmp[8];
            int tp = 0;
            while (v > 0) { tmp[tp++] = (char)('0' + (v % 10)); v /= 10; }
            while (tp > 0) buf[p++] = tmp[--tp];
        }
        buf[p] = '\0';
        terminal_write_at(SIGILDRAW_STATUS_ROW, 42, buf);
    }

    terminal_write_at(SIGILDRAW_STATUS_ROW, 48, "Color ");
    terminal_write_at(SIGILDRAW_STATUS_ROW, 54, sigildraw_color_name(ed->cur_color));

    if (ed->message[0]) {
        terminal_write_at(SIGILDRAW_STATUS_ROW, 64, ed->message);
    }
}

static void sigildraw_draw_cmd(sigildraw_t* ed) {
    (void)ed;
    terminal_clear_row(SIGILDRAW_CMD_ROW);
    terminal_write_at(SIGILDRAW_CMD_ROW, 0, "Arrows=Move|1=K 2=W 3=R 4=G 5=B|Space=Paint|c=clear|w=save|x=save+quit|q=quit");
}

static void sigildraw_render(sigildraw_t* ed) {
    terminal_clear_text_area();

    size_t row = sigildraw_canvas_row(ed);
    size_t col = sigildraw_canvas_col(ed);

    glyphforge_draw_cursor_at(&ed->bmp, row, col, ed->cur_x, ed->cur_y);
    sigildraw_draw_status(ed);
    sigildraw_draw_cmd(ed);
}

static int sigildraw_save(sigildraw_t* ed) {
    if (!rune_save(ed->filename, &ed->bmp)) return 0;
    ed->modified = 0;
    sigildraw_set_message(ed, "Saved.");
    return 1;
}

void sigildraw_open(const char* filename) {
    if (!filename || !filename[0]) {
        terminal_write("Usage: sigildraw <file.rune>\n");
        return;
    }

    sigildraw_t ed;
    ed.cur_x = 0;
    ed.cur_y = 0;
    ed.cur_color = RUNE_COLOR_WHITE;
    ed.modified = 0;
    kstrncpy0(ed.filename, filename, sizeof(ed.filename));
    kstrncpy0(ed.message, "New rune.", sizeof(ed.message));

    if (!rune_load(filename, &ed.bmp)) {
        if (!rune_create(&ed.bmp, SIGILDRAW_DEFAULT_W, SIGILDRAW_DEFAULT_H)) {
            terminal_write("Could not create rune.\n");
            return;
        }
        rune_clear(&ed.bmp, RUNE_COLOR_BLACK);
        ed.modified = 1;
        sigildraw_set_message(&ed, "New 8x8 rune.");
    } else {
        sigildraw_set_message(&ed, "Loaded.");
    }

    sigildraw_render(&ed);

    for (;;) {
        key_event_t ev;
        if (!keyboard_try_get_key(&ev)) {
            yield();
            continue;
        }

        if (ev.type == KEY_LEFT) {
            if (ed.cur_x > 0) ed.cur_x--;
            sigildraw_render(&ed);

        } else if (ev.type == KEY_RIGHT) {
            if (ed.cur_x + 1 < ed.bmp.width) ed.cur_x++;
            sigildraw_render(&ed);

        } else if (ev.type == KEY_UP) {
            if (ed.cur_y > 0) ed.cur_y--;
            sigildraw_render(&ed);

        } else if (ev.type == KEY_DOWN) {
            if (ed.cur_y + 1 < ed.bmp.height) ed.cur_y++;
            sigildraw_render(&ed);

        } else if (ev.type == KEY_CHAR) {
            if (ev.ch == '1') {
                ed.cur_color = RUNE_COLOR_BLACK;
                sigildraw_set_message(&ed, "Color: black");
                sigildraw_render(&ed);

            } else if (ev.ch == '2') {
                ed.cur_color = RUNE_COLOR_WHITE;
                sigildraw_set_message(&ed, "Color: white");
                sigildraw_render(&ed);

            } else if (ev.ch == '3') {
                ed.cur_color = RUNE_COLOR_RED;
                sigildraw_set_message(&ed, "Color: red");
                sigildraw_render(&ed);

            } else if (ev.ch == '4') {
                ed.cur_color = RUNE_COLOR_GREEN;
                sigildraw_set_message(&ed, "Color: green");
                sigildraw_render(&ed);

            } else if (ev.ch == '5') {
                ed.cur_color = RUNE_COLOR_BLUE;
                sigildraw_set_message(&ed, "Color: blue");
                sigildraw_render(&ed);

            } else if (ev.ch == ' ') {
                if (rune_set_pixel(&ed.bmp, ed.cur_x, ed.cur_y, ed.cur_color)) {
                    ed.modified = 1;
                    sigildraw_set_message(&ed, "Painted.");
                }
                sigildraw_render(&ed);

            } else if (ev.ch == 'c') {
                rune_clear(&ed.bmp, RUNE_COLOR_BLACK);
                ed.modified = 1;
                sigildraw_set_message(&ed, "Cleared.");
                sigildraw_render(&ed);

            } else if (ev.ch == 'w') {
                if (!sigildraw_save(&ed)) sigildraw_set_message(&ed, "Save failed.");
                sigildraw_render(&ed);

            } else if (ev.ch == 'x') {
                if (sigildraw_save(&ed)) break;
                sigildraw_set_message(&ed, "Save failed.");
                sigildraw_render(&ed);

            } else if (ev.ch == 'q') {
                break;
            }
        }
    }

    terminal_clear_text_area();
    rune_free(&ed.bmp);
}
