# Soundbox Phase 8 - OTA Service

## Implemented

Phase 8 adds rollback-aware OTA infrastructure:

- Firmware OTA manifest parsing.
- Audio-pack OTA manifest parsing.
- HMAC-SHA256 signed manifest validation using `ql_securedata_read()` key index 1.
- SHA-256 download hash validation.
- HTTPS/HTTP download using Quectel HTTP client API.
- Firmware staging through Quectel FOTA image APIs.
- Firmware activation flag set only after manifest signature, download size, SHA-256, and Quectel FOTA package verification pass.
- Audio-pack staging to `U:/sb_audio_pack.tmp` and atomic activation to target path.
- OTA status, progress, staged, and failure events.
- OTA command hook through the Phase 7 command dispatcher.

## EG800AK SDK examples referenced

- `interface/fota/example_http_fota.c` for HTTP download to `ql_fota_image_write()` and FOTA verification flow.
- `interface/fota/example_fota.c` for `ql_fota_init()`, image write/flush/verify and package path usage.
- `interface/http/example_httpclient.c` for HTTP client callback style.
- `interface/securedata/example_securedata.c` for secure data read/write API usage.
- `interface/os/example_rtos.c` for RTOS task, queue, and mutex usage.
- `interface/fs/example_fs.c` for file write and rename patterns.

## Quectel documents used

- FOTA Upgrade Guide.
- HTTP Development Guide.
- SSL Application Note.
- File System Development Guide.
- File Application Note.
- RTOS Development Guide.
- Device Management Guide.

## EC200U modules migrated

The EC200U update concepts are migrated into a clean EG800AK service:

- Firmware update command -> `sb_ota_service`.
- Audio update staging -> `sb_ota_service` audio-pack path.
- Command-triggered update -> Phase 7 `sb_command_dispatcher` OTA hook.

No EC200U platform APIs are copied.

## Manifest format

Example firmware manifest command payload:

```json
{"cmd":"ota_firmware","type":"firmware","version":"1.0.1","url":"https://server/fw.bin","size":123456,"sha256":"64_hex_chars","signature":"64_hex_chars"}
```

Example audio-pack manifest command payload:

```json
{"cmd":"ota_audio_pack","type":"audio_pack","version":"en-001","url":"https://server/audio.bin","size":12345,"sha256":"64_hex_chars","target_path":"U:/sb_audio_pack.bin","signature":"64_hex_chars"}
```

Canonical signed text:

```text
type|version|url|size|sha256|target_path
```

For firmware with no `target_path`, the last field is omitted.

## Secure key provisioning

The 32-byte HMAC key must be provisioned in secure-data index 1 before OTA testing. The key is not hardcoded in firmware.

## Build

Copy the package over the EG800AK `ql-application/threadx` tree and build using the normal Quectel OpenEntry SDK flow.

## Known assumptions

- Firmware OTA package is a valid Quectel FOTA/DFOTA image accepted by `ql_fota_image_verify_without_setflag()`.
- Audio-pack OTA activation stages a verified binary/asset file to the configured target path. Multi-file audio pack extraction can be added in a later asset-pack format phase.
- External NOR remains a separate hardware issue; OTA staging uses internal `U:` filesystem and Quectel FOTA storage path.

## Audio pack file format

When `type=audio_pack` has no `target_path`, the downloaded file is treated as a complete Soundbox Audio Pack binary.

Binary format, little-endian:

```text
u32 magic        0x50414253  // 'SBAP'
u32 pack_version non-zero
u32 file_count   1..16
repeat file_count times:
  u32 path_len
  u32 data_len
  char target_path[path_len]   // must start with U:/audio/
  uint8 file_data[data_len]
```

The installer writes every asset to a temporary file, backs up the existing asset, renames the new file into place, and restores prior files if any later file fails. This gives rollback-safe activation for the pack within available U: filesystem space.

If `target_path` is present, the manifest is treated as a single signed audio asset update. The target must still pass the same allowlist policy and start with `U:/audio/`.
