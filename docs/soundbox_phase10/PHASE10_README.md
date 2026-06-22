# Static QR UPI Soundbox - Phase 10 Audio Assets Business Logic

## Scope

Phase 10 integrates the supplied `audio_assets.zip` business logic and asset path model into the EG800AK-CN QuecOpen soundbox application.

Implemented areas:

- Audio asset logical path mapping for the supplied asset tree.
- Language-folder aliasing: `mr -> ma`, `te -> tl`.
- Correct `alerts/` folder and exact `no_SIM.mp3` filename handling.
- Common prompt playback: `start_tune.mp3`, `ping.mp3`, `good_bye.mp3`, `transaction_error.mp3`.
- Health status audio for battery and signal percentage.
- Payment transaction announcement with and without paise.
- Last transaction announcement with and without paise.
- Daily summary with single/multiple transaction grammar.
- Provider fallback: Google Pay uses `googlepay.mp3`; missing Paytm/PhonePe/BHIM use `bank.mp3`.
- Missing advertisement handling: `advert.mp3` is optional and skipped.
- Marathi 100 fallback uses `1.mp3 + hundred.mp3` because `100.mp3` is not present.
- External NOR aware asset store with one-asset staging to `U:/sb_play.mp3`.
- UFS debug fallback for small tests without external NOR.
- Single USER_LED_1 semantic status patterns instead of RGB/three LED logic.

## Audio package files inspected

The uploaded package contains:

- Audio root: `audio/`
- Languages: `bn`, `en`, `gu`, `hi`, `kn`, `ma`, `ml`, `pa`, `ta`, `tl`
- MP3 count: 1292
- Business rule workbooks:
  - `README/Health_Status.xlsx`
  - `README/Last_Transaction.xlsx`
  - `README/Summary_WITH_NO_PAISE.xlsx`
  - `README/Summary_WITH_PAISE.xlsx`
  - `README/Transaction_With_NO_PAISE.xlsx`
  - `README/Transaction_With_PAISE.xlsx`
- Text logic reference:
  - `README/audio_notification_logic_readme`

## EG800AK SDK references used

- `common/include/ql_application.h`
- `common/include/ql_rtos.h`
- `common/include/ql_audio.h`
- `common/include/ql_fs.h`
- `common/include/ql_spi_nor.h`
- `common/include/ql_gpio.h`
- `interface/audio/example_mp3.c`
- `interface/audio/example_play_audio.c`
- `interface/fs/example_fs.c`
- `interface/driver/example_gpio.c`
- `interface/os/example_rtos.c`
- `config/common/makefile.mk`

## External NOR audio asset model

The firmware now treats audio paths as logical paths, for example:

```text
audio/en/alerts/no_mqtt.mp3
audio/en/audio_files/transaction_prefix.mp3
audio/common/start_tune.mp3
```

Playback is resolved through `sb_audio_asset_store`:

1. If `U:/audio/...` exists, it is used for debug/small asset testing.
2. If an external NOR asset pack with the Phase 10 index is present, the selected single MP3 is staged to `U:/sb_play.mp3`.
3. If neither backend has the file, the audio request fails gracefully with `SB_EVENT_AUDIO_FAULT` and the app continues.

Internal `U:` is not used to hold the full library.

## Single LED adaptation

KAE8_SQ1 has one firmware-controlled USER_LED_1, so Phase 10 implements one-LED semantic patterns:

- Internet OK: ON
- No internet: slow blink
- No MQTT: fast blink
- Unregistered: double blink
- Battery low: triple blink
- Volume mode: one short blink
- Transaction mode: two short blinks
- Fault/error: rapid blink

## Integration

Copy this package over the EG800AK `ql_application` ThreadX application tree and build normally. The app remains under:

```text
interface/soundbox
```

No Quectel common headers or libraries are modified.


## External NOR filesystem mount update

The audio asset store now attempts to mount the KAE8 external W25Q64 SPI NOR as a native QuecOpen filesystem drive `C:` using `qextfs_init()`. If the mount succeeds, logical audio paths resolve directly to `C:/audio/...`. If the mount fails because the active flash layout does not expose an external filesystem partition, the firmware remains boot-safe and falls back to `U:/audio/...` debug assets and the existing raw SBAS external-NOR asset index.

Default builds do not format the external NOR. For a one-time lab format build only, define `SB_AUDIO_STORE_EXTFS_FORMAT_ON_MOUNT=1`, then rebuild without it after the first successful mount.


## Raw audio asset pack UART/FTP update

The updated Vi_mp3 package uses a flat folder structure. Runtime paths are now `audio/<file>.mp3` for common prompts and `audio/<lang>/<file>.mp3` for language prompts. The firmware keeps the folder-path strings inside an SBAS index stored in external NOR. UART factory commands and FTP download can write the full SBAS pack directly to external NOR. See `SOUNDBOX_PHASE10_RAWPACK_UART_FTP.md`.
