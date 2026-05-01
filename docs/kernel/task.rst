Tasks
=====

The task subsystem manages kernel tasks, their stacks, lifecycle states, and
cleanup.

Each task has a saved stack pointer, state, entry function, name, and allocated
kernel stack. Tasks are stored in a fixed-size table controlled by
``MAX_TASKS``.

Task States
-----------

``TASK_READY``
    The task can be scheduled.

``TASK_RUNNING``
    The task is currently executing.

``TASK_BLOCKED``
    Reserved for tasks that should not currently run.

``TASK_ZOMBIE``
    The task has exited and is waiting for cleanup.

``TASK_DEAD``
    Dead/unused state.

Functions
---------

``task_init()``
    Clear the task table and reset current task tracking.

``task_create(entry, name)``
    Allocate a task control block and stack, build its initial context, and
    place it in the task table.

``task_kill(id)``
    Mark a task as zombie. Shell and Wraith are protected from being killed.

``task_exit()``
    Mark the current task as zombie and yield indefinitely.

``task_current_id()``
    Return the id of the currently running task.

``task_at(id)``
    Return the task pointer for an id.

``task_wraith()``
    Background reaper that frees zombie tasks.

``task_delay(loops)``
    Busy-loop delay that periodically yields.

``task_state_char(state)``
    Convert a task state into a printable character.

``task_print_to_console()``
    Print all active tasks to the terminal.

``hb_instance_index(hb_name, my_id)``
    Return the instance index for heartbeat-style tasks.

Notes
-----

New tasks start through ``task_trampoline()``, which calls the task entry
function and then exits the task if the entry function returns.

Zombie cleanup is handled by Wraith, which frees task stacks and removes Janus
metadata during cleanup.
