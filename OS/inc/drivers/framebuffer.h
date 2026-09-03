#pragma once
#include <stdint.h>

/*
 * Multiboot supplies linear, physical framebuffer
 * The physical address is also its 32-bit linear address, so no paging
 */
typedef struct {
    volatile uint8_t* address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t red_pos, red_bits;
    uint8_t green_pos, green_bits;
    uint8_t blue_pos, blue_bits;
} framebuffer_t;

int framebuffer_init(uint32_t multiboot_magic, uint32_t info_address);
int framebuffer_available(void);
const framebuffer_t* framebuffer_info(void);
void framebuffer_pixel(int x, int y, uint32_t rgb);
uint32_t framebuffer_pack_rgb(uint32_t rgb);
