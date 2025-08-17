/*
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>

// Include Notecard note-c library
#include <note.h>

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
    NoteBinaryStoreReset();

    uint8_t event_counter = 0;
    while (1) {
        if (++event_counter > MAX_ITERATIONS) {
            LOG_INF("Demo complete. Program stopping to conserve data.");
            return 0;
        }

        // Example data to transmit
        char data[] = "https://youtu.be/0epWToAOlFY?t=21";
        uint32_t data_len = strlen(data);
        const uint32_t notecard_binary_area_offset = 0;

        // Transmit data to Notecard storage
        NoteBinaryStoreTransmit((uint8_t *)data, data_len, sizeof(data), notecard_binary_area_offset);
        LOG_INF("Transmitted %d bytes", data_len);

        // Receive data length from Notecard storage
        uint32_t rx_data_len = 0;
        NoteBinaryStoreDecodedLength(&rx_data_len);

        // Allocate receive buffer
        uint32_t rx_buffer_len = NoteBinaryCodecMaxEncodedLength(rx_data_len);
        uint8_t *rx_buffer = k_malloc(rx_buffer_len);
        if (!rx_buffer) {
            LOG_ERR("Failed to allocate receive buffer");
            return -1;
        }

        // Receive the actual data from Notecard storage
        NoteBinaryStoreReceive(rx_buffer, rx_buffer_len, 0, rx_data_len);
        LOG_INF("Received %d bytes: %.*s", rx_data_len, rx_data_len, rx_buffer);

        k_free(rx_buffer);

        // Send binary data to Notehub
        req = NoteNewRequest("note.add");
        if (req) {
            JAddStringToObject(req, "file", "cobs.qo");
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
