## Notecard Outboard Firmware Updates with MCUBoot

This MCUBoot example is based on the `zephyr/samples/basic/blinky` example from the Nordic Connect SDK (NCS).

This example has been configured and tested on Adafruit nRF52840 Feather Express, although it
is expected to work with other Nordic devices with only minor changes necessary.

> **Note**: The examples here use the default MCUBoot keys for signing. We strongly recommend that you create your own keys for signing firmware.

### Board support

The Adafruit Feather nRF52840 (both **Express** and **Sense**) is a supported upstream
Zephyr board (`adafruit_feather_nrf52840`), so you can build for either. On recent NCS the
Sense target is `adafruit_feather_nrf52840/nrf52840/sense` (Express:
`adafruit_feather_nrf52840`).

> **NCS version note:** the configuration in this folder uses the older `child_image/`
> mechanism. Recent NCS (≥ 2.7) uses **sysbuild**; on those toolchains the MCUboot
> configuration moves to `sysbuild/mcuboot/` (or `sysbuild.conf`). The board target and
> Notecard configuration below are unchanged.

### Project Configuration

These are the steps you take to add MCUBoot support to an existing NCS application:

1. Add `CONFIG_BOOTLOADER_MCUBOOT=y` to your project's `prj.conf` file. This enables MCUBoot for the project.

2. Copy the contents of `child_image` to a `child_image` directory in your project. You may need to create this directory if it is not already present. This folder contains configuration for MCUBoot.

3. DFU mode is typically triggered via a boot pin that MCUBoot checks shortly after device reset. In this example, we used the Feather's D2 pin, which is GPIO 0, Port 10. The boot pin must be pulled up and active low. The boot pin is defined in `child_image/mcuboot.overlay`:

```
    buttonDFU: button_dfu {
        gpios = <&gpio0 10 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
        label = "DFU activation button";
    };
```

If you wish to use a different pin to activate the bootloader, edit `gpio0 10` to correspond with the GPIO peripheral and port of your chosen pin.

We recommend that you use a boot pin if an unused GPIO is available and is connected to Notecard's AUX3 pin. If you wish to not use a boot pin, please see  [MCUBoot Configuration Options](#mcuboot-configuration-options) below.

# Building and Flashing the Example

1. Use `west build` and `west flash` to rebuild and upload the project to the device.

2. When the device is reset with the boot pin held low, MCUBoot will start in serial recovery mode. This is indicated via the board's LED, which is ON while MCUBoot is active.

3. You can use the `mcumgr` tool to inspect and test image updates via MCUBoot. Here's a brief outline of how to install and use the tool. You can read more about the tool in the Zephyr project [mcumgr page](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html).

    * If not already installed, install `mcumgr`. This assumes `go` version >= 1.18 is installed.
    ```
      go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest
      export PATH=$PATH:~/go/bin
    ```

    * Add the connection to `mcumgr`
    ```
    export MCUMGR_DEV=<USB-serial-device>
    mcumgr conn add acm0 type="serial" connstring="dev=$MCUMGR_DEV,baud=115200,mtu=512"
    ```

    * Update the application on the device via MCUBoot:
    ```
    mcumgr -c acm0 image upload build/zephyr/app_update.bin
    ```

## MCUBoot Configuration Options

MCUBoot configuration options are found in [`child_image/mcuboot.conf`](child_image/mcuboot.conf). The majority of them cannot be changed and are required for MCUBoot to operate correctly with Notecard.

These options can be changed:

* `CONFIG_MCUBOOT_INDICATION_LED`: Enables the on-board LED, which is ON when the device is performing a DFU update. You may wish to set this to `n` to conserve power.

* `CONFIG_BOOT_SERIAL_WAIT_FOR_DFU`: Configures the bootloader to wait for a valid request before starting the application. When a valid request is received, the device remains in bootloader mode. This is an alternative to using a boot pin. This is only necessary when AUX3 is not connected to a boot pin on the target device.

## Using a Custom Signing Key

MCUBoot ships with default keys that are used for signing the firmware during development. These are adequate for testing, but should not be used for firmware you plan to deploy.

We will use `imgtool` that ships with the Nordic Connect SDK to generate a new key pair.

1. Change directory to the `fast` directory and generate a new key using `imgtool`.
  ```
  imgtool keygen -k child_image/priv.pem -t rsa-2048
  ```

2. Edit `CMakeLists.txt` and uncomment the line by removing the preceding `#` comment marker.
  ```
  SET(mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE "\"${CMAKE_SOURCE_DIR}/child_image/priv.pem\"")
  ```

3. Rebuild the app.
  ```
  west build -p
  ```

4. Verify that the built app uses the custom private key.
  ```
  imgtool verify -k child_image/priv.pem build/zephyr/app_update.bin
  ```

5. Change directory to the `slow` version of the app.
  ```
  cd ../slow
  ```

6. Copy the contents of `child_image` from the `fast` directory to the `slow` directory so both versions use the same key and configuration.
  ```
  cp ../fast/child_image/* child_image/
  ```

7. Edit `CMakeLists.txt` and uncomment the line by removing the preceding `#` comment marker.
  ```
  SET(mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE "\"${CMAKE_SOURCE_DIR}/child_image/priv.pem\"")
  ```

8. Rebuild the app.
  ```
  west build -p
  ```

9. Verify that the built app uses the custom private key.
  ```
  imgtool verify -k child_image/priv.pem build/zephyr/app_update.bin
  ```

10.  Flash the app and bootloader to your device.
  ```
  west flash
  ```

Your device is now running the `slow` version of the app. To test upgrading, we will use Notecard Outboard Firmware Updates to update to the `fast` version.

## Using MCUBoot DFU with Notecard

MCUboot support was added in Notecard firmware version 5.3.1. Ensure that your Notecard firmware is at least this version.

### Notecard Configuration

In a terminal emulator, or the blues [In-Browser Terminal](https://dev.blues.io/terminal/), run these commands to enable Outboard DFU with MCUBoot:

```
  {"req":"card.aux", "mode":"dfu"}
```

```
  {"req":"card.dfu", "name":"mcuboot"}
```

Test an upgrade to the `fast` version of the app:

* The app binary is located at `fast/build/zephyr/app_update.bin`. Follow the [Uploading Firmware to Notehub](https://dev.blues.io/guides-and-tutorials/notecard-guides/notecard-outboard-firmware-update/#uploading-firmware-to-notehub) tutorial to upload this file to Notehub so that it is available for download to your device.
* With the firmware uploaded, schedule a Device Firmware Update from Notehub.


# Resources

* [NCS - Adding an immutable bootloader](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/app_dev/bootloaders_and_dfu/bootloader_adding.html#id11)
* [`mcumgr`@Zephyr](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html)
* [Adding Custom Signing Keys](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/app_dev/bootloaders_and_dfu/fw_update.html#id6)