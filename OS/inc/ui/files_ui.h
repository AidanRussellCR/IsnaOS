#pragma once

#include <stdint.h>
#include "drivers/keyboard.h"

/* Read-only browser; navigation uses the VFS's current global CWD */
void files_ui_init(void);
void files_ui_draw(int x, int y, int width, int height);
int files_ui_key(const key_event_t* ev);
uint32_t files_ui_generation(void);
void files_ui_select_at(int row);
