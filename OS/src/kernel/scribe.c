#include <stddef.h>
#include <stdint.h>
#include "kernel/scribe.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "kernel/sched.h"
#include "lib/str.h"
#include "fs/vfs.h"
#include "mm/heap.h"

#define SCRIBE_MAX_LINES 512
#define SCRIBE_STATUS_ROW (TERM_HEIGHT - 2)
#define SCRIBE_CMD_ROW (TERM_HEIGHT - 1)
#define SCRIBE_TEXT_ROWS (TERM_HEIGHT - 2)

#define SCRIBE_GUTTER_WIDTH 5
#define SCRIBE_TEXT_COL0 SCRIBE_GUTTER_WIDTH
#define SCRIBE_TEXT_WIDTH (TEXT_WIDTH - SCRIBE_GUTTER_WIDTH)
#define SCRIBE_GUTTER_COLOR 0x70 // black text white background

typedef enum {
	SCRIBE_MODE_WRITE = 0,
	SCRIBE_MODE_COMMAND
} scribe_mode_t;

typedef struct {
	char* lines[SCRIBE_MAX_LINES];
	size_t line_count;

	size_t cur_line;
	size_t cur_col;

	size_t top_line;

	int modified;
	scribe_mode_t mode;

	char filename[32];
	char message[80];
} scribe_t;

static size_t scribe_strlen(const char* s) {
	return s ? kstrlen(s) : 0;
}

static void scribe_set_message(scribe_t* ed, const char* msg) {
	if (!ed) return;
	kstrncpy0(ed->message, msg ? msg : "", sizeof(ed->message));
}

static char* scribe_make_empty_line(void) {
	char* s = (char*)kmalloc(1);
	if (!s) return 0;
	s[0] = '\0';
	return s;
}

static void scribe_init_empty(scribe_t* ed, const char* filename) {
	for (size_t i = 0; i < SCRIBE_MAX_LINES; i++) ed->lines[i] = 0;

	ed->line_count = 1;
	ed->lines[0] = scribe_make_empty_line();

	ed->cur_line = 0;
	ed->cur_col = 0;
	ed->top_line = 0;
	ed->modified = 0;
	ed->mode = SCRIBE_MODE_WRITE;

	kstrncpy0(ed->filename, filename ? filename : "untitled.txt", sizeof(ed->filename));
	kstrncpy0(ed->message, "WRITE mode  |  Esc=COMMAND", sizeof(ed->message));
}

static void scribe_free(scribe_t* ed) {
	if (!ed) return;
	for (size_t i = 0; i < ed->line_count; i++) {
		if (ed->lines[i]) {
			kfree(ed->lines[i]);
			ed->lines[i] = 0;
		}
	}
}

static int scribe_load_from_text(scribe_t* ed, const char* text) {
	scribe_init_empty(ed, ed->filename);

	if (!text || !text[0]) return 1;

	// clear empty default line
	kfree(ed->lines[0]);
	ed->lines[0] = 0;
	ed->line_count = 0;

	const char* p = text;
	while (*p) {
		if (ed->line_count >= SCRIBE_MAX_LINES) break;

		const char* start = p;
		while (*p && *p != '\n') p++;

		size_t len = (size_t)(p - start);
		if (len > 0 && start[len - 1] == '\r') len--;

		char* line = (char*)kmalloc(len + 1);
		if (!line) return 0;

		for (size_t i = 0; i < len; i++) line[i] = start[i];
		line[len] = '\0';

		ed->lines[ed->line_count++] = line;

		if (*p == '\n') p++;
	}

	if (ed->line_count == 0) {
		ed->line_count = 1;
		ed->lines[0] = scribe_make_empty_line();
	}

	return 1;
}

static void scribe_keep_cursor_visible(scribe_t* ed) {
	if (ed->cur_line < ed->top_line) ed->top_line = ed->cur_line;
	if (ed->cur_line >= ed->top_line + SCRIBE_TEXT_ROWS) {
		ed->top_line = ed->cur_line - (SCRIBE_TEXT_ROWS - 1);
	}
}

static void scribe_draw_status(scribe_t* ed) {
	terminal_clear_row(SCRIBE_STATUS_ROW);

	terminal_write_at(SCRIBE_STATUS_ROW, 0,
		(ed->mode == SCRIBE_MODE_WRITE) ? "[WRITE] " : "[COMMAND] ");

	terminal_write_at(SCRIBE_STATUS_ROW, 10, ed->filename);

	if (ed->modified) {
		terminal_write_at(SCRIBE_STATUS_ROW, 10 + kstrlen(ed->filename) + 1, "*");
	}

	char info[40];
	size_t ln = ed->cur_line + 1;
	size_t col = ed->cur_col + 1;

	int p = 0;
	info[p++] = 'L';
	info[p++] = 'n';
	info[p++] = ' ';

	// line number
	{
		char tmp[16];
		int tp = 0;
		size_t v = ln;
		if (v == 0) tmp[tp++] = '0';
		while (v > 0) { tmp[tp++] = (char)('0' + (v % 10)); v /= 10; }
		while (tp > 0) info[p++] = tmp[--tp];
	}

	info[p++] = ' ';
	info[p++] = 'C';
	info[p++] = 'o';
	info[p++] = 'l';
	info[p++] = ' ';

	{
		char tmp[16];
		int tp = 0;
		size_t v = col;
		if (v == 0) tmp[tp++] = '0';
		while (v > 0) { tmp[tp++] = (char)('0' + (v % 10)); v /= 10; }
		while (tp > 0) info[p++] = tmp[--tp];
	}
	info[p] = '\0';

	terminal_write_at(SCRIBE_STATUS_ROW, 32, info);

	if (ed->message[0]) {
		terminal_write_at(SCRIBE_STATUS_ROW, 48, ed->message);
	}
}

static void scribe_draw_command(scribe_t* ed) {
	terminal_clear_row(SCRIBE_CMD_ROW);

	if (ed->mode == SCRIBE_MODE_WRITE) {
		terminal_write_at(SCRIBE_CMD_ROW, 0,
			"Esc=COMMAND  Arrows=Move  Tab=Indent  Enter=Split  Backspace/Delete=Edit");
	} else {
		terminal_write_at(SCRIBE_CMD_ROW, 0,
			"i=write  w=save  q=quit  x=save+quit  /=search  g=goto");
	}
}

static void scribe_draw_gutter(size_t row, size_t line_no) {
	char buf[SCRIBE_GUTTER_WIDTH];
	for (size_t i = 0; i < SCRIBE_GUTTER_WIDTH; i++) buf[i] = ' ';

	size_t pos = SCRIBE_GUTTER_WIDTH - 2;
	size_t v = line_no;

	if (v == 0) {
		buf[pos] = '0';
	} else {
		while (v > 0) {
			buf[pos] = (char)('0' + (v % 10));
			v /= 10;
			if (pos == 0) break;
			pos--;
		}
	}

	for (size_t i = 0; i < SCRIBE_GUTTER_WIDTH; i++) {
		terminal_putentry_at(row, i, buf[i], SCRIBE_GUTTER_COLOR);
	}
}

static void scribe_draw_text(scribe_t* ed) {
	for (size_t vr = 0; vr < SCRIBE_TEXT_ROWS; vr++) {
		terminal_clear_row(vr);

		size_t line_index = ed->top_line + vr;

		if (line_index < ed->line_count) {
			scribe_draw_gutter(vr, line_index + 1);

			const char* line = ed->lines[line_index];
			if (line) {
				terminal_write_at(vr, SCRIBE_TEXT_COL0, line);
			}
		} else {
			for (size_t i = 0; i < SCRIBE_GUTTER_WIDTH; i++) {
				terminal_putentry_at(vr, i, ' ', SCRIBE_GUTTER_COLOR);
			}
		}
	}
}

static void scribe_render(scribe_t* ed) {
	scribe_keep_cursor_visible(ed);
	scribe_draw_text(ed);
	scribe_draw_status(ed);
	scribe_draw_command(ed);

	size_t screen_row = ed->cur_line - ed->top_line;
	size_t screen_col = SCRIBE_TEXT_COL0 + ed->cur_col;
	if (screen_row >= SCRIBE_TEXT_ROWS) screen_row = SCRIBE_TEXT_ROWS - 1;
	if (screen_col >= TEXT_WIDTH) screen_col = TEXT_WIDTH - 1;

	terminal_set_cursor_pos(screen_row, screen_col);
}

static int scribe_insert_line(scribe_t* ed, size_t index, char* line) {
	if (ed->line_count >= SCRIBE_MAX_LINES) return 0;
	if (index > ed->line_count) index = ed->line_count;

	for (size_t i = ed->line_count; i > index; i--) {
		ed->lines[i] = ed->lines[i - 1];
	}
	ed->lines[index] = line;
	ed->line_count++;
	return 1;
}

static void scribe_delete_line(scribe_t* ed, size_t index) {
	if (ed->line_count == 0 || index >= ed->line_count) return;

	kfree(ed->lines[index]);
	for (size_t i = index; i + 1 < ed->line_count; i++) {
		ed->lines[i] = ed->lines[i + 1];
	}
	ed->line_count--;

	if (ed->line_count == 0) {
		ed->line_count = 1;
		ed->lines[0] = scribe_make_empty_line();
	}

	if (ed->cur_line >= ed->line_count) ed->cur_line = ed->line_count - 1;
}

static int scribe_insert_char(scribe_t* ed, char ch) {
	char* line = ed->lines[ed->cur_line];
	size_t len = scribe_strlen(line);

	char* nline = (char*)kmalloc(len + 2);
	if (!nline) return 0;

	size_t p = 0;
	for (size_t i = 0; i < ed->cur_col && i < len; i++) nline[p++] = line[i];
	nline[p++] = ch;
	for (size_t i = ed->cur_col; i < len; i++) nline[p++] = line[i];
	nline[p] = '\0';

	kfree(line);
	ed->lines[ed->cur_line] = nline;
	ed->cur_col++;
	ed->modified = 1;
	return 1;
}

static int scribe_backspace(scribe_t* ed) {
	if (ed->cur_line == 0 && ed->cur_col == 0) return 1;

	char* line = ed->lines[ed->cur_line];
	size_t len = scribe_strlen(line);

	if (ed->cur_col > 0) {
		char* nline = (char*)kmalloc(len);
		if (!nline) return 0;

		size_t p = 0;
		for (size_t i = 0; i < len; i++) {
			if (i == ed->cur_col - 1) continue;
			nline[p++] = line[i];
		}
		nline[p] = '\0';

		kfree(line);
		ed->lines[ed->cur_line] = nline;
		ed->cur_col--;
		ed->modified = 1;
		return 1;
	}

	// merge with the previous line
	if (ed->cur_line > 0) {
		char* prev = ed->lines[ed->cur_line - 1];
		char* curr = ed->lines[ed->cur_line];
		size_t prev_len = scribe_strlen(prev);
		size_t curr_len = scribe_strlen(curr);

		char* merged = (char*)kmalloc(prev_len + curr_len + 1);
		if (!merged) return 0;

		for (size_t i = 0; i < prev_len; i++) merged[i] = prev[i];
		for (size_t i = 0; i < curr_len; i++) merged[prev_len + i] = curr[i];
		merged[prev_len + curr_len] = '\0';

		kfree(prev);
		ed->lines[ed->cur_line - 1] = merged;
		ed->cur_col = prev_len;
		ed->cur_line--;

		scribe_delete_line(ed, ed->cur_line + 1);
		ed->modified = 1;
		return 1;
	}

	return 1;
}

static int scribe_delete_char(scribe_t* ed) {
	char* line = ed->lines[ed->cur_line];
	size_t len = scribe_strlen(line);

	if (ed->cur_col < len) {
		char* nline = (char*)kmalloc(len);
		if (!nline) return 0;

		size_t p = 0;
		for (size_t i = 0; i < len; i++) {
			if (i == ed->cur_col) continue;
			nline[p++] = line[i];
		}
		nline[p] = '\0';

		kfree(line);
		ed->lines[ed->cur_line] = nline;
		ed->modified = 1;
		return 1;
	}

	// merge next line if applicable
	if (ed->cur_line + 1 < ed->line_count) {
		char* next = ed->lines[ed->cur_line + 1];
		size_t curr_len = scribe_strlen(line);
		size_t next_len = scribe_strlen(next);

		char* merged = (char*)kmalloc(curr_len + next_len + 1);
		if (!merged) return 0;

		for (size_t i = 0; i < curr_len; i++) merged[i] = line[i];
		for (size_t i = 0; i < next_len; i++) merged[curr_len + i] = next[i];
		merged[curr_len + next_len] = '\0';

		kfree(line);
		ed->lines[ed->cur_line] = merged;
		scribe_delete_line(ed, ed->cur_line + 1);
		ed->modified = 1;
		return 1;
	}

	return 1;
}

static int scribe_split_line(scribe_t* ed) {
	char* line = ed->lines[ed->cur_line];
	size_t len = scribe_strlen(line);

	char* left = (char*)kmalloc(ed->cur_col + 1);
	char* right = (char*)kmalloc((len - ed->cur_col) + 1);
	if (!left || !right) return 0;

	for (size_t i = 0; i < ed->cur_col; i++) left[i] = line[i];
	left[ed->cur_col] = '\0';

	size_t rp = 0;
	for (size_t i = ed->cur_col; i < len; i++) right[rp++] = line[i];
	right[rp] = '\0';

	kfree(line);
	ed->lines[ed->cur_line] = left;

	if (!scribe_insert_line(ed, ed->cur_line + 1, right)) {
		return 0;
	}

	ed->cur_line++;
	ed->cur_col = 0;
	ed->modified = 1;
	return 1;
}

static int scribe_save(scribe_t* ed) {
	size_t total = 0;
	for (size_t i = 0; i < ed->line_count; i++) {
		total += scribe_strlen(ed->lines[i]);
		if (i + 1 < ed->line_count) total++;
	}

	char* out = (char*)kmalloc(total + 1);
	if (!out) return 0;

	size_t p = 0;
	for (size_t i = 0; i < ed->line_count; i++) {
		const char* line = ed->lines[i];
		size_t len = scribe_strlen(line);
		for (size_t j = 0; j < len; j++) out[p++] = line[j];
		if (i + 1 < ed->line_count) out[p++] = '\n';
	}
	out[p] = '\0';

	vfs_status_t st = vfs_carve(ed->filename, out);
	kfree(out);

	if (st != VFS_OK) return 0;

	ed->modified = 0;
	scribe_set_message(ed, "Saved.");
	return 1;
}

static void scribe_prompt_line(const char* prompt, char* out, size_t cap) {
	size_t len = 0;
	out[0] = '\0';

	int dirty = 1;

	for (;;) {
		if (dirty) {
			terminal_clear_row(SCRIBE_CMD_ROW);
			terminal_write_at(SCRIBE_CMD_ROW, 0, prompt);
			terminal_write_at(SCRIBE_CMD_ROW, kstrlen(prompt), out);

			size_t cursor_col = kstrlen(prompt) + len;
			if (cursor_col >= TEXT_WIDTH) cursor_col = TEXT_WIDTH - 1;
			terminal_set_cursor_pos(SCRIBE_CMD_ROW, cursor_col);

			dirty = 0;
		}

		key_event_t ev;
		if (!keyboard_try_get_key(&ev)) {
			yield();
			continue;
		}

		if (ev.type == KEY_ENTER) {
			return;
		} else if (ev.type == KEY_ESC) {
			out[0] = '\0';
			return;
		} else if (ev.type == KEY_BACKSPACE) {
			if (len > 0) {
				len--;
				out[len] = '\0';
				dirty = 1;
			}
		} else if (ev.type == KEY_CHAR) {
			if (len + 1 < cap) {
				out[len++] = ev.ch;
				out[len] = '\0';
				dirty = 1;
			}
		}
	}
}

static int scribe_confirm_quit(void) {
	for (;;) {
		terminal_clear_row(SCRIBE_CMD_ROW);
		terminal_write_at(SCRIBE_CMD_ROW, 0, "Quit without saving? (y/n): ");
		terminal_set_cursor_pos(SCRIBE_CMD_ROW, 29);

		key_event_t ev;
		if (!keyboard_try_get_key(&ev)) {
			yield();
			continue;
		}

		if (ev.type == KEY_CHAR) {
			char c = ev.ch;
			if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');

			if (c == 'y') {
				terminal_putc_at(SCRIBE_CMD_ROW, 29, 'y');
				return 1;
			}
			if (c == 'n') {
				terminal_putc_at(SCRIBE_CMD_ROW, 29, 'n');
				return 0;
			}
		} else if (ev.type == KEY_ESC) {
			return 0;
		}
	}
}

static void scribe_search(scribe_t* ed) {
	char needle[64];
	scribe_prompt_line("Search: ", needle, sizeof(needle));

	if (!needle[0]) {
		scribe_set_message(ed, "Search canceled.");
		return;
	}

	for (size_t i = ed->cur_line; i < ed->line_count; i++) {
		const char* found = kstrstr(ed->lines[i], needle);
		if (found) {
			ed->cur_line = i;
			ed->cur_col = (size_t)(found - ed->lines[i]);
			scribe_set_message(ed, "Match found.");
			return;
		}
	}

	for (size_t i = 0; i < ed->cur_line; i++) {
		const char* found = kstrstr(ed->lines[i], needle);
		if (found) {
			ed->cur_line = i;
			ed->cur_col = (size_t)(found - ed->lines[i]);
			scribe_set_message(ed, "Match found.");
			return;
		}
	}

	scribe_set_message(ed, "No match.");
}

static void scribe_goto(scribe_t* ed) {
	char buf[16];
	scribe_prompt_line("Goto line: ", buf, sizeof(buf));

	uint32_t line_num = 0;
	if (!parse_u32(buf, &line_num) || line_num == 0) {
		scribe_set_message(ed, "Bad line number.");
		return;
	}

	if (line_num > ed->line_count) line_num = (uint32_t)ed->line_count;
	ed->cur_line = (size_t)(line_num - 1);

	size_t len = scribe_strlen(ed->lines[ed->cur_line]);
	if (ed->cur_col > len) ed->cur_col = len;

	scribe_set_message(ed, "Moved.");
}

void scribe_open(const char* filename) {
	if (!filename || !filename[0]) {
		terminal_write("Usage: scribe <file>\n");
		return;
	}

	scribe_t ed;
	kstrncpy0(ed.filename, filename, sizeof(ed.filename));

	const char* text = 0;
	vfs_status_t st = vfs_insp(filename, &text);

	if (st == VFS_ERR_NOT_FOUND) {
		st = vfs_fab(filename);
		if (st != VFS_OK && st != VFS_ERR_EXISTS) {
			terminal_write("Could not create file.\n");
			return;
		}
		scribe_init_empty(&ed, filename);
		scribe_set_message(&ed, "New file.");
	} else if (st == VFS_OK) {
		if (!scribe_load_from_text(&ed, text)) {
			terminal_write("Could not load file.\n");
			return;
		}
		kstrncpy0(ed.filename, filename, sizeof(ed.filename));
		scribe_set_message(&ed, "WRITE mode  |  Esc=COMMAND");
	} else {
		terminal_write("Could not open file.\n");
		return;
	}

	terminal_clear_text_area();

	int needs_redraw = 1;

	for (;;) {
		if (needs_redraw) {
			scribe_render(&ed);
			needs_redraw = 0;
		}

		key_event_t ev;
		if (!keyboard_try_get_key(&ev)) {
			yield();
			continue;
		}

		needs_redraw = 1;

		if (ed.mode == SCRIBE_MODE_WRITE) {
			size_t line_len = scribe_strlen(ed.lines[ed.cur_line]);

			if (ev.type == KEY_LEFT) {
				if (ed.cur_col > 0) ed.cur_col--;
				else if (ed.cur_line > 0) {
					ed.cur_line--;
					ed.cur_col = scribe_strlen(ed.lines[ed.cur_line]);
				}
			} else if (ev.type == KEY_RIGHT) {
				if (ed.cur_col < line_len) ed.cur_col++;
				else if (ed.cur_line + 1 < ed.line_count) {
					ed.cur_line++;
					ed.cur_col = 0;
				}
			} else if (ev.type == KEY_UP) {
				if (ed.cur_line > 0) ed.cur_line--;
				line_len = scribe_strlen(ed.lines[ed.cur_line]);
				if (ed.cur_col > line_len) ed.cur_col = line_len;
			} else if (ev.type == KEY_DOWN) {
				if (ed.cur_line + 1 < ed.line_count) ed.cur_line++;
				line_len = scribe_strlen(ed.lines[ed.cur_line]);
				if (ed.cur_col > line_len) ed.cur_col = line_len;
			} else if (ev.type == KEY_BACKSPACE) {
				if (!scribe_backspace(&ed)) scribe_set_message(&ed, "Out of memory.");
			} else if (ev.type == KEY_DELETE) {
				if (!scribe_delete_char(&ed)) scribe_set_message(&ed, "Out of memory.");
			} else if (ev.type == KEY_ENTER) {
				if (!scribe_split_line(&ed)) scribe_set_message(&ed, "Out of memory.");
			} else if (ev.type == KEY_CHAR) {
				if (ev.ch == '\t') {
					for (int i = 0; i < 4; i++) {
						if (!scribe_insert_char(&ed, ' ')) {
							scribe_set_message(&ed, "Out of memory.");
							break;
						}
					}
				} else {
					if (!scribe_insert_char(&ed, ev.ch)) scribe_set_message(&ed, "Out of memory.");
				}
			} else if (ev.type == KEY_PAGEUP) {
				if (ed.top_line > 0) ed.top_line--;
			} else if (ev.type == KEY_PAGEDOWN) {
				if (ed.top_line + SCRIBE_TEXT_ROWS < ed.line_count) ed.top_line++;
			} else if (ev.type == KEY_NONE) {
				// do nothing
			} else {
				// do nothing
			}

			// Esc handling
			if (ev.type == KEY_ESC) {
				ed.mode = SCRIBE_MODE_COMMAND;
				scribe_set_message(&ed, "[COMMAND] i=write w=save q=quit x=save+quit /=search g=goto");
			}

		} else {
			if (ev.type == KEY_CHAR) {
				if (ev.ch == 'i') {
					ed.mode = SCRIBE_MODE_WRITE;
					scribe_set_message(&ed, "WRITE mode  |  Esc=COMMAND");
				} else if (ev.ch == 'w') {
					if (!scribe_save(&ed)) scribe_set_message(&ed, "Save failed.");
				} else if (ev.ch == 'q') {
					if (ed.modified) {
						if (scribe_confirm_quit()) break;
						scribe_set_message(&ed, "Quit canceled.");
					} else {
						break;
					}
				} else if (ev.ch == 'x') {
					if (scribe_save(&ed)) break;
					else scribe_set_message(&ed, "Save failed.");
				} else if (ev.ch == '/') {
					scribe_search(&ed);
				} else if (ev.ch == 'g') {
					scribe_goto(&ed);
				}
			}
		}
	}

	terminal_clear_text_area();
	scribe_free(&ed);
}

