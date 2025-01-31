# blinky

This example demonstrates how to use the Notecard with Zephyr. It toggles an LED and sends a Note to the Notecard with the status of the LED.
It shows how to use the Notecard with both I2C and UART.

```bash
# Build for I2C
west build examples/blinky -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/i2c.overlay
west flash

# Build for UART
west build examples/blinky -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/uart.overlay
west flash
```
