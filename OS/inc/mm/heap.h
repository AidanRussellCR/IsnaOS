#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * heap_init - initialize the kernel heap
 *
 * Sets the initial heap break to the end of the kernel image.
 */
void heap_init(void);

/**
 * kmalloc - allocate memory from the kernel heap
 * @bytes: number of bytes requested
 *
 * Return: pointer to allocated memory, NULL on failure/zero size
 */
void* kmalloc(size_t bytes);

/**
 * kfree - free memory allocated by kmalloc
 * @ptr: pointer previously returned by kmalloc
 */
void kfree(void* ptr);
