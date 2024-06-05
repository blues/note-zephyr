/**
 * Copyright (c) 2023 Blues Inc.
 * Copyright (c) 2024 Croxel Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>

size_t note_debug_print(const char *text)
{
	if (text) {
		printk("%s", text);
		return 1;
	}

	return 0;
}