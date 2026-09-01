#include <stddef.h>
#include <stdint.h>
#include "kernel/glm.h"
#include "kernel/task.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "kernel/sched.h"
#include "lib/str.h"

// This setup is currently not intended for C programs as the loader is not developed enough, but it will be in the future

/*
 * glm_runtime_t - per-golem runtime
 */
typedef struct {
    uint8_t* image;
    size_t image_size;

    uint32_t entry_offset;

    uint32_t spawn_flags;

    glm_host_api_t api;

    char filename[TASK_NAME_MAX];
} glm_runtime_t;

static glm_runtime_t* glm_current_runtime(void) {
    task_t* task = task_current();

    if (!task)
        return 0;

    if (task->kind != TASK_KIND_GLM)
        return 0;

    return (glm_runtime_t*)task->userdata;
}

static void glm_runtime_destroy(void* userdata) {
    glm_runtime_t* rt = (glm_runtime_t*)userdata;

    if (!rt)
        return;

    if (rt->image) {
        kfree(rt->image);
    }

    kfree(rt);
}

static void glm_write_u32(uint32_t value) {
    char buf[16];
    int p = 0;

    if (value == 0) {
        terminal_putc('0');
        return;
    }

    while (value > 0 && p < (int)sizeof(buf)) {
        buf[p++] =
            (char)('0' + (value % 10));

        value /= 10;
    }

    while (p > 0) {
        terminal_putc(buf[--p]);
    }
}

static void glm_write_int(int value) {
    uint32_t magnitude;

    if (value < 0) {
        terminal_putc('-');

        magnitude =
            0u - (uint32_t)value;
    } else {
        magnitude =
            (uint32_t)value;
    }

    glm_write_u32(magnitude);
}

static void glm_write_hex32(uint32_t value) {
    static const char digits[] =
        "0123456789ABCDEF";

    terminal_write("0x");

    for (int shift = 28;
         shift >= 0;
         shift -= 4) {
        terminal_putc(
            digits[(value >> shift) & 0x0F]
        );
    }
}

static void glm_trace_write_string(const char* s, size_t max_bytes) {
    if (!s) {
        terminal_write("NULL");
        return;
    }

    terminal_putc('"');

    size_t i = 0;

    while (i < max_bytes && s[i]) {
        char c = s[i++];

        switch (c) {
            case '\n':
                terminal_write("\\n");
                break;

            case '\r':
                terminal_write("\\r");
                break;

            case '\t':
                terminal_write("\\t");
                break;

            case '\\':
                terminal_write("\\\\");
                break;

            case '"':
                terminal_write("\\\"");
                break;

            default:
                if ((unsigned char)c >= 32 &&
                    (unsigned char)c <= 126) {
                    terminal_putc(c);
                } else {
                    terminal_putc('?');
                }
                break;
        }
    }

    if (i == max_bytes) {
        terminal_write("...");
    }

    terminal_putc('"');
}

static void glm_trace_prefix(void) {
    terminal_write("[strace ");

    int id = task_current_id();

    if (id < 0) {
        terminal_putc('?');
    } else {
        glm_write_u32((uint32_t)id);
    }

    terminal_write("] ");
}

// GLM host API

static void glm_api_print(const char* s) {
    terminal_write(s ? s : "");
}

static void glm_api_yield(void) {
    yield();
}

static void glm_api_print_off(uint32_t off) {
    glm_runtime_t* rt = glm_current_runtime();

    if (!rt)
        return;

    if (!rt->image)
        return;

    if (off >= rt->image_size)
        return;

    const char* s =
    (const char*)(rt->image + off);

    terminal_write(s);
}

static void glm_api_exit(int code) {
    task_exit_code(code);
}

static int glm_api_getch(void) {
    for (;;) {
        key_event_t ev;
        if (!keyboard_try_get_key(&ev)) {
            yield();
            continue;
        }

        if (ev.type == KEY_CHAR) {
            terminal_putc(ev.ch);
            return (unsigned char)ev.ch;
        }
        if (ev.type == KEY_ENTER) {
            terminal_putc('\n');
            return '\n';
        }
        if (ev.type == KEY_BACKSPACE) {
            return '\b';
        }
        if (ev.type == KEY_ESC) {
            return 27;
        }
    }
}

static void glm_api_print_num(int value) {
    glm_write_int(value);
}

static void glm_trace_print(const char* s) {
    glm_trace_prefix();

    terminal_write("print(");

    glm_trace_write_string(
        s,
        64
    );

    terminal_write(")\n");

    glm_api_print(s);
}

static void glm_trace_yield(void) {
    glm_trace_prefix();
    terminal_write("yield()\n");

    glm_api_yield();
}

static void glm_trace_print_off(uint32_t off) {
    glm_trace_prefix();

    terminal_write("print_off(");
    glm_write_hex32(off);

    glm_runtime_t* rt =
        glm_current_runtime();

    if (rt &&
        rt->image &&
        off < rt->image_size) {

        terminal_write(", ");

        size_t remaining =
            rt->image_size - off;

        if (remaining > 64) {
            remaining = 64;
        }

        glm_trace_write_string(
            (const char*)(
                rt->image + off
            ),
            remaining
        );
    }

    terminal_write(")\n");

    glm_api_print_off(off);
}

static void glm_trace_exit(int code) {
    glm_trace_prefix();

    terminal_write("exit(");
    glm_write_int(code);
    terminal_write(")\n");

    glm_api_exit(code);
}

static int glm_trace_getch(void) {
    glm_trace_prefix();
    terminal_write("getch()\n");

    int result =
        glm_api_getch();

    glm_trace_prefix();

    terminal_write("getch() = ");
    glm_write_int(result);

    if (result >= 32 &&
        result <= 126) {
        terminal_write(" ('");
        terminal_putc((char)result);
        terminal_write("')");
    }

    terminal_putc('\n');

    return result;
}

static void glm_trace_print_num(int value) {
    glm_trace_prefix();

    terminal_write("print_num(");
    glm_write_int(value);
    terminal_write(")\n");

    glm_api_print_num(value);
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

static void glm_task_main(void) {
    task_t* task = task_current();

    if (!task ||
        task->kind != TASK_KIND_GLM) {
        task_exit_code(-1);
        }

        glm_runtime_t* rt =
        (glm_runtime_t*)task->userdata;

    if (!rt || !rt->image) {
        task_exit_code(-1);
    }

    glm_entry_t entry =
    (glm_entry_t)(
        rt->image + rt->entry_offset
    );

    (void)entry(
        &rt->api,
        rt->image
    );

    task_exit_code(0);
}

int glm_spawn_ex(const char* filename, uint32_t flags) {
    const uint8_t* file_data = 0;
    size_t file_size = 0;

    vfs_status_t st =
        vfs_insp_bytes(
            filename,
            &file_data,
            &file_size
        );

    if (st != VFS_OK || !file_data) {
        terminal_write("Could not open golem.\n");
        return -1;
    }

    if (file_size < sizeof(glm_header_t)) {
        terminal_write("Bad golem file.\n");
        return -1;
    }

    const glm_header_t* h =
        (const glm_header_t*)file_data;

    if (!glm_validate(h, file_size)) {
        terminal_write("Invalid golem header.\n");
        return -1;
    }

    size_t image_size =
        (size_t)h->code_size +
        (size_t)h->data_size +
        (size_t)h->bss_size;

    if (image_size == 0) {
        terminal_write("Empty golem image.\n");
        return -1;
    }

    // allocate per-process runtime
    glm_runtime_t* rt =
        (glm_runtime_t*)kmalloc(
            sizeof(glm_runtime_t)
        );

    if (!rt) {
        terminal_write("Out of memory.\n");
        return -1;
    }

    rt->image = 0;
    rt->image_size = image_size;
    rt->entry_offset = h->entry_offset;
    rt->spawn_flags = flags;

    kstrncpy0(
        rt->filename,
        filename ? filename : "golem",
        sizeof(rt->filename)
    );

    // allocate the executable image
    rt->image =
        (uint8_t*)kmalloc(image_size);

    if (!rt->image) {
        kfree(rt);

        terminal_write("Out of memory.\n");
        return -1;
    }

    for (size_t i = 0; i < image_size; i++) {
        rt->image[i] = 0;
    }

    for (size_t i = 0;
         i < h->code_size;
         i++) {
        rt->image[i] =
            file_data[h->code_offset + i];
    }

    for (size_t i = 0;
         i < h->data_size;
         i++) {
        rt->image[h->code_size + i] =
            file_data[h->data_offset + i];
    }

    // Initialize golem API table
    rt->api.api_version = GLM_API_VERSION;

    if (flags & GLM_SPAWN_TRACE_API) {
        rt->api.print       = glm_trace_print;
        rt->api.yield       = glm_trace_yield;
        rt->api.print_off   = glm_trace_print_off;
        rt->api.exit        = glm_trace_exit;
        rt->api.getch       = glm_trace_getch;
        rt->api.print_num   = glm_trace_print_num;
    } else {
        rt->api.print       = glm_api_print;
        rt->api.yield       = glm_api_yield;
        rt->api.print_off   = glm_api_print_off;
        rt->api.exit        = glm_api_exit;
        rt->api.getch       = glm_api_getch;
        rt->api.print_num   = glm_api_print_num;
    }

    // Create scheduled golem task
    int id = task_create_ex(
        glm_task_main,
        rt->filename,
        TASK_KIND_GLM,
        rt,
        glm_runtime_destroy,
        TASK_FLAG_NONE
    );

    if (id < 0) {
        glm_runtime_destroy(rt);

        terminal_write(
            "Could not create golem task.\n"
        );

        return -1;
    }

    return id;
}

int glm_spawn(const char* filename) {
    return glm_spawn_ex(filename, GLM_SPAWN_NONE);
}
