# Docs

To simplify the process of developing Zephyr applications, we support two workflows, `native` and `devcontainer`. These are both designed to be used within VSCode.

## Native

The `native` workflow allows you to develop Zephyr applications on your host machine, managed by VSCode.

This workflow requires you to bring your own development environment, and is suitable for users who are familiar with Zephyr and the underlying hardware. You will need to install the Zephyr SDK, West, and OpenOCD.

## Devcontainer

The `devcontainer` workflow allows you to develop Zephyr applications within a containerized environment, managed by VSCode. To read more about the devcontainer, please see the [Devcontainer](./devcontainer.md) section.

This workflow is suitable for users who are new to Zephyr, or who do not want to install the Zephyr SDK, West, and OpenOCD on their host machine.

The draw back of this workflow is that there are limitations for features that are not supported in the devcontainer. This includes debugging and flashing the application via USB, under Windows and MacOS. For these users, we provide tools that connect to the devcontainer via USB over a network connection. See the [Debugging](./debugging.md) and [Flashing](./development.md#flashing-external) sections for more information.