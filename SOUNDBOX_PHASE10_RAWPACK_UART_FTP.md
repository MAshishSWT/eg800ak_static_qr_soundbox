# Phase 10 Raw Audio Asset Pack, UART and FTP Import

This package stores the complete MP3 folder tree as an SBAS raw asset pack in the KAE8 external W25Q64 NOR. The native `qextfs_init()` drive mount remains disabled by default because the active Quectel flash layout does not contain `external_fs` or `ext_reserved` partitions.

## Updated Vi_mp3 folder structure

The updated asset package is flat:

```text
Vi_mp3/start_tune.mp3
Vi_mp3/ping.mp3
Vi_mp3/good_bye.mp3
Vi_mp3/transaction_error.mp3
Vi_mp3/en/no_SIM.mp3
Vi_mp3/en/1.mp3
Vi_mp3/en/rupees.mp3
```

The runtime logical paths are:

```text
audio/start_tune.mp3
audio/ping.mp3
audio/good_bye.mp3
audio/transaction_error.mp3
audio/en/no_SIM.mp3
audio/en/1.mp3
audio/en/rupees.mp3
```

## SBAS pack format

```text
u32 magic   = 0x53424153  // SBAS
u32 version = 1
u32 count
repeat count times:
  char path[96]
  u32 offset
  u32 length
  u32 crc32
  u8  reserved[4]
raw mp3 payload bytes
```

## Build asset pack on PC

```bash
python tools/soundbox_asset_pack_builder.py Vi_mp3 Vi_mp3.sbas --manifest Vi_mp3_sbas_manifest.csv
```

Generated from the uploaded package:

```text
assets=1374
size=6459110
crc32=63595b54
```

## UART import API

UART import is available only in factory serial mode.

Begin:

```json
{"cmd":"asset_begin","size":6459110,"crc32":1666800468,"erase":1}
```

Chunk, using sequential offsets and hex data up to 160 raw bytes per command:

```json
{"cmd":"asset_chunk","offset":0,"data_hex":"5342415301000000..."}
```

Finish:

```json
{"cmd":"asset_end"}
```

Status:

```json
{"cmd":"asset_status"}
```

## FTP import API

FTP import downloads the complete `.sbas` file and streams it directly into external NOR through the FTP write callback. Credentials are not logged.

```json
{
  "cmd":"asset_ftp_get",
  "host":"ftp.example.com",
  "user":"username",
  "password":"password",
  "remote":"/soundbox/Vi_mp3.sbas",
  "cid":1,
  "size":6459110,
  "crc32":1666800468
}
```

## Playback path

At runtime the audio service resolves folder-style logical paths through the SBAS index. The selected MP3 is staged to `U:/sb_play.mp3` and played using the QuecOpen MP3 API.

## UART PC transfer helper

```bash
python tools/soundbox_uart_asset_push.py COM12 Vi_mp3.sbas --chunk 128
```

The script sends `asset_begin`, sequential `asset_chunk`, `asset_end`, and `asset_status` commands.
