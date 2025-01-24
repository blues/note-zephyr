# VSCode

## Compiling in VSCode

1. Open this folder in VSCode.
1. Reopen the folder in the Dev Container.
   1. Press the hotkey combination, `Ctrl+Shift+P` _(Mac: `Cmd+Shift+P`)_.
   1. Select **Dev Containers: Rebuild and Reopen in Container** from the
   command palette drop-down menu.
1. Build the binary using one of the following options:
   - Press the hotkey combination, `Ctrl+Shift+B` _(Mac: `Cmd+Shift+B`)_.
   - Use the menu system:
     1. Select **Terminal > Run Task...** from the application menu.
     1. Select **Zephyr: Build Application** from the drop-down menu.

> _**NOTE:** If you see the following message, then you have failed to update
> the product UID in the sources, and the Notecard will not be linked with your
> Notehub project._
>
> ```none
> /workspaces/note-zephyr/src/main.c:26:9: note: '#pragma message: PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid'
>    26 | #pragma message "PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid"
>       |         ^~~~~~~
> ```

### Flashing

**Linux:**

From the Dev Container, use the menu system:

1. Select **Terminal > Run Task...** from the application menu.
1. Select **Zephyr: Flash Application (Container)** from the drop-down menu.

**Windows/Mac:**

1. Launch Debug Server (OpenOCD)

   A debugging server opens a port to receive both debug and program
   instructions. Then, it forwards those instructions to the target
   device via a in-circuit debugger and programmer, such as the STLINK-V3MINI.

   Execute the following command on your host machine, _OUTSIDE_ the container:

   ```none
   openocd --search /usr/share/openocd/scripts --file interface/stlink.cfg --command "transport select hla_swd" --file target/stm32l4x.cfg
   ```

1. From the Dev Container, use the menu system:
   1. Select **Terminal > Run Task...** from the application menu.
   1. Select **Zephyr: Flash Application (External)** from the drop-down menu.

> _**NOTE:** You must flash your device using the STLINK-V3MINI; DFU is not
supported._

Debugging
---------

### Collecting Serial Logs

LPUART has been assigned as the default console output of the Swan. Furthermore,
the LPUART of the Swan is exposed via the JTAG connector. This means that all
strings provided to `printk()` will surface through the serial port assigned to
the STLINK-V3MINI. As long as the Swan has power (e.g. battery, `VIN`, etc.),
then there is no need for an additional USB cable.

The serial port is configured at 115200 baud, 8-bits, no parity bit, and one (1)
stop bit (i.e. [8-N-1](https://en.wikipedia.org/wiki/8-N-1)).

Using Linux as an example, and assuming the STLINK is the only USB peripheral
plugged into your machine. Then you can expect to find the serial port listed
as `/dev/ttyACM0`.

### GDB (OpenOCD via STLINK)

**Linux:**

   1. Select the appropriate debug configuation.
      - From the **Run and Debug** panel.
        1. Open the activity bar using one of the following options:
           - Press the hotkey combination, `Ctrl+Shift+D` _(Mac: `Cmd+Shift+D`)_.
           - Select the bug and triangle icon.
        1. Expand the drop-down with the green triangle at the top of the
           **Run and Debug** panel.
           - Use the drop-down to confirm **Swan Debug (Container)** is selected.
   1. Launch the debugger using one of the following options:
      - Press green triangle at the top of the **Run and Debug** panel.
      - Select **Run > Start Debugging** from the application menu.
      - Press the function key, `F5`.

**Windows/Mac:**

1. Launch Debug Server (OpenOCD)

   A debugging server opens a port to receive both debug and program
   instructions. Then, it forwards those instructions to the target
   device via a in-circuit debugger and programmer, such as the STLINK-V3MINI.

   Execute the following command on your host machine, _OUTSIDE_ the container:

   ```none
   openocd --search /usr/share/openocd/scripts --file interface/stlink.cfg --command "transport select hla_swd" --file target/stm32l4x.cfg
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
           - Use the drop-down to confirm **Swan Debug (External)** is selected.
   1. Launch the debugger using one of the following options:
      - Press green triangle at the top of the **Run and Debug** panel.
      - Select **Run > Start Debugging** from the application menu.
      - Press the function key, `F5`.
