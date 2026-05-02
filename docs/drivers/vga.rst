VGA Terminal
============

The VGA terminal driver manages text output using VGA text mode memory at
``0xB8000``.

It provides a scrollback-backed terminal area, a reserved scrollbar column, and
two overlay rows used by system UI such as the Janus tab bar.

Layout
------

``VGA_WIDTH``
    Total VGA text width, 80 columns.

``VGA_HEIGHT``
    Total VGA text height, 25 rows.

``TERM_HEIGHT``
    Main text area height. The bottom two rows are reserved for overlays.

``TEXT_WIDTH``
    Main text width. The last column is reserved for the scrollbar.

``OVERLAY_ROW0`` and ``OVERLAY_ROW1``
    Bottom rows reserved for overlay UI.

``SCROLLBACK_ROWS``
    Number of rows stored in the terminal scrollback buffer.

Functions
---------

``terminal_init()``
    Initialize and clear terminal state.

``terminal_clear()``
    Clear the full terminal, scrollback, cursor state, and overlay-visible VGA
    state.

``terminal_clear_row(row)``
    Clear a visible row. Rows inside the terminal area update scrollback;
    overlay rows draw directly to VGA memory.

``terminal_clear_text_area()``
    Clear the scrollback-backed text area while preserving overlay rows.

``terminal_write(s)``
    Write a string at the current cursor.

``terminal_write_at(row, col, s)``
    Write a string at a visible row and column.

``terminal_putc(c)``
    Write one character at the current cursor.

``terminal_putc_at(row, col, c)``
    Write one character at a visible row and column.

``terminal_putentry_at(row, col, c, color)``
    Write one colored VGA text cell.

``terminal_scroll_view_up()``
    Move scrollback view upward.

``terminal_scroll_view_down()``
    Move scrollback view downward.

``terminal_ensure_row_visible(row)``
    Adjust the scrollback viewport to show a given buffer row.

``terminal_follow_tail()``
    Return scrollback view to the newest output.

``terminal_is_following_tail()``
    Return whether the terminal view is following new output.

``terminal_get_row()`` / ``terminal_get_col()``
    Return the visible cursor row/column.

``terminal_get_buffer_row()``
    Return the cursor row in scrollback-buffer coordinates.

``terminal_get_view_top()``
    Return the first scrollback row currently visible.

``terminal_set_cursor_pos(row, col)``
    Move the logical and hardware cursor.

``vga_cursor_enable()``
    Enable the VGA hardware cursor.

``vga_cursor_hide()``
    Hide the VGA hardware cursor.

``vga_cursor_set_pos(row, col)``
    Move the VGA hardware cursor directly.

Notes
-----

The main terminal area is backed by ``g_textbuf``. Writes to normal terminal
rows update both the scrollback buffer and visible VGA memory. Overlay rows are
drawn directly to VGA memory so they can be used for persistent UI elements
like Janus tabs.

The last text column is reserved for a simple scrollbar marker.
