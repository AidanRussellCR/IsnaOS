#include <stddef.h>
#include <stdint.h>
#include "ui/files_ui.h"
#include "ui/gfx.h"
#include "fs/vfs.h"
#include "lib/str.h"

#define FILES_MAX_ENTRIES 20
#define FILES_NAME_MAX 40

typedef struct {
    char name[FILES_NAME_MAX];
    int directory;
} file_entry_t;

static file_entry_t entries[FILES_MAX_ENTRIES];
static int count, selected;
static uint32_t generation = 1;
static char preview[72];

static void collect_file(const char* name, int dir, void* unused) {
    (void)unused;
    if (count >= FILES_MAX_ENTRIES) return;
    kstrncpy0(entries[count].name, name, sizeof(entries[count].name));
    entries[count].directory = dir;
    count++;
}

static void refresh_files(void) {
    count = 0;
    vfs_shop(collect_file, 0);
    if (selected >= count) selected = count > 0 ? count - 1 : 0;
}

void files_ui_init(void) {
    selected = 0;
    preview[0] = '\0';
    refresh_files();
    generation++;
}

uint32_t files_ui_generation(void) { return generation; }

void files_ui_draw(int x, int y, int width, int height) {
    if (width < 160 || height < 90) return;
    refresh_files();
    char path[96];
    vfs_pwd(path, sizeof(path));
    gfx_text(x, y, "Directory:", 0xA7BFD2, 0x172636);
    int max_chars = (width - 8) / 8;

    for (int i = 0; path[i] && i < max_chars; i++)
        gfx_char(x + i * 8, y + 18, path[i], 0xF1F5F8, 0x172636);

    gfx_hline(x, y + 40, width - 6, 0x587086);
    int max_rows = (height - 100) / 20;
    if (max_rows > FILES_MAX_ENTRIES) max_rows = FILES_MAX_ENTRIES;
    if (max_rows < 0) max_rows = 0;
    int first = selected >= max_rows && max_rows > 0 ? selected - max_rows + 1 : 0;

    for (int row = 0; row < max_rows && first + row < count; row++) {
        int id = first + row;
        int cy = y + 48 + row * 20;
        int marked = id == selected;
        uint32_t bg = marked ? 0x396184 : 0x172636;
        uint32_t fg = entries[id].directory ? 0xF4D69B : 0xD8E7F1;
        gfx_rect(x, cy, width - 6, 18, bg);
        gfx_text(x + 4, cy + 1, entries[id].directory ? "[+]" : "[ ]", fg, bg);

        for (int k = 0; entries[id].name[k] && k < (width - 64) / 8; k++)
            gfx_char(x + 44 + k * 8, cy + 1, entries[id].name[k], fg, bg);
    }

    int footer_y = y + height - 32;
    gfx_rect(x, footer_y, width - 6, 28, 0x20384B);

    if (preview[0]) {
        for (int i = 0; preview[i] && i < (width - 8) / 8; i++)
            gfx_char(x + i * 8, footer_y + 4, preview[i], 0xD1DEE7, 0x20384B);
    } else {
        gfx_text(x + 3, footer_y + 4, "Enter=open  Esc=parent", 0xD1DEE7, 0x20384B);
    }
}

int files_ui_key(const key_event_t* ev) {
    if (!ev) return 0;

    if (ev->type == KEY_UP) {
        if (selected) selected--;
    } else if (ev->type == KEY_DOWN) {
        if (selected + 1 < count) selected++;
    } else if (ev->type == KEY_ESC) {
        vfs_cd("..");
        selected = 0;
        preview[0] = '\0';
    } else if (ev->type == KEY_ENTER) {
        if (count <= 0) return 1;
        const file_entry_t* file = &entries[selected];
        if (file->directory) {
            if (vfs_cd(file->name) == VFS_OK) {
                selected = 0;
                preview[0] = '\0';
            }
        } else {
            const char* text = 0;
            vfs_status_t st = vfs_insp(file->name, &text);
            if (st == VFS_OK && text) {
                size_t i = 0;
                for (; text[i] && text[i] != '\n' && i < sizeof(preview) - 1; i++)
                    preview[i] = text[i] >= 32 ? text[i] : ' ';
                preview[i] = '\0';
            } else {
                kstrncpy0(preview, "Cannot preview this file", sizeof(preview));
            }
        }
    } else return 0;

    refresh_files();
    generation++;
    return 1;
}

void files_ui_select_at(int row) {
    if (row >= 0 && row < count) {
        selected = row;
        generation++;
    }
}
