#!/usr/bin/env python3
"""
Push essential demo MP3 prompts to EG800AK Soundbox U:/audio over MAIN UART.

Default behavior transfers only the essential prompts needed by the current demo
logs. Use --all to transfer every file from the zip/folder.

Examples:
  python tools/soundbox_uart_ufs_push.py COM16 audio.zip --chunk 512 --baud 921600
  python tools/soundbox_uart_ufs_push.py COM16 audio.zip --all --chunk 512 --baud 921600
"""
from __future__ import annotations

import argparse
import binascii
import json
import pathlib
import time
import zipfile

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover
    raise SystemExit("pyserial is required. Install with: python -m pip install pyserial") from exc

COMMON_ESSENTIAL = {
    "start_tune.mp3",
    "ping.mp3",
    "good_bye.mp3",
    "transaction_error.mp3",
}

LANG_ESSENTIAL = {
    "no_SIM.mp3",
    "internet.mp3",
    "no_mqtt.mp3",
    "no_internet.mp3",
    "no_transactions.mp3",
    "battery_low.mp3",
    "prefix.mp3",
    "rupees.mp3",
    "paise.mp3",
    "thankyou.mp3",
    "googlepay.mp3",
    "paytm.mp3",
    "phonepe.mp3",
    "provider.mp3",
}


def _normalize_zip_name(name: str) -> str:
    clean = name.replace("\\", "/").lstrip("/")
    while clean.startswith("./"):
        clean = clean[2:]
    if not clean.startswith("audio/"):
        clean = "audio/" + clean
    return clean


def _target_path_for_source(logical_path: str, language: str, essential_only: bool) -> str | None:
    basename = logical_path.rsplit("/", 1)[-1]
    if not essential_only:
        return logical_path
    if basename in COMMON_ESSENTIAL:
        return f"audio/{basename}"
    if basename in LANG_ESSENTIAL:
        return f"audio/{language}/{basename}"
    return None


def iter_files(source: pathlib.Path, language: str, essential_only: bool):
    seen: set[str] = set()
    if source.is_dir():
        for path in sorted(source.rglob("*")):
            if not path.is_file():
                continue
            rel = path.relative_to(source).as_posix()
            logical = _normalize_zip_name(rel)
            target = _target_path_for_source(logical, language, essential_only)
            if target is None or target in seen:
                continue
            seen.add(target)
            yield target, path.read_bytes()
        return

    if zipfile.is_zipfile(source):
        with zipfile.ZipFile(source, "r") as zf:
            for name in sorted(n for n in zf.namelist() if not n.endswith("/")):
                logical = _normalize_zip_name(name)
                if logical.startswith("audio/__MACOSX/") or logical.endswith(".DS_Store"):
                    continue
                target = _target_path_for_source(logical, language, essential_only)
                if target is None or target in seen:
                    continue
                seen.add(target)
                yield target, zf.read(name)
        return

    logical = _normalize_zip_name(source.name)
    target = _target_path_for_source(logical, language, essential_only)
    if target is not None:
        yield target, source.read_bytes()


def read_reply(port: serial.Serial, timeout: float) -> dict:
    deadline = time.time() + timeout
    line = bytearray()
    while time.time() < deadline:
        b = port.read(1)
        if not b:
            continue
        if b in (b"\r", b"\n"):
            if line:
                text = line.decode("utf-8", errors="replace").strip()
                try:
                    return json.loads(text)
                except json.JSONDecodeError:
                    line.clear()
                    continue
        else:
            line.extend(b)
    raise TimeoutError("no reply from device")


def send_json(port: serial.Serial, obj: dict, timeout: float, retries: int) -> dict:
    data = (json.dumps(obj, separators=(",", ":")) + "\r\n").encode("ascii")
    last_error: Exception | None = None
    for _attempt in range(retries + 1):
        port.reset_input_buffer()
        port.write(data)
        port.flush()
        try:
            reply = read_reply(port, timeout)
            if reply.get("status") == "ok":
                return reply
            raise RuntimeError(f"device rejected command: {reply}")
        except Exception as exc:  # noqa: BLE001
            last_error = exc
            time.sleep(0.2)
    raise RuntimeError(str(last_error) if last_error else "command failed")


def push_file(port: serial.Serial, logical_path: str, payload: bytes, chunk: int, timeout: float, retries: int) -> None:
    crc = binascii.crc32(payload) & 0xFFFFFFFF
    print(f"file={logical_path} size={len(payload)} crc32=0x{crc:08x}")
    send_json(port, {"cmd": "file_begin", "path": logical_path, "size": len(payload), "crc32": crc}, timeout, retries)
    offset = 0
    total = len(payload)
    start = time.time()
    while offset < total:
        block = payload[offset:offset + chunk]
        reply = send_json(port, {"cmd": "file_chunk", "offset": offset, "data_hex": block.hex()}, timeout, retries)
        written = int(reply.get("written", len(block)))
        if written != len(block):
            raise RuntimeError(f"short write at offset {offset}: written={written}, expected={len(block)}")
        offset += len(block)
    send_json(port, {"cmd": "file_end"}, timeout, retries)
    elapsed = max(time.time() - start, 0.001)
    print(f"  done {total} bytes {total / elapsed / 1024.0:.1f} KiB/s")


def main() -> int:
    parser = argparse.ArgumentParser(description="Push essential audio files to EG800AK U:/audio over MAIN UART")
    parser.add_argument("port", help="COM port, for example COM16")
    parser.add_argument("source", type=pathlib.Path, help="audio.zip, audio folder, or a single mp3 file")
    parser.add_argument("--chunk", type=int, default=512, help="raw binary bytes per file_chunk command, max 1024")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--retries", type=int, default=3)
    parser.add_argument("--language", default="en", help="language folder for language prompts, default en")
    parser.add_argument("--all", action="store_true", help="transfer all files instead of only essential demo prompts")
    args = parser.parse_args()

    if args.chunk <= 0 or args.chunk > 1024:
        raise SystemExit("--chunk must be 1..1024")
    if not args.source.exists():
        raise SystemExit(f"source not found: {args.source}")

    essential_only = not args.all
    files = list(iter_files(args.source, args.language, essential_only))
    if not files:
        raise SystemExit("no files selected for transfer")
    total_bytes = sum(len(data) for _, data in files)
    mode = "essential" if essential_only else "all"
    print(f"source={args.source} mode={mode} files={len(files)} total={total_bytes} bytes port={args.port} baud={args.baud} chunk={args.chunk}")
    if total_bytes > 220 * 1024:
        print("WARNING: selected files are close to U: capacity. Use essential mode or delete old files first.")

    with serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=args.timeout) as port:
        time.sleep(0.5)
        for logical_path, payload in files:
            push_file(port, logical_path, payload, args.chunk, args.timeout, args.retries)
    print("DONE")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
