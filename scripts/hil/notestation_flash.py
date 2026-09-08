#!/usr/bin/env python3
"""Flash a twister-built Zephyr image onto a Notestation-hosted host MCU.

Twister calls this through its ``--flash-command`` hook, which passes
``--build-dir <dir>`` and, when a hardware map is in use, ``--board-id <id>``.
We ignore the board id: the Notestation reservation already identifies the
hardware, and with ``--device-serial`` (rather than a hardware map) twister has
no id to pass anyway.

``notestation-client flash`` auto-detects the target from the file's signature
bytes. A Zephyr image carries neither the Notecard nor the Starnote magic, so it
is treated as host MCU firmware and flashed over SWD via OpenOCD -- which is why
this must hand it the ``.elf`` and not the ``.bin``: host MCU flashing only
accepts ``.elf``/``.out``.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        required=True,
        help="Twister build directory; the image is at <build-dir>/zephyr/zephyr.elf.",
    )
    parser.add_argument(
        "--board-id",
        default=None,
        help="Passed by twister when a hardware map is used. Accepted and ignored.",
    )
    parser.add_argument(
        "--notestation",
        default=os.environ.get("NS_HOSTNAME", ""),
        help="Notestation hostname. Defaults to NS_HOSTNAME, exported by the "
        "reserve_notestation action.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if shutil.which("notestation-client") is None:
        print(
            "error: notestation-client is not on PATH. Install it with the "
            "install_notestation_client action, or pass a 'version' to "
            "reserve_notestation.",
            file=sys.stderr,
        )
        return 1

    if not args.notestation:
        print(
            "error: no Notestation hostname. NS_HOSTNAME was empty and "
            "--notestation was not given -- has the reservation been made?",
            file=sys.stderr,
        )
        return 1

    elf = Path(args.build_dir) / "zephyr" / "zephyr.elf"
    if not elf.is_file():
        print(f"error: no image to flash at {elf}", file=sys.stderr)
        return 1

    command = [
        "notestation-client",
        "flash",
        "--notestation",
        args.notestation,
        "--file",
        str(elf),
    ]

    print(f"Flashing {elf} to {args.notestation}", flush=True)

    # Stream output rather than capturing it: when a flash fails, OpenOCD's
    # complaint is the only useful thing in the twister log.
    result = subprocess.run(command, check=False)
    if result.returncode != 0:
        print(
            f"error: notestation-client flash exited {result.returncode}",
            file=sys.stderr,
        )

    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
