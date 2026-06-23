#!/usr/bin/env python3
"""Send SBAS asset pack to Soundbox over USB CDC UART factory interface.

Fast/safe mode sends one JSON command per acknowledged chunk. The firmware
accepts up to 1024 raw bytes per hex chunk in this package.
"""
from __future__ import annotations

import argparse
import binascii
import json
import sys
import time
from pathlib import Path
from typing import Any

try:
    import serial
except ImportError as exc:
    raise SystemExit("pyserial is required: python -m pip install pyserial") from exc


def _status_ok(reply: str) -> bool:
    try:
        obj: dict[str, Any] = json.loads(reply)
    except json.JSONDecodeError:
        return False
    return str(obj.get("status", "")).lower() == "ok"


def send_line(port: serial.Serial, obj: dict[str, object], wait_s: float, retries: int) -> str:
    payload = json.dumps(obj, separators=(",", ":")).encode("ascii") + b"\r\n"
    last_reply = ""
    for attempt in range(retries + 1):
        port.reset_input_buffer()
        port.write(payload)
        port.flush()
        if wait_s > 0.0:
            time.sleep(wait_s)
        reply = port.readline().decode("utf-8", errors="replace").strip()
        last_reply = reply
        if reply and _status_ok(reply):
            return reply
        if attempt < retries:
            time.sleep(0.20 + (0.10 * attempt))
    if last_reply == "":
        raise RuntimeError(
            "no reply from device. Check that firmware is built with serial asset provisioning enabled, "
            "the correct COM port is selected, and USB CDC is not held by another terminal"
        )
    raise RuntimeError(f"command failed after {retries + 1} attempt(s): {last_reply!r}")


def push(port_name: str, baud: int, pack: Path, chunk_size: int, wait_s: float, timeout_s: float, retries: int) -> None:
    data = pack.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    started = time.monotonic()
    print(f"pack={pack} size={len(data)} crc32=0x{crc:08x} chunk={chunk_size} baud={baud}")

    with serial.Serial(port_name, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s) as port:
        send_line(port, {"cmd": "asset_begin", "size": len(data), "crc32": crc, "erase": 1}, wait_s, retries)
        offset = 0
        last_report = 0.0
        while offset < len(data):
            chunk = data[offset:offset + chunk_size]
            reply = send_line(port, {"cmd": "asset_chunk", "offset": offset, "data_hex": chunk.hex()}, wait_s, retries)
            offset += len(chunk)
            now = time.monotonic()
            if (now - last_report) >= 1.0 or offset >= len(data):
                elapsed = now - started
                speed = offset / elapsed if elapsed > 0 else 0.0
                remain = (len(data) - offset) / speed if speed > 0 else 0.0
                print(f"offset={offset}/{len(data)} {offset * 100.0 / len(data):5.1f}% speed={speed / 1024.0:7.1f} KiB/s eta={remain:6.1f}s reply={reply}")
                last_report = now
        send_line(port, {"cmd": "asset_end"}, wait_s, retries)
        status = send_line(port, {"cmd": "asset_status"}, wait_s, retries)
        print(status)


def main() -> None:
    parser = argparse.ArgumentParser(description="Push SBAS audio pack over UART/USB CDC")
    parser.add_argument("port", help="serial port, for example COM16 or /dev/ttyUSB0")
    parser.add_argument("pack", type=Path, help="SBAS pack file")
    parser.add_argument("--baud", type=int, default=921600, help="host serial baud; USB CDC usually ignores this but accepts it")
    parser.add_argument("--chunk", type=int, default=512, choices=range(64, 1025), metavar="64..1024")
    parser.add_argument("--wait", type=float, default=0.0, help="optional delay after each command before reading reply")
    parser.add_argument("--timeout", type=float, default=20.0, help="serial read/write timeout in seconds")
    parser.add_argument("--retries", type=int, default=3, help="retry count per command/chunk")
    args = parser.parse_args()

    if not args.pack.exists():
        raise SystemExit(f"pack file not found: {args.pack}")
    if args.chunk > 1024:
        raise SystemExit("chunk must be <= 1024 for this firmware package")

    try:
        push(args.port, args.baud, args.pack.resolve(), args.chunk, args.wait, args.timeout, args.retries)
    except KeyboardInterrupt:
        raise SystemExit("aborted by user")
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2) from exc


if __name__ == "__main__":
    main()
