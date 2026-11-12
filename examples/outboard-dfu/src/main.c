/*
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

// This example enables Notecard Outboard Firmware Update (ODFU) for an STM32
// host (Blues Swan or Cygnet) on a Notecarrier F, then blinks the onboard LED
// and reports its state to Notehub. Once ODFU is enabled and a compatible host
// firmware image has been uploaded to Notehub, the Notecard can update this
// host over-the-air with no host involvement.

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Include Notecard note-c library
#include <note.h>

#define SLEEP_TIME_MS 10000

// Reported to Notehub by dfu.status. Bump this before rebuilding if you want
// to watch an Outboard DFU update complete.
#define FIRMWARE_VERSION "1.0.0"

#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

LOG_MODULE_REGISTER(main);

// Issue one of the requests that enable Outboard DFU, failing loudly if it
// does not land. Outboard DFU is the whole point of this example, but nothing
// downstream depends on it: if one of these requests is dropped the LED still
// blinks and Notes still arrive at Notehub, so a silent failure is
// indistinguishable from success right up until an update is attempted.
// Takes ownership of `req`, which may be NULL if the allocation failed.
static bool dfu_request(J *req, const char *name)
{
    if (!req) {
        LOG_ERR("Failed to allocate memory for %s request.", name);
        return false;
    }

    if (!NoteRequest(req)) {
        LOG_ERR("%s request failed; Outboard DFU is not enabled.", name);
        return false;
    }

    return true;
}

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

    // Associate the Notecard with a Notehub project and keep it connected.
    J *req = NoteNewRequest("hub.set");
    if (req) {
        JAddStringToObject(req, "product", CONFIG_BLUES_NOTEHUB_PRODUCT_UID);
        JAddStringToObject(req, "mode", "continuous");
        JAddStringToObject(req, "sn", "zephyr-outboard-dfu");
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

    // Enable Notecard Outboard Firmware Update for this STM32 host. On a
    // Notecarrier F the DFU signals are routed over the shared AUX pins.
    req = NoteNewRequest("card.dfu");
    if (req) {
        JAddStringToObject(req, "name", "stm32");
        JAddBoolToObject(req, "on", true);
        JAddStringToObject(req, "mode", "aux");
    }
    if (!dfu_request(req, "card.dfu")) {
        return -1;
    }

    // Free the AUX pins so they can be used for Outboard Firmware Update.
    req = NoteNewRequest("card.aux");
    if (req) {
        JAddStringToObject(req, "mode", "off");
    }
    if (!dfu_request(req, "card.aux")) {
        return -1;
    }

    // Enable host DFU and report the running firmware version to Notehub.
    // Bump FIRMWARE_VERSION when you rebuild to test an update -- Notehub uses
    // this string to tell what is running, so leaving it unchanged makes a
    // successful update look like nothing happened.
    req = NoteNewRequest("dfu.status");
    if (req) {
        JAddBoolToObject(req, "on", true);
        JAddStringToObject(req, "version", FIRMWARE_VERSION);
    }
    if (!dfu_request(req, "dfu.status")) {
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
        req = NoteNewRequest("note.add");
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
