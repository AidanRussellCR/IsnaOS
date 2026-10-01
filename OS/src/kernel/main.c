#include <stdint.h>
#include "arsc/i386/gdt.h"
#include "arsc/i386/trap.h"
#include "drivers/vga.h"
#include "drivers/framebuffer.h"
#include "drivers/mouse.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/janus.h"
#include "kernel/shell.h"
#include "ui/overlays.h"
#include "ui/desktop.h"
#include "mm/heap.h"
#include "fs/vfs.h"

void kmain(uint32_t multiboot_magic, uint32_t multiboot_info) {
    framebuffer_init(multiboot_magic, multiboot_info);
    gdt_init();

    terminal_init();
    trap_init();

    vga_cursor_hide();
    vga_cursor_enable();
    vga_cursor_set_pos(terminal_get_row(), terminal_get_col());

    terminal_write("Kernel starting tasks...\n");

    if (framebuffer_available()) {
        const framebuffer_t* fb = framebuffer_info();
        mouse_init((int)fb->width, (int)fb->height);
    }

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
    janus_init();

    task_create(task_wraith, "wraith");
    task_create(task_shell, "shell");
    if (framebuffer_available()) task_create(task_desktop, "desktop");
    //task_create(task_heartbeat0, "heartbeat0");
    //task_create(task_heartbeat1, "heartbeat1");

    overlays_redraw();

    __asm__ volatile("cli");
    schedule();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
