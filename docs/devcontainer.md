# Devcontainer

The devcontainer is a containerized development environment that is used to build and run the Zephyr application. It is configured using a `devcontainer.json` file, which is located in the `.devcontainer` directory.

## Configuration

Depending on your host machine, you may need to provide access to the USB controller of the host machine.

### Linux

To enable flashing and debugging from the container on Linux, you will need to
provide access to the USB controller of the host machine.

Perform the following steps, in order to provide USB access:

1. Open `./.devcontainer/devcontainer.json`.
2. Uncomment the `runArgs` section:

    ```json
    // Uncomment the following section if your host machine is running Linux
    "mounts": [
    {
      "type": "bind",
      "source": "/dev/bus/usb",
      "target": "/dev/bus/usb"
    }
    ```

### Windows / MacOS

Ensure Docker Desktop is running before building the dev container.

To enable flashing and debugging from the container on Windows or MacOS, you will need to start the `openocd` server on your host machine. For example:

```bash
openocd -f swan_r5.cfg
```

> **WARNING:** You should launch openocd from a terminal or outside process; _outside_ of VSCode as the IDE will restart upon opening the devcontainer.

## Launching the Devcontainer

To launch the dev container, either click the `Dev Container` launch configuration in the lower left corner of VSCode, or open the command palette and select `Dev Container: Reopen in Container`.

If you need to make changes to the dev container, you can open the command palette and select `Dev Container: Rebuild Container`.

> **NOTE:** Building the devcontainer will take a while, as it needs to download the Zephyr SDK and install the dependencies.