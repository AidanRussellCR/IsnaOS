#pragma once
#include <stdint.h>

/**
 * inb - read one byte from an I/O port
 * @port: hardware I/O port to read
 *
 * Return: byte value read from @port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * outb - write one byte to an I/O port
 * @port: hardware I/O port to write
 * @val: byte value to send
 */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * outw - write one 16-bit word to an I/O port
 * @port: hardware I/O port to write
 * @val: 16-bit value to send
 */
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * inw - read one 16-bit word from an I/O port
 * @port: hardware I/O port to read
 *
 * Return: 16-bit value read from @port
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
