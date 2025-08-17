# binary

This example demonstrates how to use the Notecard with Zephyr. It shows how to send and receive binary data to and from the Notecard (and to and from Notehub).

```bash
# Build for I2C
west build examples/binary -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/i2c.overlay
west flash

# Build for UART
west build examples/binary -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/uart.overlay
west flash
```
