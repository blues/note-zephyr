# outboard-dfu

This example enables [Notecard Outboard Firmware Update](https://dev.blues.io/notehub/host-firmware-updates/notecard-outboard-firmware-update/) (ODFU) for an STM32 host — the Blues [Swan](https://shop.blues.com/collections/feather-mcu/products/swan) or [Cygnet](https://shop.blues.com/collections/feather-mcu/products/cygnet) — on a Notecarrier F, then blinks the onboard LED and sends a Note with the LED state to the Notecard.

Once ODFU is enabled, the Notecard can flash a new host firmware image to the MCU over-the-air with no host involvement. The example configures ODFU at startup with three requests:

```json
{"req":"card.dfu","name":"stm32","on":true,"mode":"aux"}
{"req":"card.aux","mode":"off"}
{"req":"dfu.status","on":true,"version":"1.0.0"}
```

On a Notecarrier F the DFU signals are routed over the Notecard's AUX pins, so `mode` is set to `aux` on `card.dfu` and `card.aux` is set to `off` to free those pins for DFU.

All three requests are checked, and the example exits if any of them fails. That is deliberate: nothing later in the example depends on them, so without the check a Notecard that rejected `card.dfu` would still blink the LED and still send Notes, and you would not find out that Outboard DFU was never enabled until an update silently failed to arrive.

## Testing an update

`dfu.status` reports the running firmware version to Notehub, and the example takes it from `FIRMWARE_VERSION` in [`src/main.c`](src/main.c):

```c
#define FIRMWARE_VERSION "1.0.0"
```

To watch an update complete end to end:

1. Build and flash as below, leaving the version at `1.0.0`.
2. Bump `FIRMWARE_VERSION` to `1.0.1` and rebuild.
3. Upload `build/zephyr/zephyr.bin` to Notehub and schedule the update — see [Uploading Firmware to Notehub](https://dev.blues.io/guides-and-tutorials/notecard-guides/notecard-outboard-firmware-update/#uploading-firmware-to-notehub).

Notehub uses the reported version to tell what is running, so if you skip step 2 a successful update looks identical to nothing having happened.

## Building and flashing

Build for the Swan (`swan_r5`) or Cygnet (`cygnet`), over I2C or UART:

```bash
# Swan, I2C
west build examples/outboard-dfu -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/i2c.overlay
west flash

# Swan, UART
west build examples/outboard-dfu -b swan_r5 -DDTC_OVERLAY_FILE=../overlays/uart.overlay
west flash

# Cygnet (swap the board target)
west build examples/outboard-dfu -b cygnet -DDTC_OVERLAY_FILE=../overlays/i2c.overlay
west flash
```

> **NOTE:** If you see a `PRODUCT_UID is not defined` message at build time, set your
> Product UID — either uncomment and edit `CONFIG_BLUES_NOTEHUB_PRODUCT_UID` in
> `prj.conf`, or set the ProductUID directly on the Notecard. More details at
> https://bit.ly/product-uid
