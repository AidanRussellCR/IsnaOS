#pragma once
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TERM_HEIGHT (VGA_HEIGHT - 2)
#define OVERLAY_ROW0 (VGA_HEIGHT - 2)
#define OVERLAY_ROW1 (VGA_HEIGHT - 1)
#define TEXT_WIDTH (VGA_WIDTH - 1)
#define SCROLLBAR_COL (VGA_WIDTH - 1)
#define SCROLLBACK_ROWS 300

/**
 * terminal_init - initialize VGA terminal state
 */
void terminal_init(void);

/**
 * terminal_clear - clear entire terminal and scrollback buffer
 */
void terminal_clear(void);

/**
 * terminal_clear_row - clear one visible terminal or overlay row
 * @row: visible row index
 */
void terminal_clear_row(size_t row);

/**
 * terminal_clear_text_area - clear text scrollback area, preserving overlay rows
 */
void terminal_clear_text_area(void);

/**
 * terminal_write - write a null-terminated string at the current cursor
 * @s: string to write
 */
void terminal_write(const char* s);

/**
 * terminal_write_at - write a string at a visible row/column
 * @row: visible row
 * @col: visible column
 * @s: string to write
 */
void terminal_write_at(size_t row, size_t col, const char* s);

/**
 * terminal_putc - write one character at the current cursor
 * @c: character to write
 */
void terminal_putc(char c);

/**
 * terminal_putc_at - write one character at a visible row/column
 * @row: visible row
 * @col: visible column
 * @c: character to write
 */
void terminal_putc_at(size_t row, size_t col, char c);

/**
 * terminal_putentry_at - write one colored VGA text cell
 * @row: visible row
 * @col: visible col
 * @c: character to write
 * @color: VGA text color attribute
 */
void terminal_putentry_at(size_t row, size_t col, char c, uint8_t color);

/**
 * terminal_get_buffer_row - get cursor row in scrollback buffer coordinates
 */
size_t terminal_get_buffer_row(void);

/**
 * terminal_get_view_top - get first scrollback row currently visible
 */
size_t terminal_get_view_top(void);


void terminal_scroll_view_up(void);
void terminal_scroll_view_down(void);
void terminal_ensure_row_visible(size_t row);
void terminal_follow_tail(void);
int terminal_is_following_tail(void);


size_t terminal_get_row(void);
size_t terminal_get_col(void);
void terminal_set_cursor_pos(size_t row, size_t col);

/* VGA hardware cursor controls */
void vga_cursor_enable(void);
void vga_cursor_hide(void);
void vga_cursor_set_pos(size_t row, size_t col);
