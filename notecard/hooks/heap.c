/**
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>

/** Use a separate Heap to prevent from utilizing shared
 * RTOS resources and therefore improve module stability.
 */
K_HEAP_DEFINE(note_heap, CONFIG_BLUES_NOTECARD_HOOKS_HEAP_SIZE);

#define NOTE_ALLOC_TIMEOUT K_MSEC(CONFIG_BLUES_NOTECARD_HOOKS_HEAP_TIMEOUT)

void *note_heap_alloc(size_t size)
{
	void *ptr = k_heap_alloc(&note_heap, size, NOTE_ALLOC_TIMEOUT);

	__ASSERT_NO_MSG(ptr);

	return ptr;
}

void note_heap_free(void *ptr)
{
	k_heap_free(&note_heap, ptr);
}
