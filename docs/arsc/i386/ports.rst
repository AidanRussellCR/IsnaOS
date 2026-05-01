ports
=====

``ports.h`` provides small inline wrappers around x86 I/O port instructions.

These helpers are used by hardware drivers to communicate with legacy devices
through port-mapped I/O.

Functions
---------

``inb(uint16_t port)``
    Read one byte from an I/O port.

``outb(uint16_t port, uint8_t val)``
    Write one byte to an I/O port.

``outw(uint16_t port, uint16_t val)``
    Write one 16-bit word to an I/O port.

``inw(uint16_t port)``
    Read one 16-bit word from an I/O port.
