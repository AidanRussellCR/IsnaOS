#pragma once

#include "drivers/keyboard.h"

void task_desktop(void);
int desktop_handle_global_key(const key_event_t* ev);
