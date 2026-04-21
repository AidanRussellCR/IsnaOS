#ifndef UI_RUNE_H
#define UI_RUNE_H

#include <stddef.h>
#include <stdint.h>

#define RUNE_MAGIC0 'R'
#define RUNE_MAGIC1 'U'
#define RUNE_MAGIC2 'N'
#define RUNE_MAGIC3 'E'
#define RUNE_VERSION 1

#define RUNE_COLOR_BLACK 0
#define RUNE_COLOR_WHITE 1
#define RUNE_COLOR_RED   2
#define RUNE_COLOR_GREEN 3
#define RUNE_COLOR_BLUE  4
#define RUNE_COLOR_COUNT 5

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t* pixels; // width * height bytes
} rune_bitmap_t;

typedef struct __attribute__((packed)) {
    char magic[4];
    uint8_t version;
    uint8_t width;
    uint8_t height;
    uint8_t reserved;
} rune_header_t;

int rune_create(rune_bitmap_t* bmp, uint8_t width, uint8_t height);
void rune_free(rune_bitmap_t* bmp);
void rune_clear(rune_bitmap_t* bmp, uint8_t color);
int rune_set_pixel(rune_bitmap_t* bmp, uint8_t x, uint8_t y, uint8_t color);
uint8_t rune_get_pixel(const rune_bitmap_t* bmp, uint8_t x, uint8_t y);

int rune_load(const char* filename, rune_bitmap_t* out);
int rune_save(const char* filename, const rune_bitmap_t* bmp);

#endif
