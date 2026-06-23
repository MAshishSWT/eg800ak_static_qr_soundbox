# Phase 10 Main UART PageWrite Fix

This package fixes the external NOR asset-pack finalization failure seen after a successful Main UART transfer.

## Observed log

```text
factory command=asset_chunk channel=0 status=OK
factory command=asset_end channel=0 status=INVALID_PARAM
factory command=asset_end channel=0 status=INVALID_STATE
```

## Root cause

The transfer path was working, but external NOR programming used the requested UART chunk size directly. A SPI NOR page program must not cross the 256-byte page boundary. With 512-byte or 1024-byte UART chunks, the lower-level write could corrupt the first bytes of the pack even though each asset_chunk command returned OK.

The first asset_end failed with INVALID_PARAM because the pack magic read back from external NOR did not match the expected raw asset-pack magic. Later asset_end attempts failed with INVALID_STATE because the loader session was already closed after the first finalization failure.

## Fixes

- External NOR writes are now split internally on 256-byte page boundaries.
- UART chunk size remains 512 or 1024 bytes; the firmware safely writes each chunk as page-safe NOR programs.
- asset_end now verifies the CRC by reading the complete pack back from external NOR, not only by trusting the UART-received bytes.
- asset_end logs the exact header bytes and magic value when final validation fails.
- asset_status now reports expected size, received size, expected CRC, flash CRC, header magic and first 4 header bytes.
- The PC transfer tool checks the `written` value returned by every chunk.
- The PC transfer tool prints pre-end and final status.
- `--status-only` was added to read current loader diagnostics without starting a transfer.

## Recommended transfer command

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 512 --baud 921600 --timeout 20 --retries 3
```

After 512-byte chunks are stable:

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 1024 --baud 921600 --timeout 20 --retries 3
```

## Diagnostic command

```bash
python tools/soundbox_uart_asset_push.py COM16 --status-only --baud 921600 --timeout 20 --retries 3
```

Expected successful header:

```json
{"header_hex":"53414253","header_magic":1396859219}
```

The hex string `53414253` is the on-flash little-endian representation used by the generated pack.
