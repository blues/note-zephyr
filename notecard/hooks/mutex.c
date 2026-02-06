/**
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>

static K_MUTEX_DEFINE(g_notecard_mutex);

void note_platform_note_lock(void)
{
	k_mutex_lock(&g_notecard_mutex, K_FOREVER);
}

void note_platform_note_unlock(void)
{
	k_mutex_unlock(&g_notecard_mutex);
}

#ifdef CONFIG_BLUES_NOTECARD_I2C_MUTEX
static K_MUTEX_DEFINE(g_i2c_mutex);

void note_platform_i2c_lock(void)
{
	k_mutex_lock(&g_i2c_mutex, K_FOREVER);
}

void note_platform_i2c_unlock(void)
{
	k_mutex_unlock(&g_i2c_mutex);
}
#endif /* CONFIG_BLUES_NOTECARD_I2C_MUTEX */
