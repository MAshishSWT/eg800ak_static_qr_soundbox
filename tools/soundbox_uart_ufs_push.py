#!/usr/bin/env python3
"""Provision EG800AK Soundbox U-drive common assets, certificates, and config over factory serial JSON."""
from __future__ import annotations

import argparse
import binascii
import json
import sys
import time
import tempfile
import zipfile
from pathlib import Path

COMMON_FILES = ("start_tune.mp3", "ping.mp3", "good_bye.mp3", "transaction_error.mp3")
CERT_TARGETS = {
    "mqtt_root_ca.pem": "U:/certs/mqtt_root_ca.pem",
    "mqtt_client.crt": "U:/certs/mqtt_client.crt",
    "mqtt_client.key": "U:/certs/mqtt_client.key",
}
CHUNK_BYTES = 256


def import_serial(port_name: str, baud: int):
    try:
        import serial  # type: ignore
    except Exception as exc:
        raise SystemExit("pyserial is required for serial transfer. Use --dry-run for planning.") from exc
    return serial.Serial(port_name, baudrate=baud, timeout=5)


def extract_if_needed(path: Path, work: Path) -> Path:
    if path.is_dir():
        return path
    if not zipfile.is_zipfile(path):
        raise SystemExit(f"assets input is not folder or zip: {path}")
    with zipfile.ZipFile(path, "r") as zf:
        zf.extractall(work)
    return work


def find_common_root(path: Path) -> Path:
    for p in [path] + [x for x in path.rglob("*") if x.is_dir()]:
        if all((p / name).is_file() for name in COMMON_FILES):
            return p
    raise SystemExit("could not find all four common MP3 files")


def command(obj: dict) -> bytes:
    return (json.dumps(obj, separators=(",", ":")) + "\n").encode("utf-8")


def send_command(ser, obj: dict, dry_run: bool) -> None:
    line = command(obj)
    if dry_run:
        print(line.decode("utf-8").rstrip())
        return
    ser.write(line)
    ser.flush()
    reply = ser.readline().decode("utf-8", errors="replace").strip()
    if reply:
        print(reply)
    if '"status":"ok"' not in reply and '"status":"queued"' not in reply:
        raise SystemExit(f"device rejected command: {reply}")


def push_file(ser, local: Path, target: str, dry_run: bool) -> None:
    data = local.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    send_command(ser, {"cmd":"file_begin", "path":target, "size":len(data), "crc32":crc}, dry_run)
    for offset in range(0, len(data), CHUNK_BYTES):
        chunk = data[offset:offset + CHUNK_BYTES]
        send_command(ser, {"cmd":"file_chunk", "offset":offset, "data_hex":chunk.hex()}, dry_run)
    send_command(ser, {"cmd":"file_end"}, dry_run)


def collect_files(assets: Path | None, cert_dir: Path | None, config: Path | None) -> list[tuple[Path, str]]:
    result: list[tuple[Path, str]] = []
    with tempfile.TemporaryDirectory(prefix="sb_ufs_push_") as td:
        if assets is not None:
            root = find_common_root(extract_if_needed(assets, Path(td)))
            for name in COMMON_FILES:
                result.append((root / name, f"U:/{name}"))
        if cert_dir is not None:
            for name, target in CERT_TARGETS.items():
                source = cert_dir / name
                if source.is_file():
                    result.append((source, target))
        if config is not None:
            result.append((config, "U:/config/soundbox_config.json"))
        # Copy paths out of temp by returning original Path objects only after reads in main would be unsafe for zip.
        materialized=[]
        cache_dir=Path(tempfile.mkdtemp(prefix="sb_ufs_files_"))
        for source,target in result:
            dst=cache_dir/source.name
            dst.write_bytes(source.read_bytes())
            materialized.append((dst,target))
        return materialized


def main() -> int:
    parser = argparse.ArgumentParser(description="Provision Soundbox U-drive files over serial factory commands.")
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--assets", type=Path, help="Vi_mp3 folder or zip containing the four common root MP3 files")
    parser.add_argument("--cert-dir", type=Path, help="folder containing MQTT PEM files")
    parser.add_argument("--config", type=Path, help="JSON config file")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    files = collect_files(args.assets, args.cert_dir, args.config)
    if not files:
        raise SystemExit("no files selected")
    ser = None if args.dry_run else import_serial(args.port, args.baud)
    try:
        for local, target in files:
            print(f"push {local} -> {target}")
            push_file(ser, local, target, args.dry_run)
            time.sleep(0.05)
    finally:
        if ser is not None:
            ser.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
