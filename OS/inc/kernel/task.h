#pragma once
#include <stdint.h>

// Cap right now for tracked tasks
#define MAX_TASKS 64
#define TASK_NAME_MAX 48

#define TASK_FLAG_NONE 0u
#define TASK_FLAG_AUTOREAP (1u << 0)

#define TASK_EXIT_KILLED (-1)

/*
 * enum task_state_t - lifecycle states for kernel tasks
 * @TASK_DEAD: unused or dead task
 * @TASK_READY: task can be scheduled
 * @TASK_RUNNING: task is currently executing
 * @TASK_BLOCKED: task exists but should not run currently
 * @TASK_ZOMBIE: task has exited and is waiting for Wraith cleanup
 */
typedef enum {
    TASK_DEAD = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

/*
 * enum task_kind_t - identifier for task types
 * @TASK_KIND_KERNEL: kernel task
 * @TASK_KIND_GLM: golem task
 */
typedef enum {
    TASK_KIND_KERNEL = 0,
    TASK_KIND_GLM
} task_kind_t;

typedef void (*task_cleanup_fn)(void* userdata);

/*
 * struct task_t - kernel task control block
 * @esp: saved stack pointer for context switching
 * @state: current task lifecycle state
 * @id: process ID
 * @name: task name (human-readable)
 * @entry: task entry function
 * @kstack_base: allocated kernel stack base
 * @kstack_size: size of allocated kernel stack
 * @parent_id: process ID of parent process
 * @exit_code: exit status of task
 * @userdata: data for user glm processes
 * @cleanup: dead process cleanup
 */
typedef struct task {
    uint32_t esp;
    task_state_t state;

    int id;
    task_kind_t kind;

    char name[TASK_NAME_MAX];

    void (*entry)(void);

    void* kstack_base;
    uint32_t kstack_size;

    int parent_id;
    int exit_code;

    uint32_t flags;

    void* userdata;
    task_cleanup_fn cleanup;
} task_t;

/*
 * task_init - initialize the task table
 */
void task_init(void);

/*
 * task_create - create a new kernel task
 * @entry: function the task should execute
 * @name: display/debug name for the task
 *
 * Return: task id on success, -1 on failure
 */
int task_create(void (*entry)(void), const char* name);

/*
 * task_create_ex - create a new kernel task
 * @entry: function the task should execute
 * @name: display/debug name for the task
 * @kind: kernel or golem task
 * @userdata: data for glm process
 *
 * Return: task id on success, -1 on failure
 */
int task_create_ex(void (*entry)(void), const char* name, task_kind_t kind, void* userdata, task_cleanup_fn cleanup, uint32_t flags);

/*
 * task_kill - mark a task for cleanup
 * @id: task id to kill
 *
 * Return: 1 if the task was a marked zombie, 0 on failure
 */
int task_kill(int id);

/*
 * task_current_id - get the currently running task id
 *
 * Return: current task id, -1 if no task is active
 */
int task_current_id(void);

/*
 * task_current - get the currently running task id
 *
 * Return: current task id, 0 if no task is active
 */
task_t* task_current(void);

/*
 * task_at - look up a task by id
 * @id: task id
 *
 * Return: task pointer, NULL if invalid/not present
 */
task_t* task_at(int id);

/*
 * task_wraith - background task reaper
 *
 * Cleans up zombie tasks and frees their stacks/control blocks.
 */
void task_wraith(void);

/*
 * task_exit_code - terminate current task with explicit status
 *
 * Marks the current task as a zombie and yields indefinitely until reaped.
 */
void task_exit_code(int code) __attribute__((noreturn));

/*
 * task_exit - terminate the current task
 *
 * Marks the current task as a zombie and yields indefinitely until reaped.
 */
void task_exit(void) __attribute__((noreturn));

/*
 * task_wait - wait for a child task to exit
 *
 * Returns 1 on success
 * Returns 0 if the task does not exist or is not the child
 */
int task_wait(int id, int* out_exit_code);

/*
 * task_delay - cooperative busy delay
 * @loops: rough loop count
 *
 * Spins for a while and periodically yields to avoid starving tasks entirely.
 */
void task_delay(volatile uint32_t loops);

/*
 * task_state_char - convert a task state to a display character
 * @s: task state
 *
 * Return: printable state character
 */
char task_state_char(task_state_t s);

/*
 * task_print_to_console - print task table to terminal
 */
void task_print_to_console(void);

/*
 * hb_instance_index - get heartbeat index by task name
 * @hb_name: heartbeat task name
 * @my_id: task id to locate
 *
 * Return: instance index, -1 if not found
 */
int hb_instance_index(const char* hb_name, int my_id);
