#!/usr/bin/env python3
"""Read a serial console until an expected string appears.

Used for the example smoke test: flash an example, then prove the board booted
and got far enough to talk to the Notecard. Twister does this itself for the
ztest suite, but the examples are applications rather than test binaries, so
they need something simpler.

Exits 0 on a match, 1 on timeout, and 2 if a failure pattern shows up first --
so a board that boots and *then* logs an error fails fast instead of burning
the whole timeout.
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError:
    print(
        "error: pyserial is required (pip install pyserial)",
        file=sys.stderr,
    )
    sys.exit(1)

# Substrings that mean the run is already lost. The Zephyr log prefixes errors
# with <err>, and the examples log their own failures via LOG_ERR.
DEFAULT_FAILURE_PATTERNS = ["<err>", "Failed to", "BUS FAULT", "***** "]


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Serial device to read.")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--expect",
        required=True,
        action="append",
        help="Substring that must appear. Repeatable; all must be seen, in any order.",
    )
    parser.add_argument(
        "--fail-on",
        action="append",
        default=None,
        help=f"Substring that fails the run immediately. Defaults to {DEFAULT_FAILURE_PATTERNS}.",
    )
    parser.add_argument("--timeout", type=float, default=90.0)
    parser.add_argument(
        "--wait-for-port",
        type=float,
        default=0.0,
        help="Seconds to wait for the port to appear before reading. The "
        "console is USB CDC ACM, so it is absent while the board is flashed "
        "and returns only once USB has re-enumerated.",
    )
    parser.add_argument(
        "--log",
        type=Path,
        default=None,
        help="Write everything read to this file as well as stdout.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    failure_patterns = (
        args.fail_on if args.fail_on is not None else DEFAULT_FAILURE_PATTERNS
    )
    outstanding = list(args.expect)

    log_handle = args.log.open("w", encoding="utf-8") if args.log else None

    def emit(line: str) -> None:
        print(line, flush=True)
        if log_handle:
            log_handle.write(line + "\n")
            log_handle.flush()

    if args.wait_for_port > 0:
        appear_deadline = time.monotonic() + args.wait_for_port
        while time.monotonic() < appear_deadline and not os.path.exists(args.port):
            time.sleep(0.5)
        if not os.path.exists(args.port):
            print(
                f"error: {args.port} did not appear within {args.wait_for_port}s",
                file=sys.stderr,
            )
            return 1

    try:
        # A short read timeout keeps the deadline check responsive on a quiet
        # port; the overall budget is enforced by the loop, not by pyserial.
        with serial.Serial(args.port, args.baud, timeout=1) as port:
            emit(f"--- reading {args.port} at {args.baud} baud ---")
            deadline = time.monotonic() + args.timeout

            while time.monotonic() < deadline:
                raw = port.readline()
                if not raw:
                    continue

                line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                emit(line)

                for pattern in failure_patterns:
                    if pattern in line:
                        emit(f"--- failure pattern {pattern!r} seen ---")
                        return 2

                outstanding = [p for p in outstanding if p not in line]
                if not outstanding:
                    emit("--- all expected output seen ---")
                    return 0

            emit(
                f"--- timed out after {args.timeout}s still waiting for "
                f"{outstanding!r} ---"
            )
            return 1
    except serial.SerialException as exc:
        print(f"error: could not read {args.port}: {exc}", file=sys.stderr)
        return 1
    finally:
        if log_handle:
            log_handle.close()


if __name__ == "__main__":
    sys.exit(main())
