#pragma once
#include <stdint.h>

typedef enum {
    WM_WINDOW_TERMINAL = 0,
    WM_WINDOW_FILES = 1,
    WM_WINDOW_COUNT = 2
} wm_window_kind_t;

typedef struct {
    int x, y, width, height;
    int visible;
    wm_window_kind_t kind;
    const char* title;
} wm_window_t;

void wm_init(void);
void wm_draw(void);
void wm_focus(wm_window_kind_t kind);
wm_window_kind_t wm_active(void);
void wm_move_active(int dx, int dy);
void wm_resize_active(int dw, int dh);
uint32_t wm_generation(void);
const wm_window_t* wm_window(wm_window_kind_t kind);
