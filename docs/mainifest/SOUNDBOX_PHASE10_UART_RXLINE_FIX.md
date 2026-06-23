# Phase 10 UART RX Line Fix

## Issue

The PC tool opened the COM port but received no reply. The firmware had serial enabled, so the likely fault path was UART receive fragmentation: one JSON command can arrive across multiple USB CDC read callbacks.

## Fix

- UART RX now accumulates bytes until CR/LF before dispatching a JSON command.
- RX callback drains all available data in 512-byte blocks.
- Serial service is opened at 921600 baud for asset provisioning.
- Existing 512/1024-byte asset chunks remain supported.

## Expected boot log

```text
[SB][I][serial] enabled for asset provisioning
[SB][I][serial] task started baud=921600
[SB][I][supervisor] serial ready text=usb_cdc
```

## Recommended transfer

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 512 --baud 921600 --timeout 20 --retries 3
```
