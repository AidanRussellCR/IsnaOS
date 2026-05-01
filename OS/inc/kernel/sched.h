#pragma once

/*
 * schedule - select and switch to the next ready task
 *
 * Runs the scheduler one time. Current implementation searches
 * for the next ready task and switches to it once possible.
 */
void schedule(void);

/*
 * yield - give the scheduler a chance to run
 *
 * Tasks call this when they are waiting or want to multitask.
 */
void yield(void);
