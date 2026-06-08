/*
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <note.h>
#include "blues_logo.h"

#ifndef PRODUCT_UID
#define PRODUCT_UID ""
#pragma message "PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid"
#endif

#define SLEEP_TIME_MS 30000
#define MAX_ITERATIONS 5

LOG_MODULE_REGISTER(main);

int main(void)
{
    LOG_INF("Initializing binary send/receive example...");

    // Initialize note-c hooks
    NoteSetUserAgent((char *)"note-zephyr");

    // Configure the Notecard
    J *req = NoteNewRequest("hub.set");
    if (req) {
        JAddStringToObject(req, "product", PRODUCT_UID);
        JAddStringToObject(req, "mode", "continuous");
        if (!NoteRequest(req)) {
            LOG_ERR("Failed to configure Notecard.");
            return -1;
        }
    } else {
        LOG_ERR("Failed to allocate memory for hub.set request.");
        return -1;
    }

    // Reset the binary store
    const char *err = NoteBinaryStoreReset();
    if (err != NULL) {
        LOG_ERR("Failed to reset binary store: %s", err);
        return -1;
    }

    uint8_t event_counter = 0;
    while (1) {
        if (++event_counter > MAX_ITERATIONS) {
            LOG_INF("Demo complete. Program stopping to conserve data.");
            return 0;
        }

        const uint32_t notecard_binary_area_offset = 0;

        // Transmit data to Notecard storage
        err = NoteBinaryStoreTransmit((uint8_t *)blues_logo_png, blues_logo_png_len, blues_logo_png_len, notecard_binary_area_offset);
        if (err != NULL) {
            LOG_ERR("Failed to transmit binary data: %s", err);
            NoteBinaryStoreReset();
            k_msleep(SLEEP_TIME_MS);
            continue;
        }
        LOG_INF("Transmitted %d bytes", blues_logo_png_len);

        // Receive data length from Notecard storage
        uint32_t rx_data_len = 0;
        err = NoteBinaryStoreDecodedLength(&rx_data_len);
        if (err != NULL) {
            LOG_ERR("Failed to get decoded length: %s", err);
            NoteBinaryStoreReset();
            k_msleep(SLEEP_TIME_MS);
            continue;
        }

        // Allocate receive buffer
        uint32_t rx_buffer_len = NoteBinaryCodecMaxEncodedLength(rx_data_len);
        uint8_t *rx_buffer = k_malloc(rx_buffer_len);
        if (!rx_buffer) {
            LOG_ERR("Failed to allocate receive buffer");
            NoteBinaryStoreReset();
            k_msleep(SLEEP_TIME_MS);
            continue;
        }

        // Receive the actual data from Notecard storage
        err = NoteBinaryStoreReceive(rx_buffer, rx_buffer_len, 0, rx_data_len);
        if (err != NULL) {
            LOG_ERR("Failed to receive binary data: %s", err);
            k_free(rx_buffer);
            NoteBinaryStoreReset();
            k_msleep(SLEEP_TIME_MS);
            continue;
        }
        LOG_INF("Received %d bytes", rx_data_len);

        k_free(rx_buffer);

        // Send binary data to Notehub
        req = NoteNewRequest("note.add");
        if (req) {
            JAddStringToObject(req, "file", "data.qo");
            JAddBoolToObject(req, "binary", true);
            JAddBoolToObject(req, "live", true);
            if (!NoteRequest(req)) {
                LOG_ERR("Failed to send binary note to Notehub");
                NoteBinaryStoreReset();
            } else {
                LOG_INF("Binary note sent to Notehub");
            }
        } else {
            LOG_ERR("Failed to allocate memory for note.add request");
            NoteBinaryStoreReset();
        }

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
