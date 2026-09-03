#include <stddef.h>
#include <stdint.h>
#include "drivers/framebuffer.h"

#define MULTIBOOT_MAGIC 0x2BADB002u
#define MULTIBOOT_FLAG_FB (1u << 12)

// Multiboot information framebuffer fields start at offset 88
#define FB_ADDR_OFFSET   88
#define FB_PITCH_OFFSET  96
#define FB_WIDTH_OFFSET  100
#define FB_HEIGHT_OFFSET 104
#define FB_BPP_OFFSET    108
#define FB_TYPE_OFFSET   109
#define FB_RGB_OFFSET    110

static framebuffer_t g_fb;
static int g_available;

static uint32_t load32(const volatile uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int framebuffer_init(uint32_t magic, uint32_t info_address) {
    g_available = 0;
    if (magic != MULTIBOOT_MAGIC || !info_address) return 0;

    const volatile uint8_t* info = (const volatile uint8_t*)(uintptr_t)info_address;
    if (!(load32(info) & MULTIBOOT_FLAG_FB)) return 0;

    uint32_t low = load32(info + FB_ADDR_OFFSET);
    uint32_t high = load32(info + FB_ADDR_OFFSET + 4);
    uint32_t pitch = load32(info + FB_PITCH_OFFSET);
    uint32_t width = load32(info + FB_WIDTH_OFFSET);
    uint32_t height = load32(info + FB_HEIGHT_OFFSET);

    // For now only handles 32-bit direct-color framebuffer, fallback to VGA
    if (high || !low || info[FB_BPP_OFFSET] != 32 || info[FB_TYPE_OFFSET] != 1)
        return 0;
    if (width < 640 || width > 4096 || height < 400 || height > 2160)
        return 0;
    if (pitch < width * 4 || pitch > 16384 || pitch > UINT32_MAX / height)
        return 0;
    if (low > UINT32_MAX - (pitch * height)) return 0;

    uint8_t red_pos    = info[FB_RGB_OFFSET];
    uint8_t red_bits   = info[FB_RGB_OFFSET + 1];
    uint8_t green_pos  = info[FB_RGB_OFFSET + 2];
    uint8_t green_bits = info[FB_RGB_OFFSET + 3];
    uint8_t blue_pos   = info[FB_RGB_OFFSET + 4];
    uint8_t blue_bits  = info[FB_RGB_OFFSET + 5];
    if (red_bits != 8 || green_bits != 8 || blue_bits != 8 || red_pos > 24 || green_pos > 24 || blue_pos > 24 || red_pos == green_pos || red_pos == blue_pos || green_pos == blue_pos)
        return 0;

    g_fb.address = (volatile uint8_t*)(uintptr_t)low;
    g_fb.width = width;
    g_fb.height = height;
    g_fb.pitch = pitch;
    g_fb.red_pos = red_pos;
    g_fb.red_bits = red_bits;
    g_fb.green_pos = green_pos;
    g_fb.green_bits = green_bits;
    g_fb.blue_pos = blue_pos;
    g_fb.blue_bits = blue_bits;
    g_available = 1;
    return 1;
}

int framebuffer_available(void) { return g_available; }
const framebuffer_t* framebuffer_info(void) { return g_available ? &g_fb : 0; }

uint32_t framebuffer_pack_rgb(uint32_t rgb) {
    return (((rgb >> 16) & 255u) << g_fb.red_pos) | (((rgb >> 8) & 255u) << g_fb.green_pos) | ((rgb & 255u) << g_fb.blue_pos);
}

void framebuffer_pixel(int x, int y, uint32_t rgb) {
    if (!g_available || x < 0 || y < 0 || (uint32_t)x >= g_fb.width || (uint32_t)y >= g_fb.height) return;
    uint32_t* dest = (uint32_t*)(uintptr_t)(g_fb.address + ((uint32_t)y * g_fb.pitch) + ((uint32_t)x * 4u));
    *dest = framebuffer_pack_rgb(rgb);
}
