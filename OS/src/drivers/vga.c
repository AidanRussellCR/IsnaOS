#include <stddef.h>
#include <stdint.h>
#include "arsc/i386/ports.h"
#include "drivers/vga.h"
#include "drivers/framebuffer.h"

#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

/*
 * TTY state is independent of whichever display backend is visible
 * Current implementation is aiming for one shell TTY and one GUI
 */

static uint16_t g_textbuf[SCROLLBACK_ROWS][TEXT_WIDTH];
static uint16_t g_overlay[2][VGA_WIDTH];
static size_t   g_cursor_row, g_cursor_col, g_head_row, g_view_top;
static int      g_follow_tail = 1;
static uint8_t  g_color = 0x0F;
static uint32_t g_generation = 1;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)(uint8_t)c | ((uint16_t)color << 8);
}

static void clear_buffer_row(size_t row) {
    if (row >= SCROLLBACK_ROWS) return;
    for (size_t i = 0; i < TEXT_WIDTH; i++)
        g_textbuf[row][i] = vga_entry(' ', g_color);
}

static void sync_cursor(void) {
    if (framebuffer_available()) return;
    size_t r = g_cursor_row >= g_view_top ? g_cursor_row - g_view_top : 0;
    if (r >= TERM_HEIGHT) r = TERM_HEIGHT - 1;
    size_t c = g_cursor_col < TEXT_WIDTH ? g_cursor_col : TEXT_WIDTH - 1;
    vga_cursor_set_pos(r, c);
}

uint16_t terminal_visible_cell(size_t row, size_t col) {
    if (col >= VGA_WIDTH || row >= VGA_HEIGHT) return vga_entry(' ', g_color);
    if (row >= TERM_HEIGHT) return g_overlay[row - TERM_HEIGHT][col];
    if (col >= TEXT_WIDTH) {
        size_t max = g_head_row >= TERM_HEIGHT ? g_head_row - TERM_HEIGHT + 1 : 0;
        size_t mark = max ? (g_view_top * TERM_HEIGHT) / (max + 1) : 0;
        if (mark >= TERM_HEIGHT) mark = TERM_HEIGHT - 1;
        return vga_entry(row == mark ? '|' : ' ', g_color);
    }
    size_t src = g_view_top + row;
    if (src >= SCROLLBACK_ROWS) return vga_entry(' ', g_color);
    return g_textbuf[src][col];
}

uint32_t terminal_generation(void) { return g_generation; }

static void render_vga(void) {
    if (framebuffer_available()) return;
    for (size_t row = 0; row < VGA_HEIGHT; row++)
        for (size_t col = 0; col < VGA_WIDTH; col++)
            VGA_MEMORY[row * VGA_WIDTH + col] = terminal_visible_cell(row, col);
    sync_cursor();
}

static void terminal_touch(void) {
    g_generation++;
    render_vga();
}

static void advance_line(void) {
    g_cursor_col = 0;

    if (g_cursor_row + 1 < SCROLLBACK_ROWS) {
        g_cursor_row++;
        clear_buffer_row(g_cursor_row);
    } else {
        for (size_t row = 1; row < SCROLLBACK_ROWS; row++)
            for (size_t col = 0; col < TEXT_WIDTH; col++)
                g_textbuf[row - 1][col] = g_textbuf[row][col];
        clear_buffer_row(SCROLLBACK_ROWS - 1);
        if (g_view_top) g_view_top--;
    }
    g_head_row = g_cursor_row;
    if (g_follow_tail)
        g_view_top = g_head_row >= TERM_HEIGHT ? g_head_row - TERM_HEIGHT + 1 : 0;
}

size_t terminal_get_row(void) {
    size_t r = g_cursor_row >= g_view_top ? g_cursor_row - g_view_top : 0;
    return r < TERM_HEIGHT ? r : TERM_HEIGHT - 1;
}

size_t terminal_get_col(void) { return g_cursor_col; }

size_t terminal_get_buffer_row(void) { return g_cursor_row; }
size_t terminal_get_view_top(void) { return g_view_top; }

void terminal_set_cursor_pos(size_t row, size_t col) {
    if (row >= TERM_HEIGHT) row = TERM_HEIGHT - 1;
    if (col >= TEXT_WIDTH) col = TEXT_WIDTH - 1;

    g_cursor_row = g_view_top + row;
    if (g_cursor_row >= SCROLLBACK_ROWS) g_cursor_row = SCROLLBACK_ROWS - 1;
    g_cursor_col = col;

    terminal_touch();
}

void terminal_ensure_row_visible(size_t row) {
    if (row >= SCROLLBACK_ROWS) row = SCROLLBACK_ROWS - 1;

    if (row < g_view_top) g_view_top = row;
    else if (row >= g_view_top + TERM_HEIGHT)
        g_view_top = row - TERM_HEIGHT + 1;

    terminal_touch();
}

void terminal_scroll_view_up(void) {
    if (g_view_top) g_view_top--;
    g_follow_tail = 0;
    terminal_touch();
}

void terminal_scroll_view_down(void) {
    size_t max = g_head_row >= TERM_HEIGHT ? g_head_row - TERM_HEIGHT + 1 : 0;
    if (g_view_top < max) g_view_top++;
    if (g_view_top == max) g_follow_tail = 1;
    terminal_touch();
}

void terminal_follow_tail(void) {
    g_view_top = g_head_row >= TERM_HEIGHT ? g_head_row - TERM_HEIGHT + 1 : 0;
    g_follow_tail = 1;
    terminal_touch();
}

int terminal_is_following_tail(void) {
    return g_follow_tail;
}

void terminal_clear_text_area(void) {
    for (size_t row = 0; row < SCROLLBACK_ROWS; row++) clear_buffer_row(row);
    g_cursor_row = g_cursor_col = g_head_row = g_view_top = 0;
    g_follow_tail = 1;
    terminal_touch();
}

void terminal_clear(void) {
    for (size_t row = 0; row < 2; row++)
        for (size_t col = 0; col < VGA_WIDTH; col++)
            g_overlay[row][col] = vga_entry(' ', 0x07);
    terminal_clear_text_area();
}

void terminal_clear_row(size_t row) {
    if (row >= VGA_HEIGHT) return;
    if (row < TERM_HEIGHT) {
        size_t src = g_view_top + row;
        if (src < SCROLLBACK_ROWS) clear_buffer_row(src);
    } else {
        for (size_t col = 0; col < VGA_WIDTH; col++)
            g_overlay[row - TERM_HEIGHT][col] = vga_entry(' ', 0x07);
    }

    terminal_touch();
}

void terminal_putc(char c) {
    if (c == '\n') {
        while (g_cursor_col < TEXT_WIDTH)
            g_textbuf[g_cursor_row][g_cursor_col++] = vga_entry(' ', g_color);
        advance_line();
        terminal_touch();
        return;
    }

    if (g_cursor_col >= TEXT_WIDTH) advance_line();
    g_textbuf[g_cursor_row][g_cursor_col++] = vga_entry(c, g_color);
    if (g_cursor_col >= TEXT_WIDTH) advance_line();
    else {
        g_head_row = g_cursor_row;
        if (g_follow_tail)
            g_view_top = g_head_row >= TERM_HEIGHT ? g_head_row - TERM_HEIGHT + 1 : 0;
    }

    terminal_touch();
}

void terminal_putentry_at(size_t row, size_t col, char c, uint8_t color) {
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) return;

    uint16_t cell = vga_entry(c, color);

    if (row < TERM_HEIGHT) {
        if (col >= TEXT_WIDTH || g_view_top + row >= SCROLLBACK_ROWS) return;
        g_textbuf[g_view_top + row][col] = cell;
    } else {
        g_overlay[row - TERM_HEIGHT][col] = cell;
    }
    terminal_touch();
}

void terminal_putc_at(size_t row, size_t col, char c) {
    terminal_putentry_at(row, col, c, g_color);
}

void terminal_write(const char* s) {
    if (!s) return;
    while (*s) terminal_putc(*s++);
}

void terminal_write_at(size_t row, size_t col, const char* s) {
    if (!s || row >= VGA_HEIGHT) return;
    size_t limit = row < TERM_HEIGHT ? TEXT_WIDTH : VGA_WIDTH;

    for (size_t i = 0; s[i] && col + i < limit; i++)
        terminal_putc_at(row, col + i, s[i]);
}

void vga_cursor_enable(void) {
    if (framebuffer_available()) return;
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void vga_cursor_hide(void) {
    if (framebuffer_available()) return;
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_cursor_set_pos(size_t row, size_t col) {
    if (framebuffer_available()) return;
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    if (col >= VGA_WIDTH) col = VGA_WIDTH - 1;

    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)pos);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(pos >> 8));
}

void terminal_init(void) {
    terminal_clear();
}

