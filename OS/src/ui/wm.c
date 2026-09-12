#include <stdint.h>
#include "ui/wm.h"
#include "ui/gfx.h"
#include "ui/tty_render.h"
#include "drivers/framebuffer.h"

#define WM_HEADER_HEIGHT 24
#define WM_BORDER 3

static wm_window_t windows[WM_WINDOW_COUNT];
static wm_window_kind_t active = WM_WINDOW_TERMINAL;
static uint32_t generation = 1;

void wm_init(void) {
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    int w = (int)fb->width;
    int h = (int)fb->height;

    windows[WM_WINDOW_TERMINAL] = (wm_window_t){
        w / 14, h / 8, w > 740 ? 620 : w - 90, h > 540 ? 358 : h - 130,
        1, WM_WINDOW_TERMINAL, "Terminal"
    };

    windows[WM_WINDOW_FILES] = (wm_window_t){
        w / 2 - 50, h / 3, 340, 256, 1, WM_WINDOW_FILES, "Files"
    };

    active = WM_WINDOW_TERMINAL;
    generation++;
}

uint32_t wm_generation(void) { return generation; }

wm_window_kind_t wm_active(void) { return active; }

const wm_window_t* wm_window(wm_window_kind_t kind) {
    return kind >= 0 && kind < WM_WINDOW_COUNT ? &windows[kind] : 0;
}

void wm_focus(wm_window_kind_t kind) {
    if (kind < 0 || kind >= WM_WINDOW_COUNT) return;
    windows[kind].visible = 1;
    active = kind;
    generation++;
}

void wm_move_active(int dx, int dy) {
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    wm_window_t* win = &windows[active];
    win->x += dx;
    win->y += dy;
    if (win->x < 0) win->x = 0;
    if (win->y < 32) win->y = 32;
    if (win->x + 48 > (int)fb->width) win->x = (int)fb->width - 48;
    if (win->y + 32 > (int)fb->height - 36) win->y = (int)fb->height - 68;
    generation++;
}

void wm_resize_active(int dw, int dh) {
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    wm_window_t* win = &windows[active];
    int maxw = (int)fb->width - win->x;
    int maxh = (int)fb->height - win->y - 36;
    int nextw = win->width + dw;
    int nexth = win->height + dh;
    if (nextw < 240) nextw = 240;
    if (nexth < 120) nexth = 120;
    if (nextw > maxw) nextw = maxw;
    if (nexth > maxh) nexth = maxh;
    win->width = nextw;
    win->height = nexth;
    generation++;
}

static void wm_draw_window(const wm_window_t* win, int focused) {
    int x = win->x, y = win->y, w = win->width, h = win->height;
    if (!win->visible) return;
    gfx_clip_reset();
    gfx_rect(x, y, w, h, 0xAEC2D1);
    gfx_rect(x + 2, y + 2, w - 4, WM_HEADER_HEIGHT, focused ? 0x3D7398 : 0x4A6178);
    gfx_text(x + 12, y + 5, win->title, 0xF4F7FC, focused ? 0x3D7398 : 0x4A6178);
    gfx_rect(x + 2, y + WM_HEADER_HEIGHT + 2, w - 4, h - WM_HEADER_HEIGHT - 4, 0x172636);
    gfx_clip_set(x + WM_BORDER, y + WM_HEADER_HEIGHT + 3, w - WM_BORDER * 2, h - WM_HEADER_HEIGHT - 6);

    if (win->kind == WM_WINDOW_TERMINAL) {
        tty_render_region(x + 9, y + WM_HEADER_HEIGHT + 8, w - 18, h - WM_HEADER_HEIGHT - 16);
    } else {
        gfx_text(x + 12, y + WM_HEADER_HEIGHT + 16, "Current directory", 0xC8D9E9, 0x172636);
        gfx_text(x + 12, y + WM_HEADER_HEIGHT + 48, "File browser loading...", 0x9DB7CE, 0x172636);
    }

    gfx_clip_reset();
}

void wm_draw(void) {
    wm_window_kind_t back = active == WM_WINDOW_TERMINAL ? WM_WINDOW_FILES : WM_WINDOW_TERMINAL;
    wm_draw_window(&windows[back], 0);
    wm_draw_window(&windows[active], 1);
}
