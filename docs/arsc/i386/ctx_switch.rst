ctx_switch
==========

``ctx_switch`` is the low-level i386 context switch routine used by the
scheduler.

It saves the outgoing task's stack pointer into the location provided by
``old_esp`` and switches execution to ``new_esp``.
The assembly implementation saves flags and general-purpose registers,
changes ``esp``, restores the new task's saved context, and returns
into the resumed task.

Functions
---------

``ctx_switch(uint32_t* old_esp, uint32_t new_esp)``
    Switch from the current task context to another saved task context.

Notes
-----

This function is architecture-specific and must match the stack layout created
by the task system.
