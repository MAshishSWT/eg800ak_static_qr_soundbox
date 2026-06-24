# Phase 23 Inventory and Mismatch Report

## Package scope

Input package: `1.0.0-phase22-critical-audio-mqttfix`.
Output firmware phase: `1.0.0-phase23-production-fix`.
Board: `KAE8_SQ1_260611`, EG800AK-CN.

## Existing source modules retained

Core application, event bus, supervisor, board support, GPIO HAL, ADC, key HAL, LED status, storage, config, SIM, time, network, MQTT, JSON parser, transaction ledger, payment processor, command dispatcher, business service, mode service, factory service, serial service, SMS service, audio codec HAL, audio prompt builder, audio script, and ES8311 audio codec driver remain in the soundbox application.

## Features corrected

- External W25Q64 NOR flash is enabled through QuecOpen SPI NOR APIs.
- HTTPS registration/health service is compiled and started.
- Internal U-drive storage paths are normalized.
- Audio storage is split between internal U-drive common files and external NOR language files.
- LED status is treated as one physical LED only.
- GPIO key handling uses debounced task-level processing.
- MQTT certificate file paths are under `U:/certs`.

## Missing modules added

- `include/sb_ext_nor_flash.h`, `src/sb_ext_nor_flash.c`
- `include/sb_audio_asset_manifest.h`, `src/sb_audio_asset_manifest.c`
- `include/sb_http_service.h`, `src/sb_http_service.c`
- External NOR asset packer and production provisioning scripts under `tools/`

## GPIO mapping

| Function | Schematic net | EG800AK pin | GPIO | Active level | Firmware macro |
|---|---|---:|---:|---|---|
| Volume up | SW1 | 57 | GPIO[57] | Low | `SB_KAE8_KEY_VOLUME_UP_PIN` |
| Volume down | SW2 | 87 | GPIO[87] | Low | `SB_KAE8_KEY_VOLUME_DOWN_PIN` |
| Mode/action | SW3 | 82 | GPIO[8] | Low | `SB_KAE8_KEY_MODE_PIN` |
| User LED | USER_LED_1 | 83 | GPIO[69] | Logical high | `SB_KAE8_LED_USER_PIN` |
| Speaker PA shutdown | SPK_SHDN | 22 | GPIO[54] on MAIN_CTS alternate GPIO | Verify on PCB | `SB_KAE8_SPK_SHDN_GPIO` |
| Battery ADC | BATT_VTG_SENS | ADC0 | ADC0 | Divider 120K/47K | `SB_KAE8_BATT_ADC_CHANNEL` |

## SPI NOR mapping

The W25Q64JWSIQ is connected to schematic nets `FLASH_SYNC`, `FLASH_CLK`, `FLASH_DOUT`, `FLASH_WP`, `FLASH_DIN`, and `FLASH_RST`. The board config selects QuecOpen external NOR port `EXTERNAL_NORFLASH_PORT4_7` for the KP pin group and sets `SB_NOR_DI_DO_INTERCHANGED=1` to capture the known PCB DI/DO interchange. Normal boot performs JEDEC-ID validation and does not erase audio assets.

## U-drive path mismatches corrected

Phase 22 expected common MP3 files below a subfolder. Phase 23 resolves only these common files from U-drive root:

- `U:/start_tune.mp3`
- `U:/ping.mp3`
- `U:/good_bye.mp3`
- `U:/transaction_error.mp3`

Config, ledger, certificate, log, diagnostic, and cache paths are under `U:/config`, `U:/ledger`, `U:/certs`, `U:/logs`, `U:/diag`, and `U:/cache`.

## Audio asset compatibility

Root common files found: good_bye.mp3, ping.mp3, start_tune.mp3, transaction_error.mp3.

|Language|Count|transaction.mp3|transaction_s.mp3|trasnsactions.mp3|rupees.mp3|ruppes.mp3|internet.mp3|no_internet.mp3|no_mqtt.mp3|device_unregistered.mp3|battery_low.mp3|no_SIM.mp3|no_transactions.mp3|
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|bn|135|-|-|-|-|yes|yes|yes|yes|yes|yes|yes|yes|
|en|138|yes|yes|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|gu|137|-|yes|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|hi|135|-|yes|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|kn|138|-|-|yes|yes|-|yes|yes|yes|yes|yes|yes|yes|
|ma|137|yes|yes|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|ml|142|-|-|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|pa|136|-|-|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|ta|136|-|-|-|yes|-|yes|yes|yes|yes|yes|yes|yes|
|tl|136|-|-|-|yes|-|yes|yes|yes|yes|yes|yes|yes|


## Network and certificate corrections

- MQTT certificates are loaded from `U:/certs/mqtt_root_ca.pem`, `U:/certs/mqtt_client.crt`, and `U:/certs/mqtt_client.key`.
- TLS is gated by valid time before connection.
- HTTPS registration uses `U:/certs/mqtt_root_ca.pem` by default so a single factory certificate set can validate both services when the same CA is used.
- MQTT-ready event is posted only after connected and subscribed state is reached.

## Hardware validation points

- Confirm the selected QuecOpen NOR port matches the EG800AK KP pin group on the PCB.
- Confirm the DI/DO interchange produces valid W25Q64 JEDEC ID `EF 40 17` or compatible W25Q64 response.
- Confirm ES8311 I2C channel and PA shutdown polarity with UART diagnostics.
- Confirm the HTTPS host/path matches production Kiotel backend configuration.
