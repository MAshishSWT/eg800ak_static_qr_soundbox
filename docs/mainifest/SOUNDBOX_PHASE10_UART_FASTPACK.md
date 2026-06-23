# Phase 10 UART Fast Asset Pack Transfer

## Change

The UART/USB CDC audio-pack provisioning path has been increased from 128-byte class chunks to a safe 512-byte default, with firmware support for up to 1024 raw bytes per JSON hex chunk.

## Firmware limits

```text
SB_ASSET_PACK_UART_HEX_MAX_BYTES = 1024
SB_SERIAL_LINE_LEN = 2304
```

A 1024-byte raw chunk becomes 2048 hex characters plus JSON overhead, so the serial line buffer is sized to safely receive it.

## Recommended command

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 512 --baud 921600 --timeout 20 --retries 3
```

For faster USB CDC testing:

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 1024 --baud 921600 --timeout 20 --retries 3
```

## Safety

The PC tool waits for an `ok` response for every command/chunk before sending the next chunk. Failed chunks are retried. The device still validates final size and CRC in `asset_end`.
