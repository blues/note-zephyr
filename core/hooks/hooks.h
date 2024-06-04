/**
 * Copyright (c) 2024 Croxel Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#ifndef _NOTE_CORE_HOOKS_H_
#define _NOTE_CORE_HOOKS_H_

#include <stdlib.h>

/* Allocation Utility Hooks */
void *note_heap_alloc(size_t size);
void note_heap_free(void *ptr);

/* Time Utility Hooks */
uint32_t note_time_millis(void);
void note_time_delay(uint32_t ms);

/* Print Utility Hooks */
size_t note_debug_print(const char *text);

#endif /* _NOTE_CORE_HOOKS_H_ */
