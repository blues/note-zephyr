# note-zephyr

A `west` compatible [Zephyr RTOS](https://zephyrproject.org/) module for the [Notecard](https://shop.blues.io/collections/notecard). Include this module in your own project manifest and use the `notecard` module to access the Notecard.

## Overview

This example is designed to demonstrate the ease of adding Notecard functionality
to an existing Zephyr application.
To quickly get started, use the `blinky` example, check out the `examples/blinky` project.

## Requirements

To use this module, we make the following assumptions to make it easier to get started:

### Hardware

- Any [Blues Notecard](https://shop.blues.io/collections/notecard)
- [Blues Notecarrier F](https://shop.blues.com/products/notecarrier-f)
- [Blues Swan](https://shop.blues.io/products/swan)
- [STLINK Programmer/Debugger](https://shop.blues.io/collections/accessories/products/stlink-v3mini)

### Software

- [Docker](https://docs.docker.com/get-docker/)
- [VSCode](https://code.visualstudio.com/Download)
- [VSCode "Dev Containers" Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- **\[Windows/Mac Debugging\]** [OpenOCD](https://openocd.org/pages/getting-openocd.html)

### Cloudware

- [Notehub.io](https://notehub.io)

## Getting Setup

### Notehub.io

Before you can utilize this example, you must set up a free account (no credit
card required) on [Notehub.io](https://notehub.io). Once you have created your
account, then you need to
[create a project](https://dev.blues.io/quickstart/notecard-quickstart/notecard-simulator/#create-a-notehub-project)
to serve as an endpoint for the Notes that are tracking the state of the LED.

Once you have a project, you will need to update the `define` named
`PROJECT_UID` in `main.c` with [the UID of the project you have just
created](https://blues.dev/notehub/notehub-walkthrough/#finding-a-productuid).

After the Notecard has connected to Notehub, you can look inside the project
and see a device named `zephyr-blink`. The Notecard will be running in
`continuous` mode, which will allow it to maintain a constant cellular
connection. `continuous` mode offers the lowest latency possible for sending
messages to Notehub, but it comes at the cost of battery life. Fortunately,
this is typically not a concern while bench testing, because you are plugged
into USB power.

For more information about the Notecard API and examples, please visit
[our developer site](https://blues.dev).

### Cloning the Repository

This repository contains the `note-c` library as a submodule. Use the following
command to clone both repositories simultaneously.

```none
git clone https://github.com/blues/note-zephyr.git --recursive
```

If you cloned without the `--recursive` flag, then you can update the `note-c`
submodule separately, using the following two commands:

```none
git submodule init
git submodule update
```

### Building the Dev Container

> _**WARNING:** This step is critical to ensure you correctly build the Dev
Container image._

**Linux:**

To enable flashing and debugging from the container on Linux, you will need to
provide access to the USB controller of the host machine.

Perform the following steps, in order to provide USB access:

1. Open `./.devcontainer/devcontainer.json`.
2. Uncomment the `runArgs` section:

    ```json
    // Uncomment the following section if your host machine is running Linux
    "runArgs": [
        "--device=/dev/bus/usb/"
    ],
    ```

   > _**NOTE:** At the time of writing, it is not possible to share the host USB
   > from Windows and Mac computers._

**Windows/Mac (x86_64):**

Ensure Docker Desktop is running.

**ARM 64 (aarch64):**

1. Open `./.devcontainer/devcontainer.json`.
2. Uncomment the `args` object in the `build` section:

    ```json
    // Uncomment the following section if your host machine silicon is ARM64
    "args": {
        "HOST_ARCH":"aarch64"
    },
    ```

> _**NOTE:** If you failed to properly update `devcontainer.json` before opening
> the Dev Container, you may need to purge your docker build cache before
> trying again._
>
> ```none
> $ docker system prune
> WARNING! This will remove:
>   - all stopped containers
>   - all networks not used by at least one container
>   - all dangling images
>   - all dangling build cache
>
> Are you sure you want to continue? [y/N] y
> ```

Building and Running
--------------------

### Compiling

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
