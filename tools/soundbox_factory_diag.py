#!/usr/bin/env python3
"""Send factory diagnostic commands to EG800AK Soundbox firmware."""
from __future__ import annotations

import argparse
import json
import sys

COMMANDS = {
    "fs_self_test": {"cmd":"diag", "test":"fs_self_test"},
    "nor_id": {"cmd":"diag", "test":"nor_id"},
    "nor_rw_test": {"cmd":"diag", "test":"nor_rw_test"},
    "list_assets": {"cmd":"diag", "test":"list_assets"},
    "key_test": {"cmd":"diag", "test":"key_test"},
    "led_test": {"cmd":"diag", "test":"led_test"},
    "mqtt_test": {"cmd":"diag", "test":"mqtt_test"},
    "http_test": {"cmd":"diag", "test":"http_test"},
    "cert_check": {"cmd":"diag", "test":"cert_check"},
}


def import_serial(port_name: str, baud: int):
    try:
        import serial  # type: ignore
    except Exception as exc:
        raise SystemExit("pyserial is required for serial diagnostics. Use --dry-run to print command.") from exc
    return serial.Serial(port_name, baudrate=baud, timeout=10)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Soundbox factory diagnostics.")
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("command", choices=sorted(list(COMMANDS) + ["play_common", "play_lang", "ufs_file"]))
    parser.add_argument("args", nargs="*")
    args = parser.parse_args()

    if args.command == "play_common":
        if len(args.args) != 1:
            raise SystemExit("play_common requires filename")
        obj = {"cmd":"diag", "test":"play_common", "file":args.args[0]}
    elif args.command == "ufs_file":
        if len(args.args) != 1:
            raise SystemExit("ufs_file requires filename, for example start_tune.mp3")
        obj = {"cmd":"diag", "test":"ufs_file", "file":args.args[0]}
    elif args.command == "play_lang":
        if len(args.args) != 2:
            raise SystemExit("play_lang requires language and filename")
        obj = {"cmd":"diag", "test":"play_lang", "language":args.args[0], "file":args.args[1]}
    else:
        obj = COMMANDS[args.command]

    line = json.dumps(obj, separators=(",", ":")) + "\n"
    if args.dry_run:
        print(line.rstrip())
        return 0
    ser = import_serial(args.port, args.baud)
    try:
        ser.write(line.encode("utf-8"))
        ser.flush()
        while True:
            reply = ser.readline().decode("utf-8", errors="replace").strip()
            if not reply:
                break
            print(reply)
            if reply.startswith("{"):
                break
    finally:
        ser.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
