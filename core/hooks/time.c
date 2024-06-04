/**
 * Copyright (c) 2023 Blues Inc.
 * Copyright (c) 2024 Croxel Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <stdint.h>
#include <zephyr/kernel.h>

uint32_t note_time_millis(void)
{
	return (uint32_t)k_uptime_get();
}

void note_time_delay(uint32_t ms)
{
	k_msleep(ms);
}
