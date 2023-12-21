# Example

This example uses the `note-c` library to send a note using the Notecard, to Notehub. Initially the example will configure the notecard and then will send a note every 10 seconds with the state of the onboard LED. This example specifically targets the [Swan MCU](https://blues.com/products/swan/) and one of the Blues [carrier boards](https://blues.com/products/notecarrier/) (when connected via i2c) and provides building / flashing instructions for this board, using the STLINK-V3 debugger.

## Build

To build this example, you can use the following command from the root of this repo:

```bash
west build -b swan_r5 example
```

## Flash

To flash this example to the Swan MCU, you can use the following command:

```bash
west flash
```
