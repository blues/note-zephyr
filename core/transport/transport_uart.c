/**
 * Copyright (c) 2024 Croxel Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <note.h>

#error "transport_uart.c not yet implemented!"

#define DT_DRV_COMPAT blues_notecard

struct notecard_config {
	const struct device *bus;
};

static bool note_serial_reset(void)
{
	/** @TODO: implement */
	return true;
}

static void note_serial_transmit(uint8_t *data, size_t len, bool flush)
{
	/** @TODO: implement */
}

static bool note_serial_available(void)
{
	/** @TODO: implement */
	return false;
}

static char note_serial_receive(void)
{
	/** @TODO: implement */
	return 0;
}

static int notecard_dev_init(const struct device *dev)
{
	const struct notecard_config *config = dev->config;

	if (!device_is_ready(config->bus)) {
		return -EIO;
	}

	/** @TODO: Add UART initialization */

	NoteSetFnSerial(note_serial_reset,
			note_serial_transmit,
			note_serial_available,
			note_serial_receive);

	return 0;
}

#define NOTECARD_INST_DEFINE(inst)								   \
	static struct notecard_config notecard_config_##inst = {				   \
		.bus = DEVICE_DT_GET(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),			   \
	};											   \
												   \
	DEVICE_DT_INST_DEFINE(inst,								   \
			      notecard_dev_init,						   \
			      NULL,								   \
			      NULL,								   \
			      &notecard_config_##inst,						   \
			      POST_KERNEL,							   \
			      CONFIG_APPLICATION_INIT_PRIORITY,					   \
			      NULL);

/* Only supports single-instance driver. Needs a device reference to support
 * multi-instance driver.
 */
NOTECARD_INST_DEFINE(0)
