#!/usr/bin/env python3
"""Send JSON requests straight to the Notecard over its own serial port.

Used to put the shared Notecard back the way it was found. Some examples change
persistent Notecard configuration -- outboard-dfu enables Outboard DFU, which
survives a reflash of the host -- and the Notestation is shared with other
repos' HIL jobs. This talks to the Notecard over the reservation's
notecard_usb device rather than through the host, so it works whatever
firmware the Swan is running, or if it is running nothing at all.

Exits 0 if every request got a response without an "err" field, 1 otherwise.
"""

from __future__ import annotations

import argparse
import json
import sys
import time

try:
    import serial
except ImportError:
    print(
        "error: pyserial is required (pip install pyserial)",
        file=sys.stderr,
    )
    sys.exit(1)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Notecard serial device.")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument(
        "--request",
        required=True,
        action="append",
        help="JSON request to send. Repeatable; sent in order.",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="Seconds to wait for each response.",
    )
    return parser.parse_args(argv)


def transact(port: serial.Serial, request: str, timeout: float) -> dict | None:
    port.reset_input_buffer()
    port.write(request.encode("utf-8") + b"\n")
    port.flush()

    # The Notecard can interleave trace output on its USB port, so skip
    # anything that is not a JSON object rather than taking the first line.
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if not line.startswith("{"):
            continue
        try:
            return json.loads(line)
        except json.JSONDecodeError:
            continue
    return None


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    for request in args.request:
        # Fail on a malformed request here, not as a confusing Notecard error.
        json.loads(request)

    ok = True
    try:
        with serial.Serial(args.port, args.baud, timeout=1) as port:
            # A bare newline resynchronises the Notecard if a previous user
            # left a partial request in its buffer.
            port.write(b"\n")
            port.flush()
            time.sleep(0.25)

            for request in args.request:
                print(f"> {request}", flush=True)
                response = transact(port, request, args.timeout)
                if response is None:
                    print(f"error: no response within {args.timeout}s", file=sys.stderr)
                    ok = False
                    continue
                print(f"< {json.dumps(response)}", flush=True)
                if "err" in response:
                    ok = False
    except serial.SerialException as exc:
        print(f"error: could not open {args.port}: {exc}", file=sys.stderr)
        return 1

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
