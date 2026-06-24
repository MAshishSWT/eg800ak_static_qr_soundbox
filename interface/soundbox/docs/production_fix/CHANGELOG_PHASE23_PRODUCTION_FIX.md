# Phase 23 Production Fix Changelog

- Updated version to `1.0.0-phase23-production-fix`.
- Added external W25Q64 NOR service with JEDEC-ID validation, bounded read/write/erase, CRC, and manufacturing read/write test.
- Added audio asset manifest and two-tier audio store.
- Moved four common MP3 files to U-drive root.
- Added hardened U-drive mount, recursive mkdir, atomic write, exact byte-count checks, and FS self-test.
- Added HTTPS registration/health service.
- Updated MQTT TLS file paths and time-valid gate.
- Reworked GPIO key task for debounced edge processing.
- Preserved one-LED status model and documented color-to-pattern conversion.
- Added production provisioning and asset packing tools.
- Added board pin map and validation documents.
