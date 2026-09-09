# outboard-dfu

This example enables [Notecard Outboard Firmware Update](https://dev.blues.io/notehub/host-firmware-updates/notecard-outboard-firmware-update/) (ODFU) for an STM32 host — the Blues [Swan](https://shop.blues.com/collections/feather-mcu/products/swan) or [Cygnet](https://shop.blues.com/collections/feather-mcu/products/cygnet) — on a Notecarrier F, then blinks the onboard LED and sends a Note with the LED state to the Notecard.

Once ODFU is enabled, the Notecard can flash a new host firmware image to the MCU over-the-air with no host involvement. The example configures ODFU at startup with three requests:

```json
{"req":"card.dfu","name":"stm32","on":true,"mode":"aux"}
{"req":"card.aux","mode":"off"}
{"req":"dfu.status","on":true,"version":"1.0.0"}
```

On a Notecarrier F the DFU signals are routed over the Notecard's AUX pins, so `mode` is set to `aux` on `card.dfu` and `card.aux` is set to `off` to free those pins for DFU.

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
