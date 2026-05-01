#pragma once

#include "drivers/keyboard.h"

typedef enum {
    JANUS_KIND_NONE = 0,
    JANUS_KIND_SHELL,
    JANUS_KIND_APP,
    JANUS_KIND_SYSTEM
} janus_kind_t;

void janus_init(void);

void janus_register_current(const char* name, janus_kind_t kind);
void janus_forget_task(int task_id);

int janus_spawn_scribe(const char* filename);
int janus_spawn_sigildraw(const char* filename);

int janus_try_get_key(key_event_t* ev);

void janus_focus_task(int task_id);
void janus_focus_next(void);
int janus_focused_task(void);

int janus_current_has_focus(void);
int janus_consume_focus_event(void);

void janus_print_tasks(void);
void janus_close_current(void);

void janus_draw_tab_bar(void);
