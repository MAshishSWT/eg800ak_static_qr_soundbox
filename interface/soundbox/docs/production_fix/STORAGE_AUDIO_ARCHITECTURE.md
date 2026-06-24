# Storage and Audio Architecture

## Internal U-drive

The internal QuecOpen file system is mounted as U-drive. Phase 23 uses absolute U-drive paths only.

Common MP3 files are stored at root:

- `U:/start_tune.mp3`
- `U:/ping.mp3`
- `U:/good_bye.mp3`
- `U:/transaction_error.mp3`

Other U-drive folders:

- `U:/config` for configuration slots
- `U:/ledger` for transaction ledger
- `U:/certs` for PEM files
- `U:/logs` for runtime logs
- `U:/diag` for self-test outputs
- `U:/cache` for one temporary audio playback cache

## External NOR

All language MP3 assets are packed to W25Q64 external NOR with a manifest at address `0x00000000`. Asset data starts at `0x00030000`. Each manifest entry contains logical path, absolute NOR offset, length, CRC32, and language code.

Logical language paths use:

`audio/<language>/<file>.mp3`

Supported language folders are `bn`, `en`, `gu`, `hi`, `kn`, `ma`, `ml`, `pa`, `ta`, and `tl`.

## Playback rule

The QuecOpen MP3 player is given a filesystem path. For a language asset, the firmware copies only the selected NOR asset to `U:/cache/asset_play.mp3`, verifies CRC32, plays the file, and then reuses the same cache for the next prompt. It never mirrors all language files into U-drive.

## Fallback rule

Resolution order:

1. Requested language asset.
2. Known filename variants where present, such as `transaction.mp3`, `transaction_s.mp3`, `trasnsactions.mp3`, `rupees.mp3`, and `ruppes.mp3`.
3. English asset where the same filename exists.
4. `U:/transaction_error.mp3` for unrecoverable transaction prompt failure.
