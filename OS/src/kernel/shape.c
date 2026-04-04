#include <stddef.h>
#include <stdint.h>
#include "kernel/shape.h"
#include "kernel/glm.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/vga.h"
#include "lib/str.h"

// Shape v2.0

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

// fix for colon in string being read as a label, causing errors
static char* find_label_colon(char* line) {
	if (!line || !line[0]) return 0;

	for (size_t i = 0; line[i]; i++) {
		if (line[i] == ':') return &line[i];
		if (is_space_ch(line[i])) return 0;
	}

	return 0;
}

static int parse_ident(const char* s, char* out, size_t cap) {
	size_t i = 0;
	if (!s || !s[0]) return 0;

	while (s[i] && !is_space_ch(s[i]) && s[i] != ':' && s[i] != ',' && i + 1 < cap) {
		out[i] = s[i];
		i++;
	}
	out[i] = '\0';
	return i > 0;
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

static int parse_imm(const char* s, int32_t* out) {
	char buf[64];
	size_t n = 0;

	s = trim_left((char*)s);
	while (*s && n + 1 < sizeof(buf)) {
		buf[n++] = *s++;
	}
	buf[n] = '\0';
	trim_right(buf);

	if (!buf[0]) return 0;

	// char literal
	if (buf[0] == '\'' && buf[1]) {
		char ch = 0;

		if (buf[1] == '\\') {
			if (buf[2] == 'n') ch = '\n';
			else if (buf[2] == 't') ch = '\t';
			else if (buf[2] == '\\') ch = '\\';
			else if (buf[2] == '\'') ch = '\'';
			else return 0;

			if (buf[3] != '\'') return 0;
		} else {
			ch = buf[1];
			if (buf[2] != '\'') return 0;
		}

		*out = (int32_t)(unsigned char)ch;
		return 1;
	}

	int neg = 0;
	size_t i = 0;
	int32_t v = 0;

	if (buf[0] == '-') {
		neg = 1;
		i++;
	}

	if (!buf[i]) return 0;

	for (; buf[i]; i++) {
		if (buf[i] < '0' || buf[i] > '9') return 0;
		v = v * 10 + (buf[i] - '0');
	}

	*out = neg ? -v : v;
	return 1;
}

static int parse_bracket_label(const char* s, char* out, size_t cap) {
	s = trim_left((char*)s);
	if (*s != '[') return 0;
	s++;

	s = trim_left((char*)s);

	size_t i = 0;
	while (*s && *s != ']' && !is_space_ch(*s) && i + 1 < cap) {
		out[i++] = *s++;
	}
	out[i] = '\0';

	s = trim_left((char*)s);
	if (*s != ']') return 0;

	return i > 0;
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

static int emit_rel32(uint8_t* buf, size_t* p, size_t cap, int32_t disp) {
	return emit_u32_le(buf, p, cap, (uint32_t)disp);
}

static int code_size_of(const char* line) {
	if (streq(line, "ret")) return 1;
	if (streq(line, "nop")) return 1;
	if (streq(line, "yield")) return 7;
	if (streq(line, "loadbase")) return 4;
	if (streq(line, "exit")) return 13;
	if (streq(line, "hear")) return 7;
	if (streq(line, "show")) return 11;

	if (starts_with(line, "speak ")) return 15;
	if (starts_with(line, "say ")) return 15;

	if (starts_with(line, "mov a, [")) return 6;
	if (starts_with(line, "mov [")) return 6;
	if (starts_with(line, "mov a,")) return 5;
	if (starts_with(line, "lea a,")) return 6;

	if (starts_with(line, "add a,")) return 5;
	if (starts_with(line, "sub a,")) return 5;
	if (starts_with(line, "mul a,")) return 6;
	if (starts_with(line, "div a,")) return 8;
	if (starts_with(line, "cmp a,")) return 5;

	if (starts_with(line, "jmp ")) return 5;
	if (starts_with(line, "jz ")) return 6;
	if (starts_with(line, "jnz ")) return 6;
	if (starts_with(line, "je ")) return 6;
	if (starts_with(line, "jne ")) return 6;

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

			if (streq(line, ".code")) { sec = SHAPE_SEC_CODE; continue; }
			if (streq(line, ".data")) { sec = SHAPE_SEC_DATA; continue; }

			char* colon = find_label_colon(line);
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
				} else if (starts_with(line, "dd ")) {
					int32_t imm;
					if (!parse_imm(line + 3, &imm)) {
						terminal_write("Bad dd immediate.\n");
						return 0;
					}
					data_size += 4;
				} else {
					terminal_write("Unknown data directive: [");
					terminal_write(line);
					terminal_write("]\n");
					return 0;
				}
			} else {
				terminal_write("No section selected.\n");
				return 0;
			}
		}
	}

	{
		shape_sec_t lsec = SHAPE_SEC_NONE;
		uint32_t loff = 0;
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

			char* colon = find_label_colon(line);
			if (colon) {
				line = trim_left(colon + 1);
				if (!line[0]) continue;
			}

			if (sec == SHAPE_SEC_CODE) {
				size_t cur_off = code_p;

				if (streq(line, "ret")) {
					if (!emit_byte(code, &code_p, code_size, 0xC3)) goto emit_fail;
				} else if (streq(line, "nop")) {
					if (!emit_byte(code, &code_p, code_size, 0x90)) goto emit_fail;
				} else if (streq(line, "yield")) {
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x54)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x52)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x08)) goto emit_fail;
				} else if (streq(line, "loadbase")) {
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x5C)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x08)) goto emit_fail;
				} else if (streq(line, "exit")) {
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x54)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x6A)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x00)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x52)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x10)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC4)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC3)) goto emit_fail;
				} else if (streq(line, "hear")) {
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x54)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x52)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x14)) goto emit_fail;
				} else if (streq(line, "show")) {
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x54)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x50)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x52)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x18)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC4)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
				} else if (starts_with(line, "speak ") || starts_with(line, "say ")) {
					const char* arg = starts_with(line, "speak ") ? line + 6 : line + 4;
					char name[32];
					shape_sec_t lsec = SHAPE_SEC_NONE;
					uint32_t loff = 0;

					if (!parse_ident(arg, name, sizeof(name))) {
						terminal_write("Bad say target.\n");
						goto fail;
					}
					if (!find_label(labels, label_count, name, &lsec, &loff) || lsec != SHAPE_SEC_DATA) {
						terminal_write("Unknown data label.\n");
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
					if (!emit_byte(code, &code_p, code_size, 0x54)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x24)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x68)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, image_off)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xFF)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x52)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x0C)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC4)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x04)) goto emit_fail;

				} else if (starts_with(line, "mov a, [")) {
					char name[32];
					shape_sec_t lsec = SHAPE_SEC_NONE;
					uint32_t loff = 0;

					if (!parse_bracket_label(line + 6, name, sizeof(name))) {
						terminal_write("Bad label in mov load.\n");
						goto fail;
					}

					if (!find_label(labels, label_count, name, &lsec, &loff) || lsec != SHAPE_SEC_DATA) {
						terminal_write("Unknown data label in mov load.\n");
						goto fail;
					}

					uint32_t image_off = code_size + loff;

					// mov eax, [ebx + imm32]
					if (!emit_byte(code, &code_p, code_size, 0x8B)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, image_off)) goto emit_fail;
				} else if (starts_with(line, "mov [")) {
					const char* comma = 0;
					for (const char* p = line; *p; p++) {
						if (*p == ',') {
							comma = p;
							break;
						}
					}

					if (!comma) {
						terminal_write("Bad mov store syntax.\n");
						goto fail;
					}

					char name[32];
					shape_sec_t lsec = SHAPE_SEC_NONE;
					uint32_t loff = 0;

					char lhs[64];
					size_t lhs_len = (size_t)(comma - (line + 4));
					if (lhs_len + 1 > sizeof(lhs)) {
						terminal_write("Bad mov store label.\n");
						goto fail;
					}

					for (size_t i = 0; i < lhs_len; i++) lhs[i] = line[4 + i];
					lhs[lhs_len] = '\0';

					if (!parse_bracket_label(lhs, name, sizeof(name))) {
						terminal_write("Bad label in mov store.\n");
						goto fail;
					}

					const char* rhs = trim_left((char*)comma + 1);
					if (!streq(rhs, "a")) {
						terminal_write("Only mov [label], a is supported.\n");
						goto fail;
					}

					if (!find_label(labels, label_count, name, &lsec, &loff) || lsec != SHAPE_SEC_DATA) {
						terminal_write("Unknown data label in mov store.\n");
						goto fail;
					}

					uint32_t image_off = code_size + loff;

					// mov [ebx + imm32], eax
					if (!emit_byte(code, &code_p, code_size, 0x89)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, image_off)) goto emit_fail;

				} else if (starts_with(line, "mov a,")) {
					int32_t imm;
					if (!parse_imm(line + 6, &imm)) {
						terminal_write("Bad immediate in mov.\n");
						goto fail;
					}
					if (!emit_byte(code, &code_p, code_size, 0xB8)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, (uint32_t)imm)) goto emit_fail;

				} else if (starts_with(line, "lea a,")) {
					const char* arg = line + 7;
					char name[32];
					shape_sec_t lsec = SHAPE_SEC_NONE;
					uint32_t loff = 0;

					if (!parse_ident(arg, name, sizeof(name))) {
						terminal_write("Bad label in lea.\n");
						goto fail;
					}

					if (!find_label(labels, label_count, name, &lsec, &loff) || lsec != SHAPE_SEC_DATA) {
						terminal_write("Unknown data label in lea.\n");
						goto fail;
					}

					uint32_t image_off = code_size + loff;

					// lea eax, [ebx + imm32]
					if (!emit_byte(code, &code_p, code_size, 0x8D)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0x83)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, image_off)) goto emit_fail;
				} else if (starts_with(line, "add a,")) {
					int32_t imm;
					if (!parse_imm(line + 6, &imm)) {
						terminal_write("Bad immediate in add.\n");
						goto fail;
					}
					if (!emit_byte(code, &code_p, code_size, 0x05)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, (uint32_t)imm)) goto emit_fail;
				} else if (starts_with(line, "sub a,")) {
					int32_t imm;
					if (!parse_imm(line + 6, &imm)) {
						terminal_write("Bad immediate in sub.\n");
						goto fail;
					}
					if (!emit_byte(code, &code_p, code_size, 0x2D)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, (uint32_t)imm)) goto emit_fail;
				} else if (starts_with(line, "mul a,")) {
					int32_t imm;
					if (!parse_imm(line + 6, &imm)) {
						terminal_write("Bad immediate in mul.\n");
						goto fail;
					}
					if (!emit_byte(code, &code_p, code_size, 0x69)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xC0)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, (uint32_t)imm)) goto emit_fail;
				} else if (starts_with(line, "div a,")) {
					int32_t imm;
					if (!parse_imm(line + 6, &imm) || imm == 0) {
						terminal_write("Bad immediate in div.\n");
						goto fail;
					}
					if (!emit_byte(code, &code_p, code_size, 0x99)) goto emit_fail; /* cdq */
					if (!emit_byte(code, &code_p, code_size, 0xB9)) goto emit_fail; /* mov ecx,imm32 */
					if (!emit_u32_le(code, &code_p, code_size, (uint32_t)imm)) goto emit_fail;
					if (!emit_byte(code, &code_p, code_size, 0xF7)) goto emit_fail; /* idiv ecx */
					if (!emit_byte(code, &code_p, code_size, 0xF9)) goto emit_fail;
				} else if (starts_with(line, "cmp a,")) {
					int32_t imm;
					if (!parse_imm(line + 6, &imm)) {
						terminal_write("Bad immediate in cmp.\n");
						goto fail;
					}
					if (!emit_byte(code, &code_p, code_size, 0x3D)) goto emit_fail;
					if (!emit_u32_le(code, &code_p, code_size, (uint32_t)imm)) goto emit_fail;
				} else if (starts_with(line, "jmp ") || starts_with(line, "jz ") ||
				           starts_with(line, "jnz ") || starts_with(line, "je ") ||
				           starts_with(line, "jne ")) {
					const char* arg;
					int instr_size = 0;
					int cond = 0; // 0=jmp, 1=jz/je, 2=jnz/jne

					if (starts_with(line, "jmp ")) { arg = line + 4; instr_size = 5; cond = 0; }
					else if (starts_with(line, "jz ")) { arg = line + 3; instr_size = 6; cond = 1; }
					else if (starts_with(line, "je ")) { arg = line + 3; instr_size = 6; cond = 1; }
					else if (starts_with(line, "jnz ")) { arg = line + 4; instr_size = 6; cond = 2; }
					else { arg = line + 4; instr_size = 6; cond = 2; }

					char name[32];
					shape_sec_t lsec = SHAPE_SEC_NONE;
					uint32_t loff = 0;

					if (!parse_ident(arg, name, sizeof(name))) {
						terminal_write("Bad jump target.\n");
						goto fail;
					}
					if (!find_label(labels, label_count, name, &lsec, &loff) || lsec != SHAPE_SEC_CODE) {
						terminal_write("Unknown code label in jump.\n");
						goto fail;
					}

					int32_t disp = (int32_t)loff - (int32_t)(cur_off + instr_size);

					if (cond == 0) {
						if (!emit_byte(code, &code_p, code_size, 0xE9)) goto emit_fail;
						if (!emit_rel32(code, &code_p, code_size, disp)) goto emit_fail;
					} else if (cond == 1) {
						if (!emit_byte(code, &code_p, code_size, 0x0F)) goto emit_fail;
						if (!emit_byte(code, &code_p, code_size, 0x84)) goto emit_fail;
						if (!emit_rel32(code, &code_p, code_size, disp)) goto emit_fail;
					} else {
						if (!emit_byte(code, &code_p, code_size, 0x0F)) goto emit_fail;
						if (!emit_byte(code, &code_p, code_size, 0x85)) goto emit_fail;
						if (!emit_rel32(code, &code_p, code_size, disp)) goto emit_fail;
					}
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
				} else if (starts_with(line, "dd ")) {
					int32_t imm;
					if (!parse_imm(line + 3, &imm)) {
						terminal_write("Bad dd immediate.\n");
						goto fail;
					}
					if (!emit_u32_le(data, &data_p, data_size, (uint32_t)imm)) goto emit_fail;
				} else {
					terminal_write("Unknown data directive: [");
					terminal_write(line);
					terminal_write("]\n");
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
			shape_sec_t esec = SHAPE_SEC_NONE;
			uint32_t eoff = 0;

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

		for (size_t i = 0; i < sizeof(glm_header_t); i++) out[i] = ((const uint8_t*)&h)[i];
		for (size_t i = 0; i < code_size; i++) out[h.code_offset + i] = code[i];
		for (size_t i = 0; i < data_size; i++) out[h.data_offset + i] = data[i];

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

