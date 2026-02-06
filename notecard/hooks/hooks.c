/**
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>
#include <note.h>

#include "hooks/hooks.h"

static int note_hooks_register(void)
{
	NoteSetFnDefault(note_heap_alloc, note_heap_free,
			 note_time_delay, note_time_millis);

	NoteSetFnDebugOutput(note_debug_print);

	NoteSetFnNoteMutex(note_platform_note_lock, note_platform_note_unlock);

#ifdef CONFIG_BLUES_NOTECARD_I2C_MUTEX
	NoteSetFnI2CMutex(note_platform_i2c_lock, note_platform_i2c_unlock);
#endif

	return 0;
}

SYS_INIT(note_hooks_register,
	 APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
