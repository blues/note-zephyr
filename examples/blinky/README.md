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

> **NOTE:** If you see the following message, then you have failed to update
> the product UID in the sources, and the Notecard will not be linked with your
> Notehub project.
>
> ```none
> /workspaces/note-zephyr/src/main.c:26:9: note: '#pragma message: PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid'
>    26 | #pragma message "PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid"
>       |         ^~~~~~~
> ```