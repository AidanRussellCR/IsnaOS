#pragma once
#include <stdint.h>

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
 * struct task_t - kernel task control block
 * @esp: saved stack pointer for context switching
 * @state: current task lifecycle state
 * @name: task name (human-readable)
 * @entry: task entry function
 * @kstack_base: allocated kernel stack base
 * @kstack_size: size of allocated kernel stack
 */
typedef struct task {
    uint32_t esp;
    task_state_t state;
    const char* name;
    void (*entry)(void);
    void* kstack_base;
    uint32_t kstack_size;
} task_t;

// Cap right now for tracked tasks
#define MAX_TASKS 64

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
 * task_exit - terminate the current task
 *
 * Marks the current task as a zombie and yields indefinitely until reaped.
 */
void task_exit(void) __attribute__((noreturn));

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
