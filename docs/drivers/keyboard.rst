Keyboard
========

The keyboard driver polls the PS/2 controller and converts raw scancodes into
higher-level ``key_event_t`` events.

It supports printable characters, shifted characters, arrows, paging keys,
delete/backspace, function keys, and Janus Alt+F shortcuts.

Types
-----

``key_type_t``
    Normalized key event type.

``key_event_t``
    Decoded keyboard event containing a key type and optional character.

Functions
---------

``keyboard_try_get_key(key_event_t* ev)``
    Poll the keyboard controller and decode one key event if available.

    Returns ``1`` when an event is produced and ``0`` when no usable key is
    available.

Notes
-----

The driver tracks Shift and Alt state internally. Extended ``0xE0`` scancodes
are used for arrow/navigation keys. Plain F1-F4 and Alt+F1-F4 are separated so
Janus can use Alt shortcuts while preserving normal function keys for apps.
