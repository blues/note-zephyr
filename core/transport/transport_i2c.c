/**
 * Copyright (c) 2023 Blues Inc.
 * Copyright (c) 2024 Croxel Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <note.h>

#define DT_DRV_COMPAT blues_notecard

static const size_t REQUEST_HEADER_SIZE = 2;

struct notecard_config {
	const struct i2c_dt_spec bus;
};

static const char *note_i2c_receive(uint16_t addr, uint8_t *data, uint16_t size, uint32_t *avail)
{
	const struct device *dev = DEVICE_DT_GET_ANY(blues_notecard);
	const struct notecard_config *config = dev->config;
	uint8_t size_buf[2] = {0 , size};
	const int req_len = size + REQUEST_HEADER_SIZE;
	uint8_t read_buf[req_len];
	uint8_t bytes_to_read;
	int err;

	__ASSERT(config->bus.addr == addr, "%d", addr);

	/** Let the Notecard know that we are getting ready to read some data */
	err = i2c_write_dt(&config->bus, size_buf, sizeof(size_buf));
	if (err != 0) {
		return "i2c: Unable to initate read from the Notecard\n";
	}

	err = i2c_read_dt(&config->bus, read_buf, req_len);
	if (err != 0) {
		return "i2c: Unable to receive data from the Notecard.\n";
	}

	*avail = read_buf[0];

	bytes_to_read = read_buf[1];
	__ASSERT(size >= bytes_to_read, "Size: %d, Bytes to Read: %d", size, bytes_to_read);

	memcpy(data, &read_buf[2], bytes_to_read);

	return NULL;
}

static const char *note_i2c_transmit(uint16_t addr, uint8_t *data, uint16_t size)
{
	const struct device *dev = DEVICE_DT_GET_ANY(blues_notecard);
	const struct notecard_config *config = dev->config;
	int err;

	__ASSERT(config->bus.addr == addr, "%d", addr);

	uint8_t write_buf[size + 1];

	write_buf[0] = size;
	memcpy(write_buf + 1, data, size);

	err = i2c_write_dt(&config->bus, write_buf, sizeof(write_buf));
	if (err != 0) {
		return "i2c: Unable to transmit data to the Notecard\n";
	}

	return NULL;
}

static bool note_i2c_reset(uint16_t addr)
{
	ARG_UNUSED(addr);

	const struct device *dev = DEVICE_DT_GET_ONE(blues_notecard);

	if (!device_is_ready(dev)) {
		return false;
	}

	return true;
}

static int notecard_dev_init(const struct device *dev)
{
	const struct notecard_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -EIO;
	}

	NoteSetFnI2C(config->bus.addr,
		     NOTE_I2C_MAX_DEFAULT,
		     note_i2c_reset,
		     note_i2c_transmit,
		     note_i2c_receive);

	return 0;
}

#define NOTECARD_INST_DEFINE(inst)								   \
	static struct notecard_config notecard_config_##inst = {				   \
		.bus = I2C_DT_SPEC_INST_GET(inst),						   \
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
