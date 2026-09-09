# `notestation` snippet

Builds firmware whose console is reachable from a [Notestation][ns].

[ns]: https://github.com/blues/notestation

Most Zephyr boards, `swan_r5` included, default `zephyr,console` to a hardware
UART — `lpuart1` on the Swan's Feather header. The Notestations do not wire that
UART through: a reservation exposes `host_mcu_usb`, the MCU's *native* USB, and
nothing for `lpuart1`. Firmware built the default way therefore says nothing
that CI can read.

This snippet moves both the console and the shell onto a USB CDC ACM device, so
the board's output arrives on the device the Notestation actually tunnels.

## Usage

```bash
west build -b swan_r5 -S notestation examples/blinky
```

In a twister `testcase.yaml`:

```yaml
tests:
  my.test:
    required_snippets:
      - notestation
```

The snippet is provided by this module via `snippet_root` in
`zephyr/module.yml`, so it is available to any application in a west workspace
that includes note-zephyr — no path arguments needed.

## Why the console interface matters beyond output

A Notestation tunnel publishes its symlink only while the attached device is
presenting that interface. Firmware with no USB device means no `host_mcu_usb`,
which older versions of
[`reserve_notestation`](https://github.com/blues/notestation-actions) treat as a
hard failure — so a board flashed with a UART-console build could make the whole
station unreservable, including for other repositories' jobs. Using this snippet
keeps a USB device present across reflashes.

## Boot delay

`CONFIG_BOOT_DELAY=3000` is deliberate. The USB console vanishes while the board
is flashed and returns only after re-enumeration, so a host reading it can
otherwise miss the beginning of the output — including a ztest banner, which
leaves twister waiting for something already gone.
