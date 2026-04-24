#pragma once
#include <stdint.h>

typedef enum {
	KEY_NONE = 0,
	KEY_CHAR,
	KEY_ESC,
	KEY_ENTER,
	KEY_BACKSPACE,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_DELETE,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	// For Janus
	KEY_ALT_F1,
	KEY_ALT_F2,
	KEY_ALT_F3,
	KEY_ALT_F4
} key_type_t;

typedef struct {
	key_type_t type;
	char ch;
} key_event_t;

int keyboard_try_get_key(key_event_t* ev);
