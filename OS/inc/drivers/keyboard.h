#pragma once
#include <stdint.h>

/**
 * enum key_type_t - normalized keyboard event types
 *
 * Converts raw keyboard scancodes into higher-level key events
 * used by terminal programs, editors, and Janus focus switching.
 */
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

/**
 * struct key_event_t - decoded keyboard event
 * @type: normalized key type
 * @ch: printable character when @type is KEY_CHAR
 */
typedef struct {
    key_type_t type;
    char ch;
} key_event_t;

/**
 * keyboard_try_get_key - poll keyboard controller for one decoded key event
 * @ev: output event structure
 *
 * Return: 1 if a key event was produced, 0 if no usable key is available
 */
int keyboard_try_get_key(key_event_t* ev);
