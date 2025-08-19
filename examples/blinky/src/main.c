/*
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 *
 * Author: Zachary J. Fields
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Include Notecard note-c library
#include <note.h>

#define SLEEP_TIME_MS 10000

#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

LOG_MODULE_REGISTER(main);

int main(void)
{
    int ret;

    LOG_INF("Initializing...");

    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("Failed to activate LED.");
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure LED.");
        return ret;
    }

    // Send a Notecard hub.set using note-c
    J *req = NoteNewRequest("hub.set");
    if (req) {
        JAddStringToObject(req, "product", CONFIG_BLUES_NOTEHUB_PRODUCT_UID);
        JAddStringToObject(req, "mode", "continuous");
        JAddStringToObject(req, "sn", "zephyr-blink");
        if (!NoteRequest(req))
        {
            LOG_ERR("Failed to configure Notecard.");
            return -1;
        }
    }
    else {
        LOG_ERR("Failed to allocate memory.");
        return -1;
    }

    LOG_INF("Entering main loop...");
    while (1)
    {
        // Toggle LED state
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0)
        {
            LOG_ERR("Failed to toggle LED.");
            break;
        }

        // Report LED state to Notehub.io
        J *req = NoteNewRequest("note.add");
        if (req)
        {
            JAddBoolToObject(req, "sync", true);
            J *body = JAddObjectToObject(req, "body");
            JAddBoolToObject(body, "LED", gpio_pin_get(led.port, led.pin));
            if (!NoteRequest(req))
            {
                LOG_ERR("Failed to submit Note to Notecard.");
                ret = -1;
                break;
            }
            LOG_INF("Note sent to Notehub.");
        }
        else
        {
            LOG_ERR("Failed to allocate memory.");
            ret = -1;
            break;
        }

        k_msleep(SLEEP_TIME_MS);
    }

    return ret;
}
