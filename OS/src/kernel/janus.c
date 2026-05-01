#include <stddef.h>
#include <stdint.h>
#include "kernel/janus.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/scribe.h"
#include "kernel/sigildraw.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "lib/str.h"
#include "ui/overlays.h"

typedef struct {
    int active;
    int task_id;
    janus_kind_t kind;
    char name[32];
    char arg[32];
} janus_slot_t;

static janus_slot_t g_janus[MAX_TASKS];
static int g_focused_task = -1;

static uint32_t g_focus_generation = 1;
static uint32_t g_seen_focus_generation[MAX_TASKS];

static void janus_copy(char* dst, size_t cap, const char* src) {
    kstrncpy0(dst, src ? src : "", cap);
}

void janus_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        g_janus[i].active = 0;
        g_janus[i].task_id = i;
        g_janus[i].kind = JANUS_KIND_NONE;
        g_janus[i].name[0] = '\0';
        g_janus[i].arg[0] = '\0';
        g_seen_focus_generation[i] = 0;
    }
    g_focused_task = -1;
    g_focus_generation = 1;
}

void janus_register_current(const char* name, janus_kind_t kind) {
    int id = task_current_id();
    if (id < 0 || id >= MAX_TASKS) return;

    g_janus[id].active = 1;
    g_janus[id].task_id = id;
    g_janus[id].kind = kind;
    janus_copy(g_janus[id].name, sizeof(g_janus[id].name), name ? name : "task");

    if (g_focused_task < 0 && kind != JANUS_KIND_SYSTEM) {
        g_focused_task = id;
        janus_draw_tab_bar();
    }
}

void janus_forget_task(int task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) return;

    g_janus[task_id].active = 0;
    g_janus[task_id].kind = JANUS_KIND_NONE;
    g_janus[task_id].name[0] = '\0';
    g_janus[task_id].arg[0] = '\0';

    if (g_focused_task == task_id) {
        g_focused_task = -1;
        janus_focus_next();
        janus_draw_tab_bar();
    }
}

static void janus_mark_focus_changed(void) {
    g_focus_generation++;
    janus_draw_tab_bar();
}

void janus_focus_task(int task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) return;
    if (!g_janus[task_id].active) return;
    if (g_janus[task_id].kind == JANUS_KIND_SYSTEM) return;

    if (g_focused_task != task_id) {
        g_focused_task = task_id;
        g_focus_generation++;
        janus_draw_tab_bar();
    }
}

int janus_focused_task(void) {
    return g_focused_task;
}

void janus_focus_next(void) {
    int start = g_focused_task;
    if (start < 0) start = task_current_id();
    if (start < 0) start = 0;

    for (int step = 1; step <= MAX_TASKS; step++) {
        int idx = (start + step) % MAX_TASKS;
        if (!g_janus[idx].active) continue;
        if (g_janus[idx].kind == JANUS_KIND_SYSTEM) continue;
        if (!task_at(idx)) continue;

        if (g_focused_task != idx) {
            g_focused_task = idx;
            g_focus_generation++;
            janus_draw_tab_bar();
        }
        return;
    }
}

int janus_try_get_key(key_event_t* ev) {
    if (!ev) return 0;

    if (!janus_current_has_focus()) {
        return 0;
    }

    if (!keyboard_try_get_key(ev)) {
        return 0;
    }

    if (ev->type == KEY_ALT_F1) {
        janus_focus_next();
        return 0;
    }

    return 1;
}

int janus_current_has_focus(void) {
    int id = task_current_id();
    return id >= 0 && id == g_focused_task;
}

int janus_consume_focus_event(void) {
    int id = task_current_id();
    if (id < 0 || id >= MAX_TASKS) return 0;
    if (id != g_focused_task) return 0;

    if (g_seen_focus_generation[id] != g_focus_generation) {
        g_seen_focus_generation[id] = g_focus_generation;
        return 1;
    }

    return 0;
}

static void janus_task_scribe(void) {
    int id = task_current_id();
    const char* filename = "untitled.txt";

    if (id >= 0 && id < MAX_TASKS && g_janus[id].arg[0]) {
        filename = g_janus[id].arg;
    }

    terminal_clear_text_area();
    scribe_open(filename);
    janus_close_current();
    task_exit();
}

static void janus_task_sigildraw(void) {
    int id = task_current_id();
    const char* filename = "icon.rune";

    if (id >= 0 && id < MAX_TASKS && g_janus[id].arg[0]) {
        filename = g_janus[id].arg;
    }

    terminal_clear_text_area();
    sigildraw_open(filename);
    janus_close_current();
    task_exit();
}

int janus_spawn_scribe(const char* filename) {
    int id = task_create(janus_task_scribe, "scribe");
    if (id < 0) return -1;

    g_janus[id].active = 1;
    g_janus[id].task_id = id;
    g_janus[id].kind = JANUS_KIND_APP;
    janus_copy(g_janus[id].name, sizeof(g_janus[id].name), "scribe");
    janus_copy(g_janus[id].arg, sizeof(g_janus[id].arg), filename ? filename : "untitled.txt");

    janus_focus_task(id);
    janus_mark_focus_changed();
    janus_draw_tab_bar();

    return id;
}

int janus_spawn_sigildraw(const char* filename) {
    int id = task_create(janus_task_sigildraw, "sigildraw");
    if (id < 0) return -1;

    g_janus[id].active = 1;
    g_janus[id].task_id = id;
    g_janus[id].kind = JANUS_KIND_APP;
    janus_copy(g_janus[id].name, sizeof(g_janus[id].name), "sigildraw");
    janus_copy(g_janus[id].arg, sizeof(g_janus[id].arg), filename ? filename : "icon.rune");

    janus_focus_task(id);
    janus_mark_focus_changed();
    janus_draw_tab_bar();

    return id;
}

void janus_close_current(void) {
    int id = task_current_id();
    if (id < 0 || id >= MAX_TASKS) return;

    janus_forget_task(id);
}

static void print_u32(uint32_t v) {
    char tmp[12];
    int p = 0;

    if (v == 0) {
        tmp[p++] = '0';
    } else {
        char rev[12];
        int rp = 0;
        while (v > 0 && rp < 11) {
            rev[rp++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (rp > 0) tmp[p++] = rev[--rp];
    }
    tmp[p] = '\0';
    terminal_write(tmp);
}

static void pad_spaces(int count) {
    for (int i = 0; i < count; i++) terminal_putc(' ');
}

static void print_padded(const char* s, int width) {
    int len = 0;
    while (s && s[len]) {
        terminal_putc(s[len]);
        len++;
    }
    while (len < width) {
        terminal_putc(' ');
        len++;
    }
}

void janus_print_tasks(void) {
    terminal_write("JANUS TASK MANAGER\n");
    terminal_write("ID  F  TYPE    NAME         ARG\n");
    terminal_write("----------------------------------------\n");

    for (int i = 0; i < MAX_TASKS; i++) {
        if (!g_janus[i].active) continue;
        if (!task_at(i)) continue;

        // ID
        print_u32((uint32_t)i);
        pad_spaces(3);

        // Focus marker
        terminal_putc(i == g_focused_task ? '*' : ' ');
        pad_spaces(3);

        // Type
        const char* type = "?";
        if (g_janus[i].kind == JANUS_KIND_SHELL) type = "shell";
        else if (g_janus[i].kind == JANUS_KIND_APP) type = "app";
        else if (g_janus[i].kind == JANUS_KIND_SYSTEM) type = "system";

        print_padded(type, 8);

        // Name
        print_padded(g_janus[i].name, 12);

        // Arg
        if (g_janus[i].arg[0]) {
            terminal_write(g_janus[i].arg);
        }

        terminal_putc('\n');
    }
}

static void janus_write_at(size_t row, size_t* col, const char* s) {
    while (s && *s && *col < VGA_WIDTH) {
        terminal_putc_at(row, (*col)++, *s++);
    }
}

static void janus_clear_overlay_row(size_t row) {
    for (size_t c = 0; c < VGA_WIDTH; c++) {
        terminal_putentry_at(row, c, ' ', 0x07);
    }
}

static const char* janus_kind_label(janus_kind_t kind) {
    switch (kind) {
        case JANUS_KIND_SHELL: return "shell";
        case JANUS_KIND_APP: return "app";
        case JANUS_KIND_SYSTEM: return "sys";
        default: return "?";
    }
}

void janus_draw_tab_bar(void) {
    size_t row = OVERLAY_ROW0;
    size_t col = 0;

    janus_clear_overlay_row(OVERLAY_ROW0);
    janus_clear_overlay_row(OVERLAY_ROW1);

    janus_write_at(row, &col, "Janus ");

    for (int i = 0; i < MAX_TASKS; i++) {
        if (!g_janus[i].active) continue;
        if (!task_at(i)) continue;
        if (g_janus[i].kind == JANUS_KIND_SYSTEM) continue;

        if (col + 4 >= VGA_WIDTH) break;

        uint8_t color = (i == g_focused_task) ? 0x1F : 0x70;

        terminal_putentry_at(row, col++, '[', color);

        if (i == g_focused_task && col < VGA_WIDTH) {
            terminal_putentry_at(row, col++, '*', color);
        }

        const char* name = g_janus[i].name[0] ? g_janus[i].name : janus_kind_label(g_janus[i].kind);

        for (size_t n = 0; name[n] && col < VGA_WIDTH; n++) {
            terminal_putentry_at(row, col++, name[n], color);
        }

        if (g_janus[i].arg[0]) {
            if (col < VGA_WIDTH) terminal_putentry_at(row, col++, ':', color);

            for (size_t n = 0; g_janus[i].arg[n] && col < VGA_WIDTH; n++) {
                if (n >= 12) break;
                terminal_putentry_at(row, col++, g_janus[i].arg[n], color);
            }
        }

        if (col < VGA_WIDTH) terminal_putentry_at(row, col++, ']', color);
        if (col < VGA_WIDTH) terminal_putentry_at(row, col++, ' ', 0x07);
    }

    row = OVERLAY_ROW1;
    col = 0;
    janus_write_at(row, &col, "Alt+F1 next  |  janus = list  |  janus focus <id>");
}
