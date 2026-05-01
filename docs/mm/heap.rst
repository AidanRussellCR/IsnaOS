Heap
====

The heap subsystem provides a simple kernel allocator based on a linked list of
heap blocks.

The allocator starts at the linker-provided ``end`` symbol and grows upward as
new blocks are requested. Blocks are aligned to 16 bytes.

Functions
---------

``heap_init()``
    Initialize the heap break to the aligned end of the kernel image.

``kmalloc(bytes)``
    Allocate a block of memory. Reuses free blocks when possible, otherwise
    requests a new block from the current heap break.

``kfree(ptr)``
    Mark a block as free and attempt to coalesce adjacent free blocks.

Internal Helpers
----------------

``align16(x)``
    Align an address or size to a 16-byte boundary.

``request_block(size)``
    Create a new heap block at the current heap break.

``split_block(block, want)``
    Split a larger free block if enough space remains.

``coalesce(block)``
    Merge adjacent free blocks after freeing memory.

Limitations
-----------

This allocator does not currently enforce a heap limit or page-level backing.
It assumes the heap can grow upward from the kernel end symbol.
