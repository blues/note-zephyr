# note-zephyr

This is a Zephyr `west` module for the Blues Notecard. It is an abstraction layer for the [note-c](https://github.com/blues/note-c) library, that allows you to easily use a [Notecard](https://blues.com/products/notecard/) in a Zephyr application.

It supports both `I2C` and `UART` communication with the Notecard as well as utilizing Zephyr native APIs for logging and error handling as well as memory management.

## Requirements

While this module is designed to be used with the Blues' Feather MCU boards, it can be used with any Zephyr compatible board. Overlays may need to be modified to work with other board device trees.

### Hardware

- Any [Blues Notecard](https://shop.blues.io/collections/notecard)
- [Blues Notecarrier F](https://shop.blues.com/products/notecarrier-f)
- Blues Feather MCU boards
  - [Swan](https://shop.blues.io/products/swan)
- [STLINK Programmer/Debugger](https://shop.blues.io/collections/accessories/products/stlink-v3mini)

### Software / Cloudware

- [Zephyr SDK](https://docs.zephyrproject.org/latest/getting_started)
- [west](https://docs.zephyrproject.org/latest/guides/west/install.html)
- [Notehub.io](https://notehub.io)

## Usage

The easiest way to use this module is to add it to your project as a module using `west`.
If you don't have `west` installed, you can install it by following the instructions on the [Zephyr Documentation](https://docs.zephyrproject.org/latest/guides/west/install.html).

### Option 1: Adding note-zephyr to your own project

To use this module in your Zephyr project, you need to add it to your manifest. For example, if you are managing your project with `west`, you can add it to your `west.yml` manifest like this:

```yaml
manifest:
  projects:
    - name: zephyr
      revision: main
      url: https://github.com/zephyrproject-rtos/zephyr
      import:
       name-allowlist:
          - hal_stm32
          - cmsis
    - name: note-zephyr
        path: modules/note-zephyr
        revision: main
        submodules: true
        url: https://github.com/blues/note-zephyr
    # your other modules here ...
```

> **Warning:** `submodules: true` is a REQUIRED option to for the `note-zephyr` module. This is because the `note-c` library is a submodule and dependency of the `note-zephyr` module.

You can then run `west update` to fetch the latest changes from the `note-zephyr` module.

To use it in your project, you will need to enable the module in your `prj.conf` file. For example:

```sh
CONFIG_NEWLIB_LIBC=y # Required by `note-c`
CONFIG_BLUES_NOTECARD=y
# Optional: Enable logging
CONFIG_BLUES_NOTECARD_LOGGING=y
```

> **Note:** The `CONFIG_NEWLIB_LIBC` option is required by `note-c` and must be enabled in your `prj.conf` along with the `CONFIG_BLUES_NOTECARD` option.

### Option 2: Initialize the repository for development

Once you have `west` installed, you can initialize the repository:

```sh
west init -m https://github.com/blues/note-zephyr --mr main my-workspace
cd my-workspace
west update
```

This will create a new workspace (`my-workspace`) with the `note-zephyr` project, ready to build.

## Getting started with examples

To get started with the examples, the easiest thing to do is to follow the instructions for [option 2](#option-2-initialize-the-repository-for-development) above. This will install the dependencies for Blues' Feather MCU boards and allow you to build the examples.

We're assuming you're using the `swan_r5` board and flashing using an [STLINK](https://shop.blues.io/collections/accessories/products/stlink-v3mini) programmer/debugger.

```sh
west build -b swan_r5 examples/blinky
west flash
```

### Notehub.io

To see your Notecard data in Notehub, you'll need to set up a free account (no credit card required) on [Notehub.io](https://notehub.io).
Once you have created youraccount, then you need to [create a project](https://dev.blues.io/quickstart/notecard-quickstart/notecard-simulator/#create-a-notehub-project) to serve as an endpoint for the Notes that are tracking the state of the LED.

Once you have a project, you will need to update the `define` named `PROJECT_UID` in your `main.c` with [the UID of the project you have just created](https://blues.dev/notehub/notehub-walkthrough/#finding-a-productuid).

After the Notecard has connected to Notehub, you can look inside the project and see a device named e.g. `zephyr-blink` (for the `blinky` example).
The Notecard will be running in `continuous` mode, which will allow it to maintain a constant cellular connection.
`continuous` mode offers the lowest latency possible for sending messages to Notehub, but it comes at the cost of battery life. Fortunately, this is typically not a concern while bench testing, because you are plugged into USB power.

For more information about the Notecard API and examples, please visit [our developer site](https://blues.dev).

## Development environments

### VSCode

See [docs/vscode.md](docs/vscode.md) for more information.

### Dev Container

See [docs/dev-container.md](docs/dev-container.md) for more information.