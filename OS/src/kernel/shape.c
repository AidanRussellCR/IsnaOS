#include <stddef.h>
#include <stdint.h>
#include "kernel/shape.h"
#include "kernel/glm.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/vga.h"
#include "lib/str.h"

// Shape v1.0

#define SHAPE_MAX_LABELS 128
#define SHAPE_MAX_LINE   160

typedef enum {
	SHAPE_SEC_NONE = 0,
	SHAPE_SEC_CODE,
	SHAPE_SEC_DATA
} shape_sec_t;

typedef struct {
	char name[32];
	shape_sec_t sec;
	uint32_t off; // offset within that section
} shape_label_t;

static int is_space_ch(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char* trim_left(char* s) {
	while (*s && is_space_ch(*s)) s++;
	return s;
}

static void trim_right(char* s) {
	size_t n = kstrlen(s);
	while (n > 0 && is_space_ch(s[n - 1])) {
		s[n - 1] = '\0';
		n--;
	}
}

static void strip_comment(char* s) {
	for (size_t i = 0; s[i]; i++) {
		if (s[i] == ';') {
			s[i] = '\0';
			return;
		}
	}
}

static int next_line(const char** p, char* out, size_t cap) {
	if (!p || !*p || !**p) return 0;

	size_t i = 0;
	while (**p && **p != '\n' && i + 1 < cap) {
		if (**p != '\r') out[i++] = **p;
		(*p)++;
	}
	out[i] = '\0';

	if (**p == '\n') (*p)++;
	return 1;
}

static int parse_ident(const char* s, char* out, size_t cap) {
	size_t i = 0;
	if (!s || !s[0]) return 0;

	while (s[i] && !is_space_ch(s[i]) && s[i] != ':' && i + 1 < cap) {
		out[i] = s[i];
		i++;
	}
	out[i] = '\0';
	return i > 0;
}

static int add_label(shape_label_t* labels, int* count, const char* name, shape_sec_t sec, uint32_t off) {
	if (*count >= SHAPE_MAX_LABELS) return 0;

	for (int i = 0; i < *count; i++) {
		if (streq(labels[i].name, name)) return 0;
	}

	kstrncpy0(labels[*count].name, name, sizeof(labels[*count].name));
	labels[*count].sec = sec;
	labels[*count].off = off;
	(*count)++;
	return 1;
}

static int find_label(shape_label_t* labels, int count, const char* name, shape_sec_t* sec_out, uint32_t* off_out) {
	for (int i = 0; i < count; i++) {
		if (streq(labels[i].name, name)) {
			if (sec_out) *sec_out = labels[i].sec;
			if (off_out) *off_out = labels[i].off;
			return 1;
		}
	}
	return 0;
}

static int parse_string_literal(const char* s, uint8_t* out, size_t* out_len, size_t cap, int add_nul) {
	s = trim_left((char*)s);
	if (*s != '"') return 0;
	s++;

	size_t n = 0;
	while (*s && *s != '"') {
		uint8_t ch;

		if (*s == '\\') {
			s++;
			if (*s == 'n') ch = '\n';
			else if (*s == 't') ch = '\t';
			else if (*s == '\\') ch = '\\';
			else if (*s == '"') ch = '"';
			else return 0;
			s++;
		} else {
			ch = (uint8_t)*s++;
		}

		if (n >= cap) return 0;
		out[n++] = ch;
	}

	if (*s != '"') return 0;

	if (add_nul) {
		if (n >= cap) return 0;
		out[n++] = 0;
	}

	if (out_len) *out_len = n;
	return 1;
}

static int emit_u32_le(uint8_t* buf, size_t* p, size_t cap, uint32_t v) {
	if (*p + 4 > cap) return 0;
	buf[(*p)++] = (uint8_t)(v & 0xFF);
	buf[(*p)++] = (uint8_t)((v >> 8) & 0xFF);
	buf[(*p)++] = (uint8_t)((v >> 16) & 0xFF);
	buf[(*p)++] = (uint8_t)((v >> 24) & 0xFF);
	return 1;
}

static int emit_byte(uint8_t* buf, size_t* p, size_t cap, uint8_t b) {
	if (*p >= cap) return 0;
	buf[(*p)++] = b;
	return 1;
}

static int code_size_of(const char* line) {
	if (streq(line, "ret")) return 1;
	if (streq(line, "yield")) return 7;
	if (streq(line, "exit")) return 13;
	if (starts_with(line, "speak ")) return 15;
	return -1;
}

int shape_asm_to_glm(const char* input_name, const char* output_stem) {
	const char* src = 0;
	vfs_status_t st = vfs_insp(input_name, &src);
	if (st != VFS_OK || !src) {
		terminal_write("Could not open source.\n");
		return 0;
	}

	shape_label_t labels[SHAPE_MAX_LABELS];
	int label_count = 0;

	shape_sec_t sec = SHAPE_SEC_NONE;
	uint32_t code_size = 0;
	uint32_t data_size = 0;

	// PASS 1
	{
		const char* p = src;
		char raw[SHAPE_MAX_LINE];

		while (next_line(&p, raw, sizeof(raw))) {
			strip_comment(raw);
			trim_right(raw);

			char* line = trim_left(raw);
			if (!line[0]) continue;

			if (streq(line, ".code")) {
				sec = SHAPE_SEC_CODE;
				continue;
			}
			if (streq(line, ".data")) {
				sec = SHAPE_SEC_DATA;
				continue;
			}

			// label
			{
				char* colon = 0;
				for (size_t i = 0; line[i]; i++) {
					if (line[i] == ':') { colon = &line[i]; break; }
				}
				if (colon) {
					char name[32];
					size_t n = (size_t)(colon - line);
					if (n == 0 || n >= sizeof(name)) {
						terminal_write("Bad label.\n");
						return 0;
					}
					for (size_t i = 0; i < n; i++) name[i] = line[i];
					name[n] = '\0';

					uint32_t off = (sec == SHAPE_SEC_CODE) ? code_size : data_size;
					if (!add_label(labels, &label_count, name, sec, off)) {
						terminal_write("Duplicate or bad label.\n");
						return 0;
					}

					line = trim_left(colon + 1);
					if (!line[0]) continue;
				}
			}

			if (sec == SHAPE_SEC_CODE) {
				int sz = code_size_of(line);
				if (sz < 0) {
					terminal_write("Unknown code instruction.\n");
					return 0;
				}
				code_size += (uint32_t)sz;
			} else if (sec == SHAPE_SEC_DATA) {
				if (starts_with(line, "asciz ")) {
					uint8_t tmp[SHAPE_MAX_LINE];
					size_t n = 0;
					if (!parse_string_literal(line + 6, tmp, &n, sizeof(tmp), 1)) {
						terminal_write("Bad asciz string.\n");
						return 0;
					}
					data_size += (uint32_t)n;
				} else {
					terminal_write("Unknown data directive.\n");
					return 0;
				}
			} else {
				terminal_write("No section selected.\n");
				return 0;
			}
		}
	}

	// require entry label in code
	{
		shape_sec_t lsec;
		uint32_t loff;
		if (!find_label(labels, label_count, "entry", &lsec, &loff) || lsec != SHAPE_SEC_CODE) {
			terminal_write("Missing code label: entry\n");
			return 0;
		}
	}

	uint8_t* code = 0;
	uint8_t* data = 0;

	if (code_size > 0) {
		code = (uint8_t*)kmalloc(code_size);
		if (!code) return 0;
	}
	if (data_size > 0) {
		data = (uint8_t*)kmalloc(data_size);
		if (!data) {
			if (code) kfree(code);
			return 0;
		}
	}

	size_t code_p = 0;
	size_t data_p = 0;

	// PASS 2
	{
		const char* p = src;
		char raw[SHAPE_MAX_LINE];

		sec = SHAPE_SEC_NONE;

		while (next_line(&p, raw, sizeof(raw))) {
			strip_comment(raw);
			trim_right(raw);

			char* line = trim_left(raw);
			if (!line[0]) continue;

			if (streq(line, ".code")) { sec = SHAPE_SEC_CODE; continue; }
			if (streq(line, ".data")) { sec = SHAPE_SEC_DATA; continue; }

			// skip label if present
			{
				char* colon = 0;
				for (size_t i = 0; line[i]; i++) {
					if (line[i] == ':') { colon = &line[i]; break; }
				}
				if (colon) {
					line = trim_left(colon + 1);
					if (!line[0]) continue;
				}
			}

			if (sec == SHAPE_SEC_CODE) {
				if (streq(line, "ret")) {
					if (!emit_byte(code, &code_p, code_size, 0xC3)) goto emit_fail;
				} else if (streq(line, "yield")) {
					// mov eax,[esp+4] ; call dword ptr [eax+8]
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x44)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x50)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x08)) goto emit_fail;
				} else if (streq(line, "exit")) {
					// mov eax,[esp+4] ; push 0 ; call [eax+16] ; add esp,4 ; ret
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x44)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x6A)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x00)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x50)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x10)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC4)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC3)) goto emit_fail;
				} else if (starts_with(line, "speak ")) {
					char name[32];
					shape_sec_t lsec;
					uint32_t loff;

					if (!parse_ident(line + 6, name, sizeof(name))) {
						terminal_write("Bad speak target.\n");
						goto fail;
					}
					if (!find_label(labels, label_count, name, &lsec, &loff) || lsec != SHAPE_SEC_DATA) {
						terminal_write("Unknown data label in speak.\n");
						goto fail;
					}

					// final image offset of data label = code_size + data_offset
					uint32_t image_off = code_size + loff;

					/* 
					mov eax,[esp+4]
					push imm32(data_offset)
					call dword ptr [eax+12]
					add esp,4
					*/
					
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x44)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;

					if (!emit_byte(code, &code_p, code_size, 0x68)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, image_off)) goto emit_fail;

					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x50)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x0C)) goto emit_fail;

					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC4)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
				} else {
					terminal_write("Unknown code instruction.\n");
					goto fail;
				}
			} else if (sec == SHAPE_SEC_DATA) {
				if (starts_with(line, "asciz ")) {
					uint8_t tmp[SHAPE_MAX_LINE];
					size_t n = 0;
					if (!parse_string_literal(line + 6, tmp, &n, sizeof(tmp), 1)) {
						terminal_write("Bad asciz string.\n");
						goto fail;
					}
					for (size_t i = 0; i < n; i++) {
						if (!emit_byte(data, &data_p, data_size, tmp[i])) goto emit_fail;
					}
				} else {
					terminal_write("Unknown data directive.\n");
					goto fail;
				}
			}
		}
	}

	// pack into .glm
	{
		glm_header_t h;
		h.magic = GLM_MAGIC;
		h.version = GLM_VERSION;
		h.flags = GLM_FLAG_NONE;

		h.entry_offset = 0;
		{
			shape_sec_t esec;
			uint32_t eoff;

			if (!find_label(labels, label_count, "entry", &esec, &eoff) || esec != SHAPE_SEC_CODE) {
				terminal_write("Missing code label: entry\n");
				goto fail;
			}

			h.entry_offset = eoff;
		}

		h.code_offset = (uint32_t)sizeof(glm_header_t);
		h.code_size = code_size;

		h.data_offset = h.code_offset + h.code_size;
		h.data_size = data_size;

		h.bss_size = 0;
		h.api_version = GLM_API_VERSION;

		size_t total = sizeof(glm_header_t) + code_size + data_size;
		uint8_t* out = (uint8_t*)kmalloc(total);
		if (!out) {
			terminal_write("Out of memory.\n");
			goto fail;
		}

		for (size_t i = 0; i < sizeof(glm_header_t); i++) {
			out[i] = ((const uint8_t*)&h)[i];
		}
		for (size_t i = 0; i < code_size; i++) {
			out[h.code_offset + i] = code[i];
		}
		for (size_t i = 0; i < data_size; i++) {
			out[h.data_offset + i] = data[i];
		}

		char outname[40];
		kstrncpy0(outname, output_stem, sizeof(outname));
		{
			size_t n = kstrlen(outname);
			if (n + 4 >= sizeof(outname)) {
				kfree(out);
				terminal_write("Output name too long.\n");
				goto fail;
			}
			outname[n++] = '.';
			outname[n++] = 'g';
			outname[n++] = 'l';
			outname[n++] = 'm';
			outname[n] = '\0';
		}

		st = vfs_fab(outname);
		if (st != VFS_OK && st != VFS_ERR_EXISTS) {
			kfree(out);
			terminal_write("Could not create output file.\n");
			goto fail;
		}

		st = vfs_carve_bytes(outname, out, total);
		kfree(out);

		if (st != VFS_OK) {
			terminal_write("Could not write golem.\n");
			goto fail;
		}

		terminal_write("Golem shaped: ");
		terminal_write(outname);
		terminal_putc('\n');
	}

	if (code) kfree(code);
	if (data) kfree(data);
	return 1;

emit_fail:
	terminal_write("Assembler emit overflow.\n");
fail:
	if (code) kfree(code);
	if (data) kfree(data);
	return 0;
}
