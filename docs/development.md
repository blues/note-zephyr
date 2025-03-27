# Development

Building and flashing applications is provided by the `west` tool, with the exception of the `external` tasks. We provide [tasks](../.vscode/tasks.json) in VSCode to make it easier to build and flash applications.

## Compiling

Build the binary using one of the following options:
   - Press the hotkey combination, `Ctrl+Shift+B` _(Mac: `Cmd+Shift+B`)_.
   - Use the menu system:
     1. Select **Terminal > Run Task...** from the application menu.
     2. Select **Zephyr: Build Application** from the drop-down menu.

Alternatively, you can use the `west build` command from the terminal:

```bash
west build -b swan_r5 examples/blinky
```

## Flashing

### Local

From the VSCode, use the menu system:

1. Select **Terminal > Run Task...** from the application menu.
1. Select **Zephyr: Flash Application** from the drop-down menu.

Alternatively, you can use the `west flash` command from the terminal:

```bash
west flash
```

### External

1. Launch Debug Server (OpenOCD)

   A debugging server opens a port to receive both debug and program
   instructions. Then, it forwards those instructions to the target
   device via a in-circuit debugger and programmer, such as the STLINK-V3MINI.

   Execute the following command on your host machine, _OUTSIDE_ the container:

   ```none
   openocd -f swan_r5.cfg
   ```

1. From the Dev Container, use the menu system:
   1. Select **Terminal > Run Task...** from the application menu.
   1. Select **Zephyr: Flash Application (External)** from the drop-down menu.

> **NOTE:** You must flash your device using the STLINK-V3MINI; DFU is not
> supported.