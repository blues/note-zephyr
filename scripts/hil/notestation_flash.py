#!/usr/bin/env python3
"""Flash a twister-built Zephyr image onto a Notestation-hosted host MCU.

Twister calls this through its ``--flash-command`` hook, which passes
``--build-dir <dir>`` and, when a hardware map is in use, ``--board-id <id>``.
We ignore the board id: the Notestation reservation already identifies the
hardware, and with ``--device-serial`` (rather than a hardware map) twister has
no id to pass anyway.

Flashing goes over GDB to the OpenOCD instance the Notestation runs for the
reservation, reached on the ``gdb_port`` from ``reservation.json``. This is
deliberately *not* ``notestation-client flash``: that syncs the ELF to the
Notestation over SSH first, which needs an SSH credential a CI runner does not
have. Talking to the GDB port needs nothing beyond the Tailnet connection the
reservation already required. It is the same approach note-c's HIL workflow
takes.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

# Printed by GDB after a successful `load`. Its absence means nothing was
# written, even when GDB itself exits cleanly.
LOAD_SUCCESS_MARKER = "Transfer rate"


def find_gdb() -> str | None:
    """Locate an ARM-capable GDB.

    A plain `gdb` cannot debug an ARM target, so use the one the Zephyr SDK
    ships. It is normally on PATH after the SDK is set up; fall back to walking
    the SDK directory when it is not.
    """
    found = shutil.which("arm-zephyr-eabi-gdb")
    if found:
        return found

    # action-zephyr-setup neither puts the toolchain on PATH nor exports
    # ZEPHYR_SDK_INSTALL_DIR, so look where SDKs actually land.
    roots: list[Path] = []

    sdk = os.environ.get("ZEPHYR_SDK_INSTALL_DIR")
    if sdk:
        roots.append(Path(sdk))

    # action-zephyr-setup extracts to <base-path>/zephyr-sdk, and base-path
    # defaults to the workspace root.
    workspace = os.environ.get("GITHUB_WORKSPACE")
    if workspace:
        roots.append(Path(workspace) / "zephyr-sdk")
    roots.append(Path.cwd() / "zephyr-sdk")

    # A locally installed SDK registers itself as a CMake package; the file
    # holds the SDK's cmake/ directory, whose parent is the SDK root.
    registry = Path.home() / ".cmake" / "packages" / "Zephyr-sdk"
    if registry.is_dir():
        for entry in sorted(registry.iterdir(), reverse=True):
            try:
                roots.append(Path(entry.read_text(encoding="utf-8").strip()).parent)
            except OSError:
                continue

    for pattern in ("zephyr-sdk-*", ".local/opt/zephyr-sdk-*"):
        roots.extend(sorted(Path.home().glob(pattern), reverse=True))
    for parent in ("/opt/toolchains", "/opt"):
        roots.extend(sorted(Path(parent).glob("zephyr-sdk-*"), reverse=True))

    # SDK 0.17 puts the toolchain at <root>/arm-zephyr-eabi/bin; SDK 1.x moved
    # it under a gnu/ directory. Try both rather than pinning to either.
    subpaths = (
        Path("arm-zephyr-eabi") / "bin" / "arm-zephyr-eabi-gdb",
        Path("gnu") / "arm-zephyr-eabi" / "bin" / "arm-zephyr-eabi-gdb",
    )

    for root in roots:
        for subpath in subpaths:
            candidate = root / subpath
            if candidate.is_file():
                return str(candidate)

    print(
        "no arm-zephyr-eabi-gdb found under: "
        + ", ".join(str(r) for r in roots),
        file=sys.stderr,
    )
    return None


def read_gdb_target(reservation_dir: str) -> tuple[str, int]:
    """Return (hostname, gdb_port) for the reservation's host MCU debug server."""
    path = Path(reservation_dir) / "reservation.json"
    with path.open(encoding="utf-8") as handle:
        reservation = json.load(handle)

    hostname = reservation.get("hostname")
    if not hostname:
        raise ValueError(f"{path} has no 'hostname'")

    for server in reservation.get("debug_servers") or []:
        if server.get("target") == "host_mcu":
            port = server.get("gdb_port")
            if not port:
                raise ValueError(f"{path} host_mcu debug server has no 'gdb_port'")
            return hostname, int(port)

    raise ValueError(
        f"{path} exposes no host_mcu debug server -- the reservation needs a "
        "Notestation with an MCU debug probe (tag mcu_debugger)"
    )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        required=True,
        help="Build directory; the image is at <build-dir>/zephyr/zephyr.elf.",
    )
    parser.add_argument(
        "--board-id",
        default=None,
        help="Passed by twister when a hardware map is used. Accepted and ignored.",
    )
    parser.add_argument(
        "--reservation-dir",
        default=os.environ.get("NS_RESERVATION_DIR", ""),
        help="Reservation directory holding reservation.json. Defaults to "
        "NS_RESERVATION_DIR, exported by the reserve_notestation action.",
    )
    parser.add_argument(
        "--gdb",
        default=None,
        help="GDB binary to use. Defaults to arm-zephyr-eabi-gdb from the Zephyr SDK.",
    )
    parser.add_argument(
        "--wait-for-port",
        default=os.environ.get("CONSOLE_PORT", ""),
        help="Console device to wait for after flashing. Defaults to "
        "CONSOLE_PORT. Set empty to skip waiting.",
    )
    parser.add_argument(
        "--wait-timeout",
        type=float,
        default=60.0,
        help="Seconds to wait for the console device to come back.",
    )
    return parser.parse_args(argv)


def wait_for_port(path: str, timeout: float) -> bool:
    """Block until `path` exists and can be opened.

    The console is USB CDC ACM, so flashing takes the device down and the
    reservation's symlink only returns once the host MCU has re-enumerated.
    Twister opens the port with a single, un-retried ``serial.Serial()``
    immediately after this script exits, so returning early fails the run
    outright with "Serial Device Error". Existence alone is not enough -- the
    symlink can be back before the endpoint accepts an open.
    """
    deadline = time.monotonic() + timeout
    last_error: OSError | None = None

    while time.monotonic() < deadline:
        if os.path.exists(path):
            try:
                fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
            except OSError as exc:
                last_error = exc
            else:
                os.close(fd)
                return True
        time.sleep(0.5)

    if last_error is not None:
        print(
            f"error: {path} reappeared but could not be opened: {last_error}",
            file=sys.stderr,
        )
    else:
        print(
            f"error: {path} did not reappear within {timeout}s of flashing",
            file=sys.stderr,
        )
    return False


def flash_over_gdb(gdb: str, elf: Path, hostname: str, port: int) -> bool:
    command = [
        gdb,
        "--batch",
        "--nx",
        "-ex", "set confirm off",
        "-ex", "set pagination off",
        "-ex", f"target extended-remote {hostname}:{port}",
        "-ex", "monitor reset halt",
        "-ex", "load",
        "-ex", "monitor reset run",
        "-ex", "detach",
        str(elf),
    ]

    print(f"Flashing {elf} via GDB to {hostname}:{port}", flush=True)

    result = subprocess.run(command, capture_output=True, text=True)
    output = result.stdout + result.stderr
    print(output, flush=True)

    if result.returncode != 0:
        print(f"error: gdb exited {result.returncode}", file=sys.stderr)
        return False

    # GDB can exit 0 having failed to write anything, so require the evidence
    # that a load actually happened.
    if LOAD_SUCCESS_MARKER not in output:
        print(
            f"error: gdb exited cleanly but never reported "
            f"{LOAD_SUCCESS_MARKER!r}, so nothing was written",
            file=sys.stderr,
        )
        return False

    return True


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    elf = Path(args.build_dir) / "zephyr" / "zephyr.elf"
    if not elf.is_file():
        print(f"error: no image to flash at {elf}", file=sys.stderr)
        return 1

    gdb = args.gdb or find_gdb()
    if gdb is None:
        print(
            "error: no arm-zephyr-eabi-gdb on PATH and ZEPHYR_SDK_INSTALL_DIR "
            "did not contain one. A plain gdb cannot flash an ARM target.",
            file=sys.stderr,
        )
        return 1

    if not args.reservation_dir:
        print(
            "error: no reservation directory. NS_RESERVATION_DIR was empty and "
            "--reservation-dir was not given -- has the reservation been made?",
            file=sys.stderr,
        )
        return 1

    try:
        hostname, port = read_gdb_target(args.reservation_dir)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if not flash_over_gdb(gdb, elf, hostname, port):
        return 1

    if args.wait_for_port:
        print(f"Waiting for {args.wait_for_port} to come back", flush=True)
        if not wait_for_port(args.wait_for_port, args.wait_timeout):
            return 1
        print("Console device is back", flush=True)

    return 0


if __name__ == "__main__":
    sys.exit(main())
