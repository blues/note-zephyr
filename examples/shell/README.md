# shell

This example demonstrates how to use the Notecard with Zephyr's Shell Utility. Example subcommands include:

- `notecard status` - Get the status of the Notecard
- `notecard sync` - Sync the Notecard with Notehub

```bash
# Build for I2C
west build examples/shell -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/i2c.overlay
west flash

# Build for UART
west build examples/shell -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/uart.overlay
west flash
```

A Zephyr shell prompt is provided for the Swan over the STLINK-V3 debugger serial port.

> **Note:** This is not a direct shell into the Notecard. It is a Zephyr shell running on the host machine that is connected to the Notecard via i2c or uart. Use this to abstract complex interactions with the Notecard.
