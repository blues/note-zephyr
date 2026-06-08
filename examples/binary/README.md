# binary

This example demonstrates how to send and receive binary data using the Notecard. It uses the Blues logo PNG as a sample payload, writing it to the Notecard's binary store, reading it back to verify the transfer, then forwarding it to Notehub via a `note.add` request with `"binary": true`.

```bash
# Build for I2C
west build examples/binary -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/i2c.overlay
west flash

# Build for UART
west build examples/binary -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/uart.overlay
west flash
```
