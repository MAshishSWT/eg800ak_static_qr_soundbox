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

## Phase 23 v3 runtime correction

- Normalized EG800AK QuecOpen W25Q64 JEDEC byte order. Runtime log `17 60 EF` is now accepted as normalized `EF 60 17` for the W25Q64 device.
- Mapped SIM-not-inserted LED state to the non-fatal no-internet pattern instead of the fatal rapid-error pattern. Storage/NOR failures remain fatal LED errors.
- Common audio files still require factory provisioning to U-drive root: `U:/start_tune.mp3`, `U:/ping.mp3`, `U:/good_bye.mp3`, and `U:/transaction_error.mp3`.


## Phase-23 v4 runtime patch

- Version string changed to `1.0.0-phase23-production-fix-v4` so field logs prove the flashed package.
- MP3 playback now probes U-drive files before playback and logs file size plus first bytes.
- MP3 player timeout field now follows the QuecOpen audio guide recommendation and uses `0` for normal file playback.
- Added stream-mode fallback when `ql_play_mp3(path, ...)` returns failure on EG800AK U-drive files.
- External NOR JEDEC normalization remains enabled for raw `17 60 EF` responses, which represent normalized W25Q64 ID `EF 60 17`.

## Runtime patch v5

- Version string changed to `1.0.0-phase23-production-fix-v5`.
- Common U-drive MP3 existence now uses `ql_fopen()`/`ql_fsize()` instead of `ql_access()` to avoid false missing results after serial provisioning.
- Boot now logs the presence and size of all four common root MP3 assets.
- Added `ufs_file` factory diagnostic command to verify a U-drive file by name.
- If external NOR language assets are not yet provisioned, missing status prompts fall back to `U:/transaction_error.mp3` instead of ending silently.


## v6 runtime patch — U-drive MP3 playback compatibility

- U-drive was confirmed good by `ql_fopen()`/`ql_fsize()` and file persistence across reboot.
- Runtime failure was isolated to MP3 decoder/API compatibility: original common prompts were ID3v2.2 tagged 11.025/12 kHz MP3 files; QuecOpen MP3 API returned `-1` after file probe succeeded.
- Added QuecOpen sample-aligned playback init: `ql_audio_play_init`, `ql_bind_speakerpa_cb`, `ql_set_audio_path_speaker`, gain-table setup, and old-architecture `ql_mp3_file_play()` fallback.
- Updated UFS provisioning tool to push known-good 16 kHz mono 32 kbps no-ID3 common MP3 files by default.
- Added bundled factory assets under `tools/factory_assets/common_mp3_16k/`.
