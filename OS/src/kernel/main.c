#include <stdint.h>
#include "drivers/vga.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/shell.h"
#include "ui/overlays.h"
#include "mm/heap.h"
#include "fs/vfs.h"

void kmain(void) {
	terminal_init();
	terminal_write("--- IsnaOS ---\n");
	terminal_write("Welcome to the land of Myrkthrima!\n");
	terminal_write("use 'help' for a list of commands.\n--------------\n\n");

	vga_cursor_hide();
	vga_cursor_enable();
	vga_cursor_set_pos(terminal_get_row(), terminal_get_col());

	terminal_write("Kernel starting tasks...\n");

	heap_init();
	vfs_init();

	vfs_status_t st = vfs_load();
	if (st == VFS_ERR_NOT_FOUND) {
		// no fs present, make fresh one
		vfs_save();
	} else if (st != VFS_OK) {
		terminal_write("Filesystem mount failed.\n");
		terminal_write("Run formatfs to create a new filesystem.\n");
	}

	task_init();

	task_create(task_wraith, "wraith");
	task_create(task_shell, "shell");
	//task_create(task_heartbeat0, "heartbeat0");
	//task_create(task_heartbeat1, "heartbeat1");

	__asm__ volatile("cli");
	schedule();

	for (;;) {
		__asm__ volatile ("hlt");
	}
}

