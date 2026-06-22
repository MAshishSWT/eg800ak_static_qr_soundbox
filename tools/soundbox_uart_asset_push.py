#!/usr/bin/env python3
"""Send SBAS asset pack to Soundbox over USB CDC UART factory interface."""
from __future__ import annotations

import argparse
import binascii
import json
import time
from pathlib import Path

try:
    import serial
except ImportError as exc:
    raise SystemExit("pyserial is required: python -m pip install pyserial") from exc


def send_line(port: serial.Serial, obj: dict[str, object], wait_s: float) -> str:
    payload = json.dumps(obj, separators=(",", ":")).encode("ascii") + b"\r\n"
    port.write(payload)
    port.flush()
    time.sleep(wait_s)
    reply = port.readline().decode("utf-8", errors="replace").strip()
    print(reply)
    return reply


def push(port_name: str, baud: int, pack: Path, chunk_size: int, wait_s: float) -> None:
    data = pack.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    with serial.Serial(port_name, baudrate=baud, timeout=10) as port:
        send_line(port, {"cmd": "asset_begin", "size": len(data), "crc32": crc, "erase": 1}, wait_s)
        offset = 0
        while offset < len(data):
            chunk = data[offset:offset + chunk_size]
            send_line(port, {"cmd": "asset_chunk", "offset": offset, "data_hex": chunk.hex()}, wait_s)
            offset += len(chunk)
            print(f"offset={offset}/{len(data)}")
        send_line(port, {"cmd": "asset_end"}, wait_s)
        send_line(port, {"cmd": "asset_status"}, wait_s)


def main() -> None:
    parser = argparse.ArgumentParser(description="Push SBAS audio pack over UART")
    parser.add_argument("port", help="serial port, for example COM12 or /dev/ttyUSB0")
    parser.add_argument("pack", type=Path, help="SBAS pack file")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--chunk", type=int, default=128, choices=range(16, 161), metavar="16..160")
    parser.add_argument("--wait", type=float, default=0.05, help="delay after each command")
    args = parser.parse_args()
    push(args.port, args.baud, args.pack.resolve(), args.chunk, args.wait)


if __name__ == "__main__":
    main()
