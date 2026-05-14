Kernel Main
===========

``kmain()`` is the primary kernel initialization entrypoint.

Initialization Order
--------------------

The kernel currently initializes subsystems in the following order:

1. VGA terminal
2. Heap allocator
3. Virtual filesystem
4. Filesystem image loading
5. Tasking system
6. Janus application manager
7. Core tasks
8. Overlay rendering
9. Scheduler startup

Core Tasks
----------

Current startup tasks include:

``wraith``
    Reaps and cleans terminated tasks.

``shell``
    Primary interactive terminal shell.

Scheduler Startup
-----------------

After initialization completes, interrupts are disabled and the scheduler loop begins.

The kernel then remains in a halted idle loop whenever no runnable task exists.
