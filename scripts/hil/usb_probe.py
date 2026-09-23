#!/usr/bin/env python3
"""Read the Swan's USB and clock registers over SWD, to find out why it is not
enumerating.

The HIL console is the board's own USB CDC ACM device, so when USB fails to
come up the board has no way to say why -- ``usb_dc_stm32`` reports the failure
with ``LOG_ERR`` to a console that does not exist. This reaches around that by
reading the peripheral registers directly through the OpenOCD GDB server the
reservation already exposes, which needs no firmware change and no extra port.

Halting the core to read memory is what wedges a Notestation, so this always
resets the target back to running afterwards, including on failure.

Usage:
    usb_probe.py --reservation-dir "$NS_RESERVATION_DIR" [--elf path/to.elf]
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys

from notestation_flash import find_gdb, openocd_reset_run, read_debug_server

# STM32L4R5 peripheral registers. Values from RM0432.
REGISTERS: list[tuple[str, int]] = [
    ("PWR_CR2",   0x40007004),  # bit 10 USV: VDDUSB supply valid
    ("RCC_CRRCR", 0x40021098),  # bit 0 HSI48ON, bit 1 HSI48RDY
    ("RCC_CCIPR", 0x40021088),  # bits 27:26 CLK48SEL
    ("OTG_GOTGCTL", 0x50000000),
    ("OTG_GAHBCFG", 0x50000008),
    ("OTG_GCCFG",   0x50000038),  # bit 16 PWRDWN, bit 21 VBDEN
    ("OTG_DCFG",    0x50000800),
    ("OTG_DCTL",    0x50000804),  # bit 1 SDIS: soft disconnect
    ("OTG_DSTS",    0x50000808),
]


def decode(name: str, value: int) -> str:
    """Return a human-readable note for the bits that decide enumeration."""
    if name == "PWR_CR2":
        return "VDDUSB " + ("ENABLED" if value & (1 << 10) else "DISABLED <-- USB cannot work")
    if name == "RCC_CRRCR":
        on = "on" if value & 1 else "OFF"
        ready = "ready" if value & (1 << 1) else "NOT READY"
        flag = "" if (value & 0b11) == 0b11 else "  <-- HSI48 is the USB 48MHz source"
        return f"HSI48 {on}, {ready}{flag}"
    if name == "RCC_CCIPR":
        sel = (value >> 26) & 0b11
        return f"CLK48SEL={sel} ({['HSI48', 'PLLSAI1', 'PLL', 'MSI'][sel]})"
    if name == "OTG_GCCFG":
        pwrdwn = "powered" if value & (1 << 16) else "POWERED DOWN <-- transceiver off"
        vbden = "VBUS-sensing on" if value & (1 << 21) else "VBUS-sensing off"
        return f"transceiver {pwrdwn}, {vbden}"
    if name == "OTG_DCTL":
        return "pull-up " + ("SOFT-DISCONNECTED <-- host cannot see the device"
                             if value & (1 << 1) else "connected")
    if name == "OTG_DSTS":
        return f"suspended={bool(value & 1)}, enumerated_speed={(value >> 1) & 0b11}"
    return ""


def probe(gdb: str, hostname: str, port: int, elf: str | None) -> bool:
    command = [
        gdb, "--batch", "--nx",
        "-ex", "set confirm off",
        "-ex", "set pagination off",
        "-ex", f"target extended-remote {hostname}:{port}",
        # Do not reset: the point is to see the state the running firmware left
        # the peripheral in.
        "-ex", "monitor halt",
        "-ex", "info registers pc",
    ]
    for name, addr in REGISTERS:
        command += ["-ex", f'printf "REG {name} = 0x%08x\\n", *(unsigned int *){addr:#x}']
    command += ["-ex", "detach"]
    if elf:
        command.append(elf)

    print(f"Probing USB registers via GDB at {hostname}:{port}", flush=True)
    result = subprocess.run(command, capture_output=True, text=True)
    output = result.stdout + result.stderr
    print(output, flush=True)

    values = dict(re.findall(r"REG (\S+) = (0x[0-9a-fA-F]+)", output))
    if not values:
        print("error: no registers were read; the probe tells us nothing",
              file=sys.stderr)
        return False

    print("\n--- decoded ---", flush=True)
    for name, _ in REGISTERS:
        raw = values.get(name)
        if raw is None:
            continue
        note = decode(name, int(raw, 16))
        print(f"  {name:<12} {raw}  {note}", flush=True)
    return True


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reservation-dir", required=True)
    parser.add_argument("--elf", default=None,
                        help="optional ELF, so GDB can symbolise the PC")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    gdb = find_gdb()
    if not gdb:
        print("error: no arm-zephyr-eabi-gdb on PATH", file=sys.stderr)
        return 1

    hostname, gdb_port, telnet_port = read_debug_server(args.reservation_dir)

    try:
        ok = probe(gdb, hostname, gdb_port, args.elf)
    finally:
        # Always leave the core running, whatever the probe did. A halted Swan
        # presents no USB device, which makes the station unreservable for
        # everyone -- see docs/hil-testing.md.
        openocd_reset_run(hostname, telnet_port)

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
