Shell
=====

The IsnaOS shell provides the primary interactive interface for the system.

It supports command execution, filesystem management, Shape assembly, Golem execution, Janus integration, and spell scripting.

Features
--------

- Interactive command line
- Command history
- Cursor movement/editing
- Scrollback integration
- Script execution
- Janus focus switching
- Shape assembler support
- Golem loader integration
- Filesystem commands

Main Task
---------

``task_shell()``
    Main shell scheduler task.

Input System
------------

The shell uses a dynamically-sized line editor supporting:

- left/right movement
- history navigation
- delete/backspace
- scrollback paging
- inline editing

Scripts
-------

Learned ``.ms`` files may be executed using:

::

    cast <spell.ms>

Scripts execute line-by-line recursively with a recursion depth limit.

Janus Integration
-----------------

The shell cooperates with Janus focus switching and redraw handling.

When shell focus changes, the shell redraws its terminal region and restores prompt state automatically.
