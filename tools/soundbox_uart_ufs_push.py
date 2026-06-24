#!/usr/bin/env python3
"""Provision EG800AK Soundbox U-drive files over factory serial JSON.

Important production note:
The original Vi_mp3 root prompts are valid MP3 files, but they are tagged
with ID3v2.2 and use 11.025/12 kHz sample rates. On EG800AK QuecOpen this
format can be readable from U-drive yet rejected by the MP3 playback API with
ret=-1. This tool therefore provisions known-good common prompts encoded as
16 kHz mono CBR MP3 without ID3 tags.
"""
from __future__ import annotations

import argparse
import binascii
import json
import shutil
import subprocess
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
BUNDLED_COMMON_DIR = Path(__file__).resolve().parent / "factory_assets" / "common_mp3_16k"


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
    send_command(ser, {"cmd": "file_begin", "path": target, "size": len(data), "crc32": crc}, dry_run)
    for offset in range(0, len(data), CHUNK_BYTES):
        chunk = data[offset:offset + CHUNK_BYTES]
        send_command(ser, {"cmd": "file_chunk", "offset": offset, "data_hex": chunk.hex()}, dry_run)
    send_command(ser, {"cmd": "file_end"}, dry_run)


def mp3_head(path: Path) -> bytes:
    with path.open("rb") as fp:
        return fp.read(4)


def copy_or_convert_common_mp3(source: Path, cache_dir: Path, use_bundled: bool) -> Path:
    target = cache_dir / source.name
    bundled = BUNDLED_COMMON_DIR / source.name

    if use_bundled and bundled.is_file():
        shutil.copy2(bundled, target)
        print(f"using bundled EG800AK-safe MP3 {bundled.name} size={target.stat().st_size}")
        return target

    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg:
        cmd = [
            ffmpeg, "-y", "-hide_banner", "-loglevel", "error",
            "-i", str(source),
            "-vn", "-ac", "1", "-ar", "16000",
            "-codec:a", "libmp3lame", "-b:a", "32k",
            "-write_id3v1", "0", "-id3v2_version", "0",
            str(target),
        ]
        subprocess.run(cmd, check=True)
        print(f"converted {source.name} -> 16k mono no-ID3 size={target.stat().st_size}")
        return target

    shutil.copy2(source, target)
    if mp3_head(target) == b"ID3":
        print(f"WARNING: {source.name} still has ID3 header and may fail on EG800AK MP3 decoder", file=sys.stderr)
    return target


def collect_files(assets: Path | None, cert_dir: Path | None, config: Path | None,
                  use_bundled_common: bool) -> list[tuple[Path, str]]:
    result: list[tuple[Path, str]] = []
    with tempfile.TemporaryDirectory(prefix="sb_ufs_push_") as td:
        cache_dir = Path(tempfile.mkdtemp(prefix="sb_ufs_files_"))
        if assets is not None:
            root = find_common_root(extract_if_needed(assets, Path(td)))
            for name in COMMON_FILES:
                prepared = copy_or_convert_common_mp3(root / name, cache_dir, use_bundled_common)
                result.append((prepared, f"U:/{name}"))
        if cert_dir is not None:
            for name, target in CERT_TARGETS.items():
                source = cert_dir / name
                if source.is_file():
                    dst = cache_dir / source.name
                    dst.write_bytes(source.read_bytes())
                    result.append((dst, target))
        if config is not None:
            dst = cache_dir / config.name
            dst.write_bytes(config.read_bytes())
            result.append((dst, "U:/config/soundbox_config.json"))
        return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Provision Soundbox U-drive files over serial factory commands.")
    parser.add_argument("--port", default="COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--assets", type=Path, help="Vi_mp3 folder or zip containing the four common root MP3 files")
    parser.add_argument("--cert-dir", type=Path, help="folder containing MQTT PEM files")
    parser.add_argument("--config", type=Path, help="JSON config file")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--no-bundled-common", action="store_true",
                        help="do not use bundled EG800AK-safe common MP3 prompts; convert source with ffmpeg if available")
    args = parser.parse_args()

    files = collect_files(args.assets, args.cert_dir, args.config, not args.no_bundled_common)
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
