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

## Why the legacy USB device stack

The snippet sets `CONFIG_USB_DEVICE_STACK`, **not**
`CONFIG_USB_DEVICE_STACK_NEXT`, and that is load-bearing rather than
conservatism.

Zephyr gates new-stack support per board through the `usbd` entry in a board's
`supported:` list — upstream's new-stack CDC ACM sample carries
`depends_on: usbd`. At the pinned `v4.4.0`, `boards/blues/swan_r5/swan_r5.yaml`
lists `usb_device` and not `usbd`: the Swan claims the legacy stack only. The
same sample's `integration_platforms` name `stm32f723e_disco` and
`nucleo_f413zh` — F7 and F4 — and no STM32L4 at all, so the new stack has no
upstream coverage on this SoC family.

Nothing gates this at build time. A new-stack image for `swan_r5` configures,
compiles and links cleanly, then never enumerates. That failure is close to
invisible: the driver reports it with `LOG_ERR`, and the only console the board
has is the USB device that just failed to come up, so the explanation is written
to the thing that is broken. From CI it looks like a Notestation fault — the
`host_mcu_usb` symlink never appears — rather than a firmware one.

**Anything building this snippet must disable warnings-as-errors.** The legacy
stack is deprecated upstream in three ways, and the last one is not
suppressible:

- The Kconfig symbol `select DEPRECATED`, which only prints a warning while
  configuring. Harmless.
- The stack's C API is marked `__deprecated`, so Zephyr's own
  `subsys/usb/device/usb_device.c` trips `-Wdeprecated-declarations` against
  itself. That one *is* demotable, with `-Wno-error=deprecated-declarations`.
- Its macros are marked with Zephyr's `__DEPRECATED_MACRO`, which expands to a
  bare `#pragma GCC warning`. GCC attaches **no `-W` category** to those
  diagnostics — they print as `error: Macro is deprecated [-Werror]` with
  nothing after the `-Werror` — so no `-Wno-error=` can target them.

Under twister's default `-Werror` that last class fails the build outright, in
Zephyr's sources rather than ours. So `.github/workflows/hil-tests.yml` passes
`-W` to the twister build, and the local command in
[`docs/hil-testing.md`](../../docs/hil-testing.md) does the same. The trade-off
is real — warnings in our own test sources no longer fail that build — and it
goes away when the Swan can use the new stack.

When `swan_r5` gains `usbd` in its
`supported:` list, switch this file to `CONFIG_USB_DEVICE_STACK_NEXT` and drop
this section — the devicetree overlay needs no change, because both stacks
consume the same `zephyr,cdc-acm-uart` binding.

## Boot delay

`CONFIG_BOOT_DELAY=3000` is deliberate. The USB console vanishes while the board
is flashed and returns only after re-enumeration, so a host reading it can
otherwise miss the beginning of the output — including a ztest banner, which
leaves twister waiting for something already gone.
