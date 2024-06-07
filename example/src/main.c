/*
 * Copyright (c) 2023 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 *
 * Author: Zachary J. Fields
 */

// Include Zephyr Headers
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

// Include Notecard note-c library
#include <note.h>

// Notecard node-c helper methods
#include "note_c_hooks.h"

// Uncomment the define below and replace com.your-company:your-product-name
// with your ProductUID.
// #define PRODUCT_UID "com.your-company:your-product-name"

#ifndef PRODUCT_UID
#define PRODUCT_UID ""
#pragma message                                                                                    \
	"PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid"
#endif

// 10000 msec = 10 sec
#define SLEEP_TIME_MS 10000

// The devicetree node identifier for the "led0" alias.
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#else

// A build error on this line means your board is unsupported.
// See the sample documentation for information on how to fix this.
#error "Unsupported board: led0 devicetree alias is not defined"

#endif

int main(void)
{
	int ret;

	printk("[INFO] main(): Initializing...\n");

	// Initialize note-c hooks
	NoteSetUserAgent((char *)"note-zephyr");
	NoteSetFnDefault(malloc, free, platform_delay, platform_millis);
	NoteSetFnDebugOutput(note_log_print);
	NoteSetFnI2C(NOTE_I2C_ADDR_DEFAULT, NOTE_I2C_MAX_DEFAULT, note_i2c_reset, note_i2c_transmit,
		     note_i2c_receive);

	// Send a Notecard hub.set using note-c
	J *req = NoteNewRequest("hub.set");
	if (req) {
		JAddStringToObject(req, "product", PRODUCT_UID);
		JAddStringToObject(req, "mode", "continuous");
		JAddStringToObject(req, "sn", "zephyr-blink");
		if (!NoteRequest(req)) {
			printk("Failed to configure Notecard.\n");
			return -1;
		}
	} else {
		printk("Failed to allocate memory.\n");
		return -1;
	}

	if (!gpio_is_ready_dt(&led)) {
		printk("Failed to activate LED.\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Failed to configure LED.\n");
		return ret;
	}

	// Application Loop
	printk("[INFO] main(): Entering loop...\n");
	while (1) {
		// Toggle LED state
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			printk("Failed to toggle LED.\n");
			break;
		}

		// Report LED state to Notehub.io
		J *req = NoteNewRequest("note.add");
		if (req) {
			JAddBoolToObject(req, "sync", true);
			J *body = JAddObjectToObject(req, "body");
			JAddBoolToObject(body, "LED", gpio_pin_get(led.port, led.pin));
			if (!NoteRequest(req)) {
				printk("Failed to submit Note to Notecard.\n");
				ret = -1;
				break;
			}
		} else {
			printk("Failed to allocate memory.\n");
			ret = -1;
			break;
		}

		// Wait to iterate
		k_msleep(SLEEP_TIME_MS);
	}

	return ret;
}