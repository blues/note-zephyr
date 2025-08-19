/**
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

 #include <zephyr/kernel.h>
 #include <note.h>

static int note_user_agent_register(void)
{
	NoteSetUserAgent((char *)"note-zephyr");

	return 0;
}

SYS_INIT(note_user_agent_register,
    APPLICATION,
    CONFIG_APPLICATION_INIT_PRIORITY);
