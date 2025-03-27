# note-zephyr

This is a Zephyr `west` module for the Blues Notecard.
It is an abstraction layer for the [note-c](https://github.com/blues/note-c) library, that allows you to easily use a [Notecard](https://blues.com/products/notecard/) in a Zephyr application.

It supports both `I2C` and `UART` communication with the Notecard as well as utilizing Zephyr native APIs for logging and error handling as well as memory management.

## Requirements

While this module is designed to be used with the Blues' Feather MCU boards, it can be used with any Zephyr compatible board. Overlays may need to be modified to work with other board device trees.

### Hardware

- Any [Blues Notecard](https://shop.blues.io/collections/notecard)
- [Blues Notecarrier F](https://shop.blues.com/products/notecarrier-f)
- Blues Feather MCU boards
  - [Swan](https://shop.blues.io/products/swan)
- [STLINK Programmer/Debugger](https://shop.blues.io/collections/accessories/products/stlink-v3mini)

### Software

- [Zephyr SDK](https://docs.zephyrproject.org/latest/getting_started) (at least 3.7 LTS)
- [west](https://docs.zephyrproject.org/latest/guides/west/install.html)

### Cloudware

- [Notehub.io](https://notehub.io)

## Usage

The easiest way to use this module is to add it to your project as a module using `west`.
If you don't have `west` installed, you can install it by following the instructions on the [Zephyr Documentation](https://docs.zephyrproject.org/latest/guides/west/install.html).

### Adding note-zephyr module to your project

To use this module in your Zephyr project, you will need to add it to your manifest.
For example, if you are managing your project with `west`, you can add it to your `west.yml` manifest:

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

> **Warning:** `submodules: true` is a REQUIRED option for the `note-zephyr` module. This is because the `note-c` library is a submodule and dependency of the `note-zephyr` module.

You can then run `west update` to fetch the latest changes from the `note-zephyr` module.

You will also need to enable the module in your `prj.conf` file.
For example:

```sh
# Required by `note-c`
CONFIG_NEWLIB_LIBC=y
CONFIG_BLUES_NOTECARD=y
# Optional: Enable logging
CONFIG_BLUES_NOTECARD_LOGGING=y

```

> **Note:** The `CONFIG_NEWLIB_LIBC` option is required by `note-c` and must be enabled in your `prj.conf` along with the `CONFIG_BLUES_NOTECARD` option.

## Getting started with examples

To try the [examples](examples/README.md) standalone, without importing `note-zephyr` as a module, the easiest thing to do is to follow the instructions for [development](#development) below. This will install the dependencies for Blues' Feather MCU boards and allow you to build the examples.

We're assuming you're using the `swan_r5` board and flashing using an [STLINK](https://shop.blues.io/collections/accessories/products/stlink-v3mini) programmer/debugger.

```sh
# from the note-zephyr directory
west build -b swan_r5 examples/blinky
west flash
```

### Notehub.io

To see your Notecard data in Notehub, you'll need to set up a free account (no credit card required) on [Notehub.io](https://notehub.io).
Once you have created your account, then you need to [create a project](https://dev.blues.io/quickstart/notecard-quickstart/notecard-simulator/#create-a-notehub-project) to serve as an endpoint for the Notes that are tracking the state of the LED.

Once you have a project, you will need to update the `define` named `PROJECT_UID` in your `main.c` with [the UID of the project you have just created](https://blues.dev/notehub/notehub-walkthrough/#finding-a-productuid).

After the Notecard has connected to Notehub, you can look inside the project and see a device named e.g. `zephyr-blink` (for the `blinky` example).
The Notecard will be running in `continuous` mode, which will allow it to maintain a constant cellular connection.
`continuous` mode offers the lowest latency possible for sending messages to Notehub, but it comes at the cost of battery life. Fortunately, this is typically not a concern while bench testing, because you are plugged into USB power.

For more information about the Notecard API and examples, please visit [our developer site](https://blues.dev).

## Development

To initialize the repository for development, you can use the following commands:

```sh
west init -m https://github.com/blues/note-zephyr --mr main my-workspace
cd my-workspace
west update
```

This will create a new workspace (`my-workspace`) with the `note-zephyr` project, ready to build.

### VSCode

You can also use VSCode with or without a devcontainer to develop with this module.
`git clone` the repository and open the directory in VSCode.

Opening the directory in VSCode, you should see a popup asking if you want to open the folder in a devcontainer.
Click `Open in Container`.

You can then build an example application by opening the command palette and selecting `Zephyr: Build` and specifying the board you want to build for, e.g. `swan_r5`, then the example you want to build, e.g. `examples/blinky`.

See the [docs](docs/README.md) for more information about the devcontainer.

## Starting a new project

To start a new project, create a new directory, e.g. `app`, in the root of the repository and add the following files:

```bash
app
├── CMakeLists.txt
├── README.md
├── prj.conf
└── src
    └── main.c
```

You may wish to copy across the files from the `examples/blinky` directory to get started.

You can then build the application by opening the command palette and selecting `Zephyr: Build` and specifying the board you want to build for, e.g. `swan_r5`, then your project directory, e.g. `app`.
