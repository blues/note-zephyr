/**
 * Copyright (c) 2024 Croxel Inc.
 * Copyright (c) 2024 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <note.h>

#define DT_DRV_COMPAT blues_notecard

struct notecard_config {
	const struct device *bus;
};

struct notecard_data {
	struct {
		struct ring_buf *rx_ringbuf;
	} uart;
};

static void note_uart_driver_cb(const struct device *dev, void *user_data)
{
	struct notecard_data *data = (struct notecard_data *)user_data;
	uint8_t c;
	int err;

	__ASSERT_NO_MSG(device_is_ready(dev));

	if (!uart_irq_update(dev)) {
		return;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &c, 1) == 1) {
		err = ring_buf_put(data->uart.rx_ringbuf, &c, 1);
		__ASSERT_NO_MSG(err == 1);
	}
}

static bool note_serial_reset(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(blues_notecard);
	struct notecard_data *data = dev->data;

	__ASSERT_NO_MSG(device_is_ready(dev));

	ring_buf_reset(data->uart.rx_ringbuf);

	return true;
}

static void note_serial_transmit(uint8_t *data, size_t len, bool flush)
{
	const struct device *dev = DEVICE_DT_GET_ANY(blues_notecard);
	const struct notecard_config *config = dev->config;

	__ASSERT_NO_MSG(device_is_ready(dev));

	for (size_t i = 0 ; i < len ; i++) {
		uart_poll_out(config->bus, data[i]);
	}
}

static bool note_serial_available(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(blues_notecard);
	struct notecard_data *data = dev->data;

	__ASSERT_NO_MSG(device_is_ready(dev));

	if (!ring_buf_is_empty(data->uart.rx_ringbuf)) {
		return true;
	}

	return false;
}

static char note_serial_receive(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(blues_notecard);
	struct notecard_data *data = dev->data;
	char c;
	uint32_t result;

	__ASSERT_NO_MSG(device_is_ready(dev));

	if (!ring_buf_is_empty(data->uart.rx_ringbuf)) {
		result = ring_buf_get(data->uart.rx_ringbuf, &c, 1);
		__ASSERT(result == 1, "%d", result);

		return c;
	}

	return 0;
}

static int notecard_dev_init(const struct device *dev)
{
	const struct notecard_config *config = dev->config;
	struct notecard_data *data = dev->data;
	int err;

	if (!device_is_ready(config->bus)) {
		__ASSERT_NO_MSG(false);
		return -EIO;
	}

	err = uart_irq_callback_user_data_set(config->bus, note_uart_driver_cb, data);
	__ASSERT(!err, "%d", err);

	uart_irq_rx_enable(config->bus);

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
	RING_BUF_DECLARE(notecard_uart_rx_rb_##inst, 1024);					   \
												   \
	static struct notecard_data notecard_data_##inst = {					   \
		.uart = {									   \
			.rx_ringbuf = &notecard_uart_rx_rb_##inst,				   \
		},										   \
	};											   \
												   \
	DEVICE_DT_INST_DEFINE(inst,								   \
			      notecard_dev_init,						   \
			      NULL,								   \
			      &notecard_data_##inst,						   \
			      &notecard_config_##inst,						   \
			      POST_KERNEL,							   \
			      CONFIG_APPLICATION_INIT_PRIORITY,					   \
			      NULL);

/* Only supports single-instance driver. Needs a device reference to support
 * multi-instance driver.
 */
NOTECARD_INST_DEFINE(0)
