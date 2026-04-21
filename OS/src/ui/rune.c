#include <stddef.h>
#include <stdint.h>
#include "ui/rune.h"
#include "fs/vfs.h"
#include "mm/heap.h"

static size_t rune_pixel_count(uint8_t w, uint8_t h) {
    return (size_t)w * (size_t)h;
}

int rune_create(rune_bitmap_t* bmp, uint8_t width, uint8_t height) {
    if (!bmp) return 0;
    if (width == 0 || height == 0) return 0;

    size_t count = rune_pixel_count(width, height);
    uint8_t* pixels = (uint8_t*)kmalloc(count);
    if (!pixels) return 0;

    for (size_t i = 0; i < count; i++) pixels[i] = RUNE_COLOR_BLACK;

    bmp->width = width;
    bmp->height = height;
    bmp->pixels = pixels;
    return 1;
}

void rune_free(rune_bitmap_t* bmp) {
    if (!bmp) return;
    if (bmp->pixels) {
        kfree(bmp->pixels);
        bmp->pixels = 0;
    }
    bmp->width = 0;
    bmp->height = 0;
}

void rune_clear(rune_bitmap_t* bmp, uint8_t color) {
    if (!bmp || !bmp->pixels) return;
    if (color >= RUNE_COLOR_COUNT) color = RUNE_COLOR_BLACK;

    size_t count = rune_pixel_count(bmp->width, bmp->height);
    for (size_t i = 0; i < count; i++) {
        bmp->pixels[i] = color;
    }
}

int rune_set_pixel(rune_bitmap_t* bmp, uint8_t x, uint8_t y, uint8_t color) {
    if (!bmp || !bmp->pixels) return 0;
    if (x >= bmp->width || y >= bmp->height) return 0;
    if (color >= RUNE_COLOR_COUNT) return 0;

    size_t idx = (size_t)y * (size_t)bmp->width + (size_t)x;
    bmp->pixels[idx] = color;
    return 1;
}

uint8_t rune_get_pixel(const rune_bitmap_t* bmp, uint8_t x, uint8_t y) {
    if (!bmp || !bmp->pixels) return RUNE_COLOR_BLACK;
    if (x >= bmp->width || y >= bmp->height) return RUNE_COLOR_BLACK;

    size_t idx = (size_t)y * (size_t)bmp->width + (size_t)x;
    uint8_t color = bmp->pixels[idx];
    if (color >= RUNE_COLOR_COUNT) return RUNE_COLOR_BLACK;
    return color;
}

int rune_load(const char* filename, rune_bitmap_t* out) {
    if (!filename || !out) return 0;

    out->width = 0;
    out->height = 0;
    out->pixels = 0;

    const uint8_t* data = 0;
    size_t size = 0;

    vfs_status_t st = vfs_insp_bytes(filename, &data, &size);
    if (st != VFS_OK || !data) return 0;

    if (size < sizeof(rune_header_t)) return 0;

    const rune_header_t* h = (const rune_header_t*)data;
    if (h->magic[0] != RUNE_MAGIC0) return 0;
    if (h->magic[1] != RUNE_MAGIC1) return 0;
    if (h->magic[2] != RUNE_MAGIC2) return 0;
    if (h->magic[3] != RUNE_MAGIC3) return 0;
    if (h->version != RUNE_VERSION) return 0;
    if (h->width == 0 || h->height == 0) return 0;

    size_t count = rune_pixel_count(h->width, h->height);
    if (size != sizeof(rune_header_t) + count) return 0;

    uint8_t* pixels = (uint8_t*)kmalloc(count);
    if (!pixels) return 0;

    for (size_t i = 0; i < count; i++) {
        uint8_t color = data[sizeof(rune_header_t) + i];
        if (color >= RUNE_COLOR_COUNT) color = RUNE_COLOR_BLACK;
        pixels[i] = color;
    }

    out->width = h->width;
    out->height = h->height;
    out->pixels = pixels;
    return 1;
}

int rune_save(const char* filename, const rune_bitmap_t* bmp) {
    if (!filename || !bmp || !bmp->pixels) return 0;
    if (bmp->width == 0 || bmp->height == 0) return 0;

    size_t count = rune_pixel_count(bmp->width, bmp->height);
    size_t total = sizeof(rune_header_t) + count;

    uint8_t* out = (uint8_t*)kmalloc(total);
    if (!out) return 0;

    rune_header_t h;
    h.magic[0] = RUNE_MAGIC0;
    h.magic[1] = RUNE_MAGIC1;
    h.magic[2] = RUNE_MAGIC2;
    h.magic[3] = RUNE_MAGIC3;
    h.version = RUNE_VERSION;
    h.width = bmp->width;
    h.height = bmp->height;
    h.reserved = 0;

    for (size_t i = 0; i < sizeof(rune_header_t); i++) {
        out[i] = ((const uint8_t*)&h)[i];
    }
    for (size_t i = 0; i < count; i++) {
        uint8_t color = bmp->pixels[i];
        if (color >= RUNE_COLOR_COUNT) color = RUNE_COLOR_BLACK;
        out[sizeof(rune_header_t) + i] = color;
    }

    vfs_status_t st = vfs_fab(filename);
    if (st != VFS_OK && st != VFS_ERR_EXISTS) {
        kfree(out);
        return 0;
    }

    st = vfs_carve_bytes(filename, out, total);
    kfree(out);

    return st == VFS_OK;
}
