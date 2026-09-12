#include <stdint.h>
#include "ui/desktop.h"
#include "ui/gfx.h"
#include "ui/wm.h"
#include "ui/tty_render.h"
#include "drivers/framebuffer.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "kernel/janus.h"
#include "kernel/sched.h"

static void desktop_draw(void) {
    const framebuffer_t* fb = framebuffer_info();
    if (!fb) return;
    int w = (int)fb->width;
    int h = (int)fb->height;

    gfx_clip_reset();
    gfx_rect(0, 0, w, h, 0x294965);
    gfx_rect(0, 0, w, 32, 0x1A3147);
    gfx_text(18, 8, "IsnaOS  /  Desktop", 0xF0F5FB, 0x1A3147);
    gfx_text(34, 80, "Welcome to IsnaOS", 0xE2ECF8, 0x294965);
    gfx_text(34, 104, "Use Ctrl+Shift+F8 for the CLI", 0xD2DEE9, 0x294965);
    wm_draw();
    gfx_rect(0, h - 36, w, 36, 0x14283B);
    gfx_rect(12, h - 30, 98, 24, 0x416986);
    gfx_text(28, h - 26, "IsnaOS", 0xFFFFFF, 0x416986);
    gfx_text(w - 170, h - 26, "VT1  DESKTOP", 0xD1DCE7, 0x14283B);
}

void task_desktop(void) {
    wm_init();
    janus_register_current("desktop", JANUS_KIND_DESKTOP);
    janus_switch_vt(1);
    uint32_t seen_terminal = 0;
    uint32_t seen_vt = 0;
    uint32_t seen_wm = 0;

    for (;;) {
        uint32_t current_vt = janus_vt_generation();
        uint32_t current_terminal = terminal_generation();
        uint32_t current_wm = wm_generation();

        if (janus_active_vt() == 1) {
            if (janus_current_has_focus()) {
                key_event_t ev;
                (void)janus_try_get_key(&ev);
            }
            if (current_vt != seen_vt || current_terminal != seen_terminal || current_wm != seen_wm) desktop_draw();
        } else if (current_vt != seen_vt || current_terminal != seen_terminal) {
            tty_render_fullscreen();
        }

        seen_vt = current_vt;
        seen_terminal = current_terminal;
        seen_wm = current_wm;
        yield();
    }
}
