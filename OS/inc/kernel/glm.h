#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * GLM_MAGIC - identifier for GLM executable files
 */
#define GLM_MAGIC 0x304D4C47u // "GLM0"
#define GLM_VERSION 1u
#define GLM_API_VERSION 1u

#define GLM_FLAG_NONE 0u

/**
 * struct glm_header_t - GLM executable image header
 * @magic: GLM file magic
 * @version: GLM file format version
 * @flags: executable flags
 * @entry_offset: executable entry offset from image base
 * @code_offset: file offset of code section
 * @code_size: size of code section
 * @data_offset: file offset of data section
 * @data_size: size of data section
 * @bss_size: size of zero-initialized memory
 * @api_version: required host API version
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;

    uint32_t entry_offset; // offset from loaded base

    uint32_t code_offset;
    uint32_t code_size;

    uint32_t data_offset;
    uint32_t data_size;

    uint32_t bss_size;

    uint32_t api_version; // host API version
} glm_header_t;

/**
 * struct glm_host_api_t - host functions exposed to golems
 *
 * Golems receive this structure at startup and may call
 * into kernel-provided functions through it.
 */
typedef struct {
    uint32_t api_version;

    void (*print)(const char* s);    // 0x04
    void (*yield)(void);             // 0x08
    void (*print_off)(uint32_t off); // 0x0C
    void (*exit)(int code);          // 0x10
    int  (*getch)(void);             // 0x14
    void (*print_num)(int value);    // 0x18
} glm_host_api_t;

/**
 * glm_entry_t - golem executable entry function type
 */
typedef int (*glm_entry_t)(const glm_host_api_t* api, uint8_t* image_base);

/**
 * glm_load_and_run - load a GLM binary and create a scheduled task
 * @filename: golem executable filename
 *
 * Return: task id on success, -1 on failure
 */
int glm_spawn(const char* filename);
