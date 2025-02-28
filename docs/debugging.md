# Debugging

In order to allow for debugging `note-zephyr` projects, we have created two debug configurations, `Debug (Native)` and `Debug (External)`. These configurations both allow for step by step debugging, and can be used to set breakpoints and inspect variables.

To use these configurations, you can open the **Run and Debug** panel and select the configuration you want to use.

## Collecting Serial Logs

LPUART has been assigned as the default console output of the Feather MCUs. Furthermore,
the LPUART is exposed via the JTAG connector. This means that all strings provided to `printk()`, `LOG_INF()`, etc. will surface through the serial port assigned to the STLINK-V3MINI.
As long as the Feather has power (e.g. battery, `VIN`, etc.), then there is no need for an additional USB cable.

The serial port is configured at 115200 baud, 8-bits, no parity bit, and one (1)
stop bit (i.e. [8-N-1](https://en.wikipedia.org/wiki/8-N-1)).

Using Linux as an example, and assuming the STLINK is the only USB peripheral
plugged into your machine. Then you can expect to find the serial port listed
as `/dev/ttyACM0`.

## Local

This configuration is supported by all operating systems, when using the `native` workflow. Under the `devcontainer` workflow, this configuration is only supported on Linux.

   1. Select the appropriate debug configuation.
      - From the **Run and Debug** panel.
        1. Open the activity bar using one of the following options:
           - Press the hotkey combination, `Ctrl+Shift+D` _(Mac: `Cmd+Shift+D`)_.
           - Select the bug and triangle icon.
        1. Expand the drop-down with the green triangle at the top of the
           **Run and Debug** panel.
           - Use the drop-down to confirm **Debug (Native)** is selected.
   1. Launch the debugger using one of the following options:
      - Press green triangle at the top of the **Run and Debug** panel.
      - Select **Run > Start Debugging** from the application menu.
      - Select your board, e.g. `swan_r5`.
      - Press the function key, `F5`.

## External

This configuration is required if you using the devcontainer on Windows or MacOS.

1. Launch Debug Server (OpenOCD)

   A debugging server opens a port to receive both debug and program
   instructions. Then, it forwards those instructions to the target
   device via a in-circuit debugger and programmer, such as the STLINK-V3MINI.

   Execute the following command on your host machine, _OUTSIDE_ the container:

   ```none
   openocd -f swan_r5.cfg
   ```

1. Launch Debugger (GDB)

   A debugger is a piece of software that allows you to step through a binary
   on a line-by-line basis. When debugging an embedded device, the binary does
   not reside on the same machine as the debugger, so we need a server (e.g.
   OpenOCD) to relay the instructions to the remote binary.

   1. Select the appropriate debug configuation.
      - From the **Run and Debug** panel.
        1. Open the activity bar using one of the following options:
           - Press the hotkey combination, `Ctrl+Shift+D` _(Mac: `Cmd+Shift+D`)_.
           - Select the bug and triangle icon.
        1. Expand the drop-down with the green triangle at the top of the
           **Run and Debug** panel.
           - Use the drop-down to confirm **Debug (External)** is selected.
   1. Launch the debugger using one of the following options:
      - Press green triangle at the top of the **Run and Debug** panel.
      - Select **Run > Start Debugging** from the application menu.
      - Select your board, e.g. `swan_r5`.
      - Press the function key, `F5`.
