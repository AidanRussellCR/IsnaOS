#include <stddef.h>
#include <stdint.h>
#include "kernel/glm.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/vga.h"
#include "kernel/sched.h"

// This setup is currently not intended for C programs as the loader is not developed enough, but it will be in the future

static uint8_t* g_glm_image = 0;
static size_t g_glm_image_size = 0;
static int g_glm_exit_called = 0;
static int g_glm_exit_code = 0;

static void glm_api_print(const char* s) {
	terminal_write(s ? s : "");
}

static void glm_api_yield(void) {
	yield();
}

static void glm_api_print_off(uint32_t off) {
	if (!g_glm_image) return;
	if (off >= g_glm_image_size) return;

	const char* s = (const char*)(g_glm_image + off);
	terminal_write(s);
}

static void glm_api_exit(int code) {
	g_glm_exit_called = 1;
	g_glm_exit_code = code;
}

static int glm_validate(const glm_header_t* h, size_t file_size) {
	if (!h) return 0;

	if (h->magic != GLM_MAGIC) return 0;
	if (h->version != GLM_VERSION) return 0;
	if (h->api_version != GLM_API_VERSION) return 0;

	if (h->code_offset > file_size) return 0;
	if (h->data_offset > file_size) return 0;

	if (h->code_size > file_size - h->code_offset) return 0;
	if (h->data_size > file_size - h->data_offset) return 0;

	// entry must be somewhere in loaded image
	if (h->entry_offset >= (h->code_size + h->data_size + h->bss_size)) return 0;

	return 1;
}

int glm_load_and_run(const char* filename) {
	const uint8_t* file_data = 0;
	size_t file_size = 0;

	vfs_status_t st = vfs_insp_bytes(filename, &file_data, &file_size);
	if (st != VFS_OK || !file_data) {
		terminal_write("Could not open golem.\n");
		return 0;
	}

	if (file_size < sizeof(glm_header_t)) {
		terminal_write("Bad golem file.\n");
		return 0;
	}

	const glm_header_t* h = (const glm_header_t*)file_data;

	if (!glm_validate(h, file_size)) {
		terminal_write("Invalid golem header.\n");
		return 0;
	}

	size_t image_size = (size_t)h->code_size + (size_t)h->data_size + (size_t)h->bss_size;
	if (image_size == 0) {
		terminal_write("Empty golem image.\n");
		return 0;
	}

	uint8_t* image = (uint8_t*)kmalloc(image_size);
	if (!image) {
		terminal_write("Out of memory.\n");
		return 0;
	}

	for (size_t i = 0; i < image_size; i++) image[i] = 0;

	for (size_t i = 0; i < h->code_size; i++) {
		image[i] = file_data[h->code_offset + i];
	}
	for (size_t i = 0; i < h->data_size; i++) {
		image[h->code_size + i] = file_data[h->data_offset + i];
	}

	g_glm_image = image;
	g_glm_image_size = image_size;
	g_glm_exit_called = 0;
	g_glm_exit_code = 0;

	glm_host_api_t api;
	api.api_version = GLM_API_VERSION;
	api.print = glm_api_print;
	api.yield = glm_api_yield;
	api.print_off = glm_api_print_off;
	api.exit = glm_api_exit;

	glm_entry_t entry = (glm_entry_t)(image + h->entry_offset);

	terminal_write("Summoning golem...\n");
	(void)entry(&api);

	terminal_write("\nGolem returned.\n");

	g_glm_image = 0;
	g_glm_image_size = 0;

	kfree(image);
	return 1;
}

