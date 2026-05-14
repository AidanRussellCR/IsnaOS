VFS
===

The IsnaOS VFS provides an in-memory hierarchical filesystem with optional disk persistence through the ATA driver.

The filesystem supports directories, text files, binary files, learned spell scripts, and filesystem serialization.

Layout
------

The default filesystem hierarchy is:

::

    /P/root/base

The shell starts in ``/P/root/base``.

Features
--------

- Hierarchical directories
- Text and binary file support
- Persistent ATA-backed storage
- Current working directory tracking
- File moving and renaming
- Script learning system
- Lazy-loading file contents from disk
- Filesystem dirty tracking

Core Operations
---------------

``vfs_init()``
    Initialize the filesystem tree.

``vfs_load()``
    Load a serialized filesystem image from disk.

``vfs_save()``
    Save the filesystem image to disk.

``vfs_pwd()``
    Return the current working directory path.

``vfs_cd(path)``
    Change the current working directory.

``vfs_mkdir(name)``
    Create a directory.

``vfs_fab(filename)``
    Create an empty file.

``vfs_insp(filename)``
    Read text file contents.

``vfs_carve(filename, text)``
    Write text file contents.

``vfs_insp_bytes(filename)``
    Read binary file contents.

``vfs_carve_bytes(filename, data, size)``
    Write binary file contents.

``vfs_burn(filename)``
    Delete a file or empty directory.

``vfs_warp(src, dst, mode)``
    Move or rename a node.

Spell System
------------

Files ending in ``.ms`` may be marked as learned spells.

``vfs_learn(filename)``
    Mark a spell as learned.

``vfs_grimoire()``
    Enumerate learned spells.

Persistence
-----------

Filesystem metadata and file contents are serialized into an ATA-backed disk image beginning at a fixed LBA offset.

The VFS currently uses:

- superblock
- node table
- packed file data blob

Notes
-----

Files are lazily materialized from disk when first accessed. Directory nodes exist entirely in memory.
