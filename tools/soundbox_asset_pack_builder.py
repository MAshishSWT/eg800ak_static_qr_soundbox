#!/usr/bin/env python3
"""Build Static QR Soundbox SBAS audio asset pack.

Input folder format expected from Vi_mp3.zip:
  Vi_mp3/start_tune.mp3
  Vi_mp3/ping.mp3
  Vi_mp3/en/1.mp3
  Vi_mp3/en/no_SIM.mp3

Output logical paths:
  audio/start_tune.mp3
  audio/en/1.mp3
  audio/en/no_SIM.mp3
"""
from __future__ import annotations

import argparse
import binascii
import struct
from dataclasses import dataclass
from pathlib import Path

MAGIC = 0x53424153  # SBAS
VERSION = 1
HEADER_BYTES = 12
ENTRY_BYTES = 112
PATH_BYTES = 96


@dataclass(frozen=True)
class Asset:
    logical_path: str
    source_path: Path
    length: int
    crc32: int


def logical_path(root: Path, path: Path) -> str:
    rel = path.relative_to(root).as_posix()
    return f"audio/{rel}"


def collect_assets(root: Path) -> list[Asset]:
    assets: list[Asset] = []
    for path in sorted(root.rglob("*.mp3")):
        data = path.read_bytes()
        assets.append(
            Asset(
                logical_path=logical_path(root, path),
                source_path=path,
                length=len(data),
                crc32=binascii.crc32(data) & 0xFFFFFFFF,
            )
        )
    if not assets:
        raise SystemExit(f"no mp3 files found under {root}")
    return assets


def build_pack(root: Path, output: Path, manifest: Path | None) -> None:
    assets = collect_assets(root)
    index_bytes = len(assets) * ENTRY_BYTES
    data_offset = HEADER_BYTES + index_bytes
    offsets: list[int] = []
    cursor = data_offset
    for asset in assets:
        if len(asset.logical_path.encode("utf-8")) >= PATH_BYTES:
            raise SystemExit(f"path too long for index: {asset.logical_path}")
        offsets.append(cursor)
        cursor += asset.length

    with output.open("wb") as out:
        out.write(struct.pack("<III", MAGIC, VERSION, len(assets)))
        for asset, offset in zip(assets, offsets):
            path_bytes = asset.logical_path.encode("utf-8")
            path_field = path_bytes + bytes(PATH_BYTES - len(path_bytes))
            out.write(path_field)
            out.write(struct.pack("<III", offset, asset.length, asset.crc32))
            out.write(bytes(ENTRY_BYTES - PATH_BYTES - 12))
        for asset in assets:
            out.write(asset.source_path.read_bytes())

    pack_crc = binascii.crc32(output.read_bytes()) & 0xFFFFFFFF
    if manifest is not None:
        with manifest.open("w", encoding="utf-8") as mf:
            mf.write("logical_path,offset,length,crc32\n")
            for asset, offset in zip(assets, offsets):
                mf.write(f"{asset.logical_path},{offset},{asset.length},{asset.crc32:08x}\n")
            mf.write(f"PACK_SIZE,{output.stat().st_size}\n")
            mf.write(f"PACK_CRC32,{pack_crc:08x}\n")
    print(f"assets={len(assets)}")
    print(f"size={output.stat().st_size}")
    print(f"crc32={pack_crc:08x}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Build SBAS audio asset pack")
    parser.add_argument("input", type=Path, help="Vi_mp3 root folder")
    parser.add_argument("output", type=Path, help="output .sbas file")
    parser.add_argument("--manifest", type=Path, default=None, help="optional CSV manifest path")
    args = parser.parse_args()
    build_pack(args.input.resolve(), args.output.resolve(), args.manifest.resolve() if args.manifest else None)


if __name__ == "__main__":
    main()
