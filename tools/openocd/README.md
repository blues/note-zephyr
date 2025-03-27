# OpenOCD

Configuration files here are provided for running a standalone OpenOCD server. This can be used for flashing and debugging the Swan R5 from within the VSCode devcontainer. By default OpenOCD uses port 4444 for telnet and 3333 for GDB.

You should start openocd from the root of the project.

## Usage

```bash
openocd -f tools/openocd/swan_r5.cfg
```