#!/usr/bin/env python3
"""Pack EG800AK Soundbox language MP3 assets for W25Q64 external NOR."""
from __future__ import annotations

import argparse
import csv
import os
import shutil
import subprocess
import struct
import sys
import tempfile
import zipfile
from pathlib import Path
from zlib import crc32

MAGIC = 0x53424D46
VERSION = 1
HEADER_SIZE = 28
PATH_LEN = 96
ENTRY_SIZE = 112
ASSETS_BASE_ADDR = 0x00030000
MAX_NOR_SIZE = 8 * 1024 * 1024
LANGUAGES = ("bn", "en", "gu", "hi", "kn", "ma", "ml", "pa", "ta", "tl")
COMMON_FILES = {"start_tune.mp3", "ping.mp3", "good_bye.mp3", "transaction_error.mp3"}
NORMALIZED_MP3_RATE = "16000"
NORMALIZED_MP3_BITRATE = "32k"


def align4(value: int) -> int:
    return (value + 3) & ~3


def extract_input(input_path: Path, work_dir: Path) -> Path:
    if input_path.is_dir():
        return input_path
    if not zipfile.is_zipfile(input_path):
        raise SystemExit(f"input is neither folder nor zip: {input_path}")
    with zipfile.ZipFile(input_path, "r") as zf:
        zf.extractall(work_dir)
    return work_dir


def find_asset_root(path: Path) -> Path:
    candidates = []
    for p in [path] + [x for x in path.rglob("*") if x.is_dir()]:
        found = sum(1 for lang in LANGUAGES if (p / lang).is_dir())
        if found >= 5:
            candidates.append((found, p))
    if not candidates:
        raise SystemExit("could not find language folders in input")
    candidates.sort(key=lambda item: (-item[0], len(str(item[1]))))
    return candidates[0][1]


def normalize_mp3_if_requested(source: Path, cache_root: Path, normalize: bool) -> Path:
    if not normalize:
        return source
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise SystemExit("--normalize-mp3 requires ffmpeg in PATH; install ffmpeg or pass --no-normalize-mp3")
    rel_name = source.name
    target = cache_root / rel_name
    target.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        ffmpeg, "-y", "-hide_banner", "-loglevel", "error",
        "-i", str(source),
        "-vn", "-ac", "1", "-ar", NORMALIZED_MP3_RATE,
        "-codec:a", "libmp3lame", "-b:a", NORMALIZED_MP3_BITRATE,
        "-write_id3v1", "0", "-id3v2_version", "0",
        str(target),
    ]
    subprocess.run(cmd, check=True)
    return target


def make_entry(logical_path: str, offset: int, data: bytes, language: str) -> bytes:
    path_bytes = logical_path.encode("utf-8")
    if len(path_bytes) >= PATH_LEN:
        raise SystemExit(f"path too long for manifest: {logical_path}")
    lang_bytes = language.encode("ascii")[:3]
    return struct.pack(
        "<96sIII4s",
        path_bytes + b"\0" * (PATH_LEN - len(path_bytes)),
        offset,
        len(data),
        crc32(data) & 0xFFFFFFFF,
        lang_bytes + b"\0" * (4 - len(lang_bytes)),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Pack Soundbox language MP3 files into an external NOR image.")
    parser.add_argument("--input", required=True, help="Vi_mp3 folder or zip")
    parser.add_argument("--out-dir", required=True, help="output folder")
    parser.add_argument("--image-name", default="soundbox_extnor_audio.bin")
    parser.add_argument("--no-normalize-mp3", action="store_true", help="pack original MP3 files without 16 kHz/no-ID3 normalization")
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="sb_asset_pack_") as td:
        root = find_asset_root(extract_input(input_path, Path(td)))
        normalized_cache = Path(td) / "normalized_mp3"
        normalize_mp3 = not args.no_normalize_mp3
        entries = []
        data_chunks = []
        current = ASSETS_BASE_ADDR
        report_rows = []
        missing_langs = []

        for lang in LANGUAGES:
            lang_dir = root / lang
            if not lang_dir.is_dir():
                missing_langs.append(lang)
                continue
            files = sorted(p for p in lang_dir.iterdir() if p.is_file() and p.suffix.lower() == ".mp3")
            for file_path in files:
                packed_path = normalize_mp3_if_requested(file_path, normalized_cache / lang, normalize_mp3)
                data = packed_path.read_bytes()
                logical = f"audio/{lang}/{file_path.name}"
                current = align4(current)
                entries.append(make_entry(logical, current, data, lang))
                data_chunks.append((current, data, logical, lang, file_path.name))
                report_rows.append([lang, file_path.name, logical, f"0x{current:08X}", len(data), f"0x{crc32(data) & 0xFFFFFFFF:08X}"])
                current += len(data)

        if current > MAX_NOR_SIZE:
            raise SystemExit(f"asset image exceeds W25Q64 size: {current} bytes")
        entries_blob = b"".join(entries)
        header = struct.pack(
            "<IIIIIII",
            MAGIC,
            VERSION,
            HEADER_SIZE,
            ENTRY_SIZE,
            len(entries),
            ASSETS_BASE_ADDR,
            crc32(entries_blob) & 0xFFFFFFFF,
        )
        manifest_blob = header + entries_blob
        if len(manifest_blob) > ASSETS_BASE_ADDR:
            raise SystemExit("manifest overlaps asset base address")

        image = bytearray(b"\xFF" * current)
        image[0:len(manifest_blob)] = manifest_blob
        for offset, data, _logical, _lang, _name in data_chunks:
            image[offset:offset + len(data)] = data

        image_path = out_dir / args.image_name
        image_path.write_bytes(image)
        (out_dir / "soundbox_audio_manifest.bin").write_bytes(manifest_blob)

        with (out_dir / "soundbox_audio_asset_report.csv").open("w", newline="", encoding="utf-8") as fp:
            writer = csv.writer(fp)
            writer.writerow(["language", "filename", "logical_path", "nor_offset", "length", "crc32"])
            writer.writerows(report_rows)

        with (out_dir / "soundbox_audio_language_report.txt").open("w", encoding="utf-8") as fp:
            fp.write(f"asset_root={root}\n")
            fp.write(f"entries={len(entries)}\n")
            fp.write(f"image_bytes={len(image)}\n")
            fp.write(f"manifest_crc32=0x{crc32(entries_blob) & 0xFFFFFFFF:08X}\n")
            fp.write(f"normalized_mp3={normalize_mp3} rate={NORMALIZED_MP3_RATE} bitrate={NORMALIZED_MP3_BITRATE}\n")
            fp.write("missing_language_folders=" + ",".join(missing_langs) + "\n")
            for lang in LANGUAGES:
                lang_dir = root / lang
                if lang_dir.is_dir():
                    count = len([p for p in lang_dir.iterdir() if p.is_file() and p.suffix.lower() == ".mp3"])
                    fp.write(f"{lang}={count}\n")

    print(f"image={image_path}")
    print(f"entries={len(entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
