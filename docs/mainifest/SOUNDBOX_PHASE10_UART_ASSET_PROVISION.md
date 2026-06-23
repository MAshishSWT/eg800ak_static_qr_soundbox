# Phase 10 UART Asset Provisioning Enablement

## Change

The firmware now enables USB CDC serial only for audio asset provisioning even when the device mode is production.

## Security boundary

Production-mode serial access is limited to asset-pack commands:

```text
asset_begin
asset_chunk
asset_end
asset_status
asset_ftp_get
```

Configuration, mode change, OTA key, and SMS recovery commands still require factory/debug access.

## Expected boot log

```text
[SB][I][serial] enabled for asset provisioning
```

## Recommended command

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 512 --baud 921600 --timeout 20 --retries 3
```

Use `--chunk 1024` after the 512-byte test is confirmed stable.
