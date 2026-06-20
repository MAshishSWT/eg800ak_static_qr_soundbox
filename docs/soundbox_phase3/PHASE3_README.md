# Static QR UPI Soundbox - Phase 3 Storage and Configuration

## Implemented scope

Phase 3 adds production-oriented storage and configuration services on top of the approved Phase 2 BSP/HAL package.

Implemented runtime items:

- QuecOpen file-system mount for `U:` and application directories under `U:/soundbox`.
- Bounded file read helper using `ql_fopen`, `ql_fread`, and `ql_fclose`.
- Atomic file write helper using temporary file, `ql_fwrite`, `ql_fsync`, `ql_fclose`, `ql_remove`, and `ql_rename`.
- A/B configuration slot manager with magic, version, sequence, slot ID, payload CRC, and header CRC.
- Configuration defaults with empty credential fields, SMS recovery disabled, English language, 70 percent volume, and secure MQTT default port.
- Configuration sanitization for fixed-length strings and bounded numeric ranges.
- External SPI NOR abstraction using EG800AK `ql_spi_nor_init`, `ql_spi_nor_read_id`, `ql_spi_nor_read`, `ql_spi_nor_write`, and `ql_spi_nor_erase_sector`.
- Storage-ready and config-ready events posted into the Phase 1 event bus.
- Supervisor logs for storage and config startup events.

## EG800AK SDK headers and examples referenced

The Phase 3 code was aligned to APIs and call style from the uploaded `ql_application_eg800ak.zip` package:

- `common/include/fs/ql_fs.h`
- `common/include/ql_spi_nor.h`
- `common/include/ql_flash.h`
- `common/include/ql_rtos.h`
- `interface/fs/example_fs.c`
- `interface/flash/example_spi_nor.c`
- `interface/flash/example_flashspeed.c`
- `interface/os/example_rtos.c`
- `config/common/makefile.mk`

## Quectel documents used

- Quick Start Guide
- RTOS Development Guide
- RTOS API Mapping User Guide
- File System Development Guide
- File Application Note
- RTOS Flash API Reference Manual
- RTOS SPI NOR Flash API Reference Manual
- SPI Development Guide
- Device Management Guide

## EC200U source logic migrated

Phase 3 migrates only durable business concepts from the EC200U source package:

- `device_config_prcs.c`: configuration persistence concept migrated into A/B slot records.
- `common_prj_def.c/.h`: product default-state concept migrated into `sb_config_make_defaults()`.
- EC200U NVM direct access patterns are replaced by EG800AK FS and SPI NOR service wrappers.

No EC200U storage API, unsafe string builder, or monolithic configuration global was copied.

## Storage layout

```text
U:/soundbox/
U:/soundbox/config/
U:/soundbox/config/config_a.bin
U:/soundbox/config/config_b.bin
U:/soundbox/config/config_tmp.bin
```

## Configuration record layout

Each slot stores a complete binary record:

```text
magic
version
header_size
sequence
slot_id
payload_size
payload_crc
header_crc
payload
```

The loader validates both slots and selects the highest valid sequence. When both slots are absent, defaults are generated and committed to slot A.

## Integration

Copy this package over the EG800AK SDK application package. The root `Makefile` already compiles only:

```text
COMMPILE_DIRS := interface/soundbox
```

The soundbox Makefile includes all Phase 1, Phase 2, and Phase 3 source files.

## Build

Use the EG800AK QuecOpen build environment:

```bat
build.bat clean
build.bat app
```

The uploaded build script treats `clean` specially and otherwise invokes the default SDK Makefile flow. Final binary packaging must follow the vendor release process supplied with the EG800AK SDK toolchain.

## KAE8_SQ1 hardware test

1. Flash the generated application.
2. Open the EG800AK debug log port.
3. Power the KAE8_SQ1 board.
4. Confirm BSP logs from Phase 2 still appear.
5. Confirm storage event and config event logs appear.
6. Inspect `U:/soundbox/config/` through the available factory/debug file access path and confirm a config slot file is created.
7. Reboot the board and confirm the config service loads from storage rather than committing defaults again.
8. Confirm no credential values are printed in logs.
9. If a W25Q64 SPI NOR is accessible through the EG800AK SPI NOR driver, confirm the logged NOR probe result is OK.

## Known assumptions

- The `U:` disk is used for soundbox application files because the uploaded QuecOpen FS examples use `U:` for writable file storage.
- Auto-format is not performed during normal boot to avoid destructive recovery behavior.
- External SPI NOR probing scans SDK-supported NOR ports exposed by `ql_spi_nor.h` and records the first valid flash ID.
- Audio asset indexing and transaction ledger storage are assigned to later storage-domain phases and will use the stable storage APIs added here.
