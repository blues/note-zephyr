# Examples

## Getting started

These examples demonstrate how to use the Notecard with Zephyr. Examples include:

- [blinky](./blinky/README.md) - Toggle an LED and sends a Note to the Notecard with the status of the LED. A modification of Zephyr's Blinky example.
- [message queues](./message-queues/README.md) - Demonstrates how to use the Zephyr's Message Queue API to send and receive messages across threads while handling real-time events and offloading the data to Notecard for cloud upload.
- [shell](./shell/README.md) - Drive the Notecard interactively from the Zephyr shell.
- [outboard-dfu](./outboard-dfu/README.md) - Enable Notecard Outboard Firmware Update (ODFU) for an STM32 host (Swan or Cygnet), then blink an LED and report its state. Shows how to update host firmware over-the-air.
- [mcuboot-blinky](./mcuboot-blinky/README.md) - Notecard Outboard Firmware Update on an Adafruit Feather nRF52840 using the MCUboot bootloader. Targets the nRF52840 (not `swan_r5`).
