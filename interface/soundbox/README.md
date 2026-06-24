# Static QR UPI Soundbox - EG800AK-CN

Production build for KAE8_SQ1_260611 hardware.

Version: `1.0.0-phase23-production-fix`

## Hardware scope

- Quectel EG800AK-CN QuecOpen target
- ES8311 codec and 8002A speaker amplifier
- Internal QuecOpen U-drive filesystem
- External W25Q64 NOR flash on the board FLASH nets
- Three active-low keys: SW1 volume up, SW2 volume down, SW3 mode/action
- One user LED only: USER_LED_1

## Storage policy

U-drive root stores only four common MP3 files:

- `U:/start_tune.mp3`
- `U:/ping.mp3`
- `U:/good_bye.mp3`
- `U:/transaction_error.mp3`

Language MP3 files are packed into external NOR and resolved through the manifest as `audio/<language>/<file>.mp3`.

U-drive folders:

- `U:/config`
- `U:/ledger`
- `U:/certs`
- `U:/logs`
- `U:/diag`
- `U:/cache`

## Build

From the QuecOpen SDK application root:

```sh
make clean
make
```

## Feature summary

At boot the firmware prints:

```text
app: feature summary phase=23 nor=enabled http=enabled mqtt_tls=file serial=main_uart
```

## Provisioning

Use the scripts in `tools/`:

- `soundbox_uart_ufs_push.py` for U-drive common MP3 files, certificates, and config.
- `soundbox_extnor_asset_pack.py` for language MP3 NOR image and manifest.
- `soundbox_factory_diag.py` for factory checks.

## LED behavior

The board has one LED. Earlier color states are converted to timing patterns:

- Ready: solid on
- No internet: slow blink
- No MQTT: fast blink
- Unregistered: double blink
- Low battery: triple blink
- Storage/fatal fault: rapid blink
- Key acknowledgement: short pulse

## Certificates

MQTT certificates are loaded from:

- `U:/certs/mqtt_root_ca.pem`
- `U:/certs/mqtt_client.crt`
- `U:/certs/mqtt_client.key`

HTTPS registration uses the configured root CA path from `sb_http_service.h`.

## Documentation

See `interface/soundbox/docs/production_fix/` for the pin map, storage/audio architecture, MQTT/SSL/HTTP flow, provisioning steps, test plan, and Phase 23 changelog.
