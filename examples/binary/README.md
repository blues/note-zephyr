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

## Header generation

`blues_logo.png` is converted to a C byte array at configure time by `scripts/png_to_header.py`, which is invoked automatically by `CMakeLists.txt` via `execute_process`. The generated `blues_logo.h` is written to the CMake build directory and is not checked into the repository.

To use a different image, replace `blues_logo.png` and rebuild — the header will be regenerated automatically. You can also run the script directly:

```bash
python3 scripts/png_to_header.py <input.png> <output.h> <variable_name>
```
