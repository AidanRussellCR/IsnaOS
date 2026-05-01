#pragma once
#include <stdint.h>

/**
 * ctx_switch - switch from the current kernel task stack to another
 * @old_esp: storage location for the outgoing task's saved ESP
 * @new_esp: saved ESP value of the task to resume
 *
 * Saves the current execution context and restores the context
 * pointed to by @new_esp.
 */
void ctx_switch(uint32_t* old_esp, uint32_t new_esp);
