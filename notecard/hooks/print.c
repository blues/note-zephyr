/**
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(note_debug);

#if defined(BLUES_NOTECARD_LOGGING)

size_t note_debug_print(const char *text)
{
	if (text) {
		LOG_INF("%s", text);
		return 1;
	}

	return 0;
}
#else

size_t note_debug_print(const char *text)
{
	return 0;
}

#endif
