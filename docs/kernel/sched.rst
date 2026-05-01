Scheduler
=========

The scheduler currently uses a simple cooperative round-robin strategy.

Tasks voluntarily call ``yield()``, which invokes ``schedule()``. The scheduler
marks the current running task as ready, searches forward through the task
table for the next ready task, and switches to it using ``ctx_switch``.

Functions
---------

``schedule()``
    Select and switch to the next ready task.

``yield()``
    Cooperative scheduling point used by tasks that are waiting or delaying.

Notes
-----

The scheduler depends on the task subsystem for task lookup and current task
tracking. Actual CPU context switching is handled by the i386 ``ctx_switch``
routine.
