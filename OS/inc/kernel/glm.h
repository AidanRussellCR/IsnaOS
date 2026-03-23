#pragma once
#include <stdint.h>
#include <stddef.h>

#define GLM_MAGIC 0x304D4C47u // "GLM0"
#define GLM_VERSION 1u
#define GLM_API_VERSION 1u

#define GLM_FLAG_NONE 0u

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

// Host functions golems can utilize
typedef struct {
	uint32_t api_version;

	void (*print)(const char* s);    // 0x04
	void (*yield)(void);             // 0x08
	void (*print_off)(uint32_t off); // 0x0C
	void (*exit)(int code);          // 0x10
	int  (*getch)(void);             // 0x14
	void (*print_num)(int value);    // 0x18
} glm_host_api_t;

typedef int (*glm_entry_t)(const glm_host_api_t* api);

int glm_load_and_run(const char* filename);
