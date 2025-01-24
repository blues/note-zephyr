# Dev Container

## Building the Dev Container

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