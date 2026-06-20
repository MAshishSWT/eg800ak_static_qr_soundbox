# Static QR UPI Soundbox - Phase 3 Storage and Configuration

This module is the EG800AK-CN QuecOpen application for the Static QR UPI Soundbox.

Phase 3 provides the Phase 1/2 application skeleton and KAE8_SQ1 BSP/HAL plus storage and configuration services:

- File-system mount and directory preparation for `U:/soundbox`.
- Atomic file write helper using QuecOpen file APIs.
- A/B configuration slots with magic, version, sequence, payload CRC, and header CRC.
- Safe factory defaults with no production credentials.
- Configuration load, validate, sanitize, and commit APIs.
- External SPI NOR abstraction using EG800AK `ql_spi_*` APIs with JEDEC NOR commands.
- Storage-ready and config-ready events for the supervisor.

Payment MQTT, audio playback, data call, OTA, SMS, transaction ledger, and audio asset indexing are attached in later phase-specific service packages.


## External NOR flash

`U:` is the Quectel user filesystem partition. The board-level W25Q64-class external NOR is handled separately by `sb_extnor.*` using the EG800AK `ql_spi_*` API with JEDEC NOR commands on the KAE8 FLASH_* nets. Legacy multi-port SPI NOR scanning has been removed.
