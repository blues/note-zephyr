# Devcontainer

The devcontainer is a containerized development environment that is used to build and run the Zephyr application. It is configured using a `devcontainer.json` file, which is located in the `.devcontainer` directory.

## The pre-built image

`devcontainer.json` points at `ghcr.io/blues/note-zephyr-devcontainer`, which is
built from `.devcontainer/Dockerfile` and published by the
[Build Devcontainer Image](../.github/workflows/devcontainer.yml) workflow for
both `linux/amd64` and `linux/arm64`.

That image already contains the `west` workspace at `/workdir` - Zephyr, the
allow-listed HAL modules, the Python requirements and the exported CMake
package. Only `/workdir/note-zephyr` is bind mounted from your checkout, so
starting the container is a one-off image pull rather than a full `west update`.
`.devcontainer/onCreateCommand.sh` still runs a `west update` afterwards, which
reconciles the baked workspace with your `west.yml` if you have changed it.

The image is rebuilt when `west.yml` or the `Dockerfile` change, and weekly,
because `west.yml` tracks Zephyr's `main` branch. To pick up a newer image, pull
it and then run `Dev Containers: Rebuild Container`:

```bash
docker pull ghcr.io/blues/note-zephyr-devcontainer:latest
```

If you would rather build the image locally - for example while changing the
`Dockerfile` - replace the `image` key in `devcontainer.json` with:

```json
"build": { "dockerfile": "Dockerfile", "context": ".." }
```

## Configuration

Depending on your host machine, you may need to provide access to the USB controller of the host machine.

### Linux

To enable flashing and debugging from the container on Linux, you will need to
provide access to the USB controller of the host machine.

Perform the following steps, in order to provide USB access:

1. Open `./.devcontainer/devcontainer.json`.
2. Add a `mounts` section:

    ```json
    "mounts": [
      {
        "type": "bind",
        "source": "/dev/bus/usb",
        "target": "/dev/bus/usb"
      }
    ],
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

> **NOTE:** The first launch downloads the pre-built image, which is several
> gigabytes and will take a while. Subsequent launches reuse the cached image.
