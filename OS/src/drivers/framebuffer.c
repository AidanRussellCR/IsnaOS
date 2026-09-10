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
#define FB_RGB_ALIGNED_OFFSET 112

static framebuffer_t g_fb;
static int g_available;

static uint32_t load32(const volatile uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int fb_rgb_layout_valid(uint8_t rpos, uint8_t rbits, uint8_t gpos, uint8_t gbits, uint8_t bpos, uint8_t bbits) {
    return rbits == 8 && gbits == 8 && bbits == 8 && rpos <= 24 && gpos <= 24 && bpos <= 24 && rpos != gpos && rpos != bpos && gpos != bpos;
}

int framebuffer_init(uint32_t magic, uint32_t info_address) {
    g_available = 0;
    if (magic != MULTIBOOT_MAGIC) return 0;
    if (!info_address) return 0;

    const volatile uint8_t* info = (const volatile uint8_t*)(uintptr_t)info_address;
    uint32_t flags = load32(info);
    if (!(flags & MULTIBOOT_FLAG_FB)) return 0;

    uint32_t low = load32(info + FB_ADDR_OFFSET);
    uint32_t high = load32(info + FB_ADDR_OFFSET + 4);
    uint32_t pitch = load32(info + FB_PITCH_OFFSET);
    uint32_t width = load32(info + FB_WIDTH_OFFSET);
    uint32_t height = load32(info + FB_HEIGHT_OFFSET);
    uint8_t bpp = info[FB_BPP_OFFSET];
    uint8_t type = info[FB_TYPE_OFFSET];

    // For now only handles 32-bit direct-color framebuffer, fallback to VGA
    if (high) return 0;
    if (!low) return 0;
    if (type != 1) return 0;
    if (bpp != 32) return 0;
    if (width < 640 || width > 4096 || height < 400 || height > 2160)
        return 0;
    if (pitch < width * 4 || pitch > 16384 || pitch > UINT32_MAX / height)
        return 0;
    if (low > UINT32_MAX - (pitch * height))
        return 0;

    uint8_t red_pos    = info[FB_RGB_OFFSET];
    uint8_t red_bits   = info[FB_RGB_OFFSET + 1];
    uint8_t green_pos  = info[FB_RGB_OFFSET + 2];
    uint8_t green_bits = info[FB_RGB_OFFSET + 3];
    uint8_t blue_pos   = info[FB_RGB_OFFSET + 4];
    uint8_t blue_bits  = info[FB_RGB_OFFSET + 5];

    if (!fb_rgb_layout_valid(red_pos, red_bits, green_pos, green_bits, blue_pos, blue_bits)) {
        if (red_pos != 0 || red_bits != 0)
            return 0;

        const volatile uint8_t* alt = info + FB_RGB_ALIGNED_OFFSET;
        uint8_t alt_rpos = alt[0], alt_rbits = alt[1];
        uint8_t alt_gpos = alt[2], alt_gbits = alt[3];
        uint8_t alt_bpos = alt[4], alt_bbits = alt[5];
        if (!fb_rgb_layout_valid(alt_rpos, alt_rbits, alt_gpos, alt_gbits, alt_bpos, alt_bbits))
            return 0;

        red_pos = alt_rpos; red_bits = alt_rbits;
        green_pos = alt_gpos; green_bits = alt_gbits;
        blue_pos = alt_bpos; blue_bits = alt_bbits;
    }

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
