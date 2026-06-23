#!/usr/bin/env python3
"""Send SBAS asset pack to Soundbox over Main UART factory interface.

The transfer uses one acknowledged JSON command per chunk. The firmware accepts
up to 1024 raw bytes per hex chunk and writes them to external NOR with
page-safe 256-byte programming internally.
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


def _parse_reply(reply: str) -> dict[str, Any]:
    try:
        obj = json.loads(reply)
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"invalid JSON reply: {reply!r}") from exc
    if not isinstance(obj, dict):
        raise RuntimeError(f"unexpected reply type: {reply!r}")
    return obj


def _status_ok_obj(reply: str) -> dict[str, Any] | None:
    try:
        obj = _parse_reply(reply)
    except RuntimeError:
        return None
    if str(obj.get("status", "")).lower() == "ok":
        return obj
    return None


def send_line(port: serial.Serial, obj: dict[str, object], wait_s: float, retries: int) -> dict[str, Any]:
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
        ok_obj = _status_ok_obj(reply) if reply else None
        if ok_obj is not None:
            return ok_obj
        if attempt < retries:
            time.sleep(0.20 + (0.10 * attempt))
    if last_reply == "":
        raise RuntimeError(
            "no reply from device. Check Main UART wiring TX/RX/GND, correct COM port, "
            "921600 baud, and that another terminal is not holding the port"
        )
    raise RuntimeError(f"command failed after {retries + 1} attempt(s): {last_reply!r}")


def read_status(port: serial.Serial, wait_s: float, retries: int) -> dict[str, Any]:
    return send_line(port, {"cmd": "asset_status"}, wait_s, retries)


def push(port_name: str, baud: int, pack: Path, chunk_size: int, wait_s: float, timeout_s: float, retries: int) -> None:
    data = pack.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    started = time.monotonic()
    print(f"pack={pack} size={len(data)} crc32=0x{crc:08x} chunk={chunk_size} baud={baud}")

    with serial.Serial(port_name, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s) as port:
        begin = send_line(port, {"cmd": "asset_begin", "size": len(data), "crc32": crc, "erase": 1}, wait_s, retries)
        print(f"begin={json.dumps(begin, separators=(',', ':'))}")
        offset = 0
        last_report = 0.0
        while offset < len(data):
            chunk = data[offset:offset + chunk_size]
            reply = send_line(port, {"cmd": "asset_chunk", "offset": offset, "data_hex": chunk.hex()}, wait_s, retries)
            written = int(reply.get("written", -1))
            if written != len(chunk):
                raise RuntimeError(f"short write at offset {offset}: expected {len(chunk)} got {written} reply={reply}")
            offset += len(chunk)
            now = time.monotonic()
            if (now - last_report) >= 1.0 or offset >= len(data):
                elapsed = now - started
                speed = offset / elapsed if elapsed > 0 else 0.0
                remain = (len(data) - offset) / speed if speed > 0 else 0.0
                print(f"offset={offset}/{len(data)} {offset * 100.0 / len(data):5.1f}% speed={speed / 1024.0:7.1f} KiB/s eta={remain:6.1f}s")
                last_report = now
        pre_end = read_status(port, wait_s, retries)
        print(f"pre_end_status={json.dumps(pre_end, separators=(',', ':'))}")
        end = send_line(port, {"cmd": "asset_end"}, wait_s, retries)
        print(f"end={json.dumps(end, separators=(',', ':'))}")
        status = read_status(port, wait_s, retries)
        print(f"status={json.dumps(status, separators=(',', ':'))}")


def status_only(port_name: str, baud: int, wait_s: float, timeout_s: float, retries: int) -> None:
    with serial.Serial(port_name, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s) as port:
        status = read_status(port, wait_s, retries)
        print(json.dumps(status, separators=(",", ":")))


def main() -> None:
    parser = argparse.ArgumentParser(description="Push SBAS audio pack over Main UART")
    parser.add_argument("port", help="serial port, for example COM16 or /dev/ttyUSB0")
    parser.add_argument("pack", nargs="?", type=Path, help="SBAS pack file")
    parser.add_argument("--baud", type=int, default=921600, help="host serial baud")
    parser.add_argument("--chunk", type=int, default=512, choices=range(64, 1025), metavar="64..1024")
    parser.add_argument("--wait", type=float, default=0.0, help="optional delay after each command before reading reply")
    parser.add_argument("--timeout", type=float, default=20.0, help="serial read/write timeout in seconds")
    parser.add_argument("--retries", type=int, default=3, help="retry count per command/chunk")
    parser.add_argument("--status-only", action="store_true", help="read asset loader status and exit")
    args = parser.parse_args()

    try:
        if args.status_only:
            status_only(args.port, args.baud, args.wait, args.timeout, args.retries)
            return
        if args.pack is None:
            raise SystemExit("pack file is required unless --status-only is used")
        if not args.pack.exists():
            raise SystemExit(f"pack file not found: {args.pack}")
        if args.chunk > 1024:
            raise SystemExit("chunk must be <= 1024 for this firmware package")
        push(args.port, args.baud, args.pack.resolve(), args.chunk, args.wait, args.timeout, args.retries)
    except KeyboardInterrupt:
        raise SystemExit("aborted by user")
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2) from exc


if __name__ == "__main__":
    main()
