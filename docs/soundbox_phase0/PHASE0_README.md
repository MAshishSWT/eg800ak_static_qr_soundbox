# Phase 0 README - EG800AK-CN Static QR UPI Soundbox

## Phase implemented

**Phase 0 only:** Deep-dive the EG800AK QuecOpen SDK application source package and the EC200U UPI soundbox source package, then create the migration architecture document and identify EG800AK API replacements and integration points.

Phase 0 now includes a build-integrated soundbox anchor module. The anchor compiles a small contract source file and public headers, but it does not register an application task and does not call `application_init()`. This preserves the original SDK startup behavior while proving that the `interface/soundbox` module follows the EG800AK Makefile structure.

## What was implemented

The following architecture and migration documents were added:

- `MIGRATION_ARCHITECTURE.md`
- `EG800AK_API_REPLACEMENT_MATRIX.md`
- `EC200U_BUSINESS_LOGIC_INVENTORY.md`
- `KAE8_SQ1_HARDWARE_MAPPING.md`
- `PHASE0_TEST_PROCEDURE.md`
- `PHASE0_ACCEPTANCE_CHECKLIST.md`
- `SOUNDBOX_PHASE0_FILE_MANIFEST.md`
- `interface/soundbox/README.md`
- `interface/soundbox/Makefile`
- `interface/soundbox/include/sb_phase0_contract.h`
- `interface/soundbox/include/sb_board_kae8_sq1.h`
- `interface/soundbox/src/sb_phase0_build_anchor.c`

## EG800AK SDK examples and headers referenced

Primary source reference was `ql_application_eg800ak.zip`. The following files were inspected and mapped:

- `common/include/ql_application.h`
- `common/include/ql_rtos.h`
- `common/include/ql_gpio.h`
- `common/include/ql_adc.h`
- `common/include/ql_iic.h`
- `common/include/ql_audio.h`
- `common/include/fs/ql_fs.h`
- `common/include/ql_flash.h`
- `common/include/ql_spi_nor.h`
- `common/include/ql_data_call.h`
- `common/include/ql_nw.h`
- `common/include/ql_sim.h`
- `common/include/ql_sms.h`
- `common/include/mqtt/MQTTClient.h`
- `common/include/mqtt/MQTTQlRTOS.h`
- `common/include/ql_http_client.h`
- `common/include/ql_ssl_hal.h`
- `common/include/ql_fota.h`
- `common/include/ql_rtc.h`
- `common/include/ql_ntp.h`
- `common/include/ql_power.h`
- `common/include/ql_wtd.h`
- `common/include/ql_securedata.h`
- `common/include/ql_uart.h`
- `interface/os/example_rtos.c`
- `interface/driver/example_gpio.c`
- `interface/driver/example_adc.c`
- `interface/driver/example_iic.c`
- `interface/driver/example_spi.c`
- `interface/driver/example_uart.c`
- `interface/audio/example_mp3.c`
- `interface/audio/example_pcm.c`
- `interface/audio/example_play_audio.c`
- `interface/fs/example_fs.c`
- `interface/flash/example_spi_nor.c`
- `interface/network/data_call/example_datacall.c`
- `interface/network/nw/example_nw.c`
- `interface/network/sim/example_sim.c`
- `interface/network/sms/example_sms.c`
- `interface/mqtt/example_mqtt.c`
- `interface/http/example_httpclient.c`
- `interface/http/example_httpclient_perform.c`
- `interface/ssl/example_ssl.c`
- `interface/fota/example_fota.c`
- `interface/fota/example_http_fota.c`
- `interface/ntp/example_ntp.c`
- `interface/time/example_rtc.c`
- `interface/platform/example_wtd.c`
- `interface/securedata/example_securedata.c`
- `config/common/makefile.mk`
- root `Makefile`

## Quectel documents used

The Phase 0 plan uses the uploaded EG800AK QuecOpen SDK document set:

- Quick Start Guide
- RTOS Development Guide
- RTOS API Mapping User Guide
- Device Management Guide
- Data Call Development Guide
- MQTT Development Guide
- HTTP Development Guide
- SSL Application Note
- GPIO Development Guide
- ADC Development Guide
- I2C Development Guide
- Audio Development Guide
- Voice Development Guide
- File System Development Guide
- File Application Note
- RTOS Flash API Reference Manual
- RTOS SPI NOR Flash API Reference Manual
- SPI Development Guide
- RTC Development Guide
- (U)SIM Development Guide
- FOTA Upgrade Guide
- Serial Port Development Guide
- Serial Port AT Command Processing User Guide

The exact implementation must continue to use the APIs and call styles that are available in `ql_application_eg800ak.zip`; do not import EC200U-only APIs into the EG800AK port.

## EC200U modules analyzed for migration

The EC200U source package was analyzed for business logic extraction. No EC200U source was copied into the EG800AK package in Phase 0.

| EC200U file | Migration purpose |
|---|---|
| `mqtt/mqtt_demo.c` | MQTT payment parsing, device command flow, health packet, NTP/RTC day reset, data call state flow, command response, FOTA/audio update control flow. |
| `mqtt/audio_prcs.c` | Audio state machine, provider/language audio file path composition, battery/network/summary/transaction playback sequencing. |
| `mqtt/transaction_play.c` | Amount-to-audio grammar for English, Hindi, Marathi, Gujarati, Bengali, Tamil, Telugu, Punjabi, Kannada, Malayalam. |
| `mqtt/summary_play.c` | Daily summary audio grammar. |
| `mqtt/dailylog_play.c` | Last transaction log browsing grammar. |
| `mqtt/keypad_prcs.c` | Short/long press actions for volume, mode, battery, signal, summary. |
| `mqtt/battery_monitoring_prcs.c` | Battery percentage conversion logic. |
| `mqtt/device_config_prcs.c` | Device configuration and audio pack download flow. |
| `mqtt/http_prcs.c` | HTTP POST request and response handling concept. |
| `mqtt/fota_ftp.c` | Firmware OTA flow concept; must be redesigned around EG800AK `ql_fota.h` and HTTP/FOTA examples. |
| `mqtt/sms_demo.c` | SMS recovery/config command concept; disabled by default in production. |
| `mqtt/common_prj_def.c/.h` | Persistent application database concept, language/provider state, daily transaction counters. |
| `mqtt/circular_buffer.c/.h` | Last transaction ring buffer concept. |
| `mqtt/string_handling.c/.h` | Indian amount string formatting concept. |
| `mqtt/systick.c/.h` | Timeout helper concept; to be replaced by `ql_rtos_get_systicks()`-based service. |

## Integration inside `ql_application_eg800ak.zip`

Copy this package over the extracted `ql_application_eg800ak.zip` tree, or build directly from the extracted `phase0_pkg` directory in the archive. Phase 0 adds the soundbox module to root `COMMPILE_DIRS` and provides `interface/soundbox/Makefile`, following the same module style as the EG800AK examples.

The Phase 0 anchor module compiles only `src/sb_phase0_build_anchor.c` and the public headers under `interface/soundbox/include`. It does not create a task and does not register `application_init()`.

Phase 1 implementation rules:

1. Keep the existing `interface/soundbox/Makefile` structure.
2. Add production source files under `interface/soundbox/src` and public headers under `interface/soundbox/include`.
3. Add exactly one production entry point, for example `application_init(sb_app_entry, "sb_app", <stack_kib>, <startup_prio>)`.
4. Exclude example modules with active `application_init()` entries from the production root `COMMPILE_DIRS` when the production supervisor is introduced.

## How to build Phase 0 package

Phase 0 registers the `interface/soundbox` anchor module in the root Makefile. Use the same command style as the Quectel SDK quick start flow:

```bat
cd <extracted_eg800ak_sdk_application_root>
build.bat app
```

The uploaded `build.bat` handles `clean` specially and otherwise invokes the default Makefile build. No separate `firmware` target was verified in the uploaded script.

## How to test Phase 0 on KAE8_SQ1 EG800AK-CN hardware

Phase 0 hardware testing validates the schematic mapping and base SDK readiness:

1. Confirm the unmodified EG800AK SDK example build succeeds.
2. Flash the existing example image only if required for base SDK sanity.
3. Confirm USB, MAIN UART, debug UART, SIM interface, VBAT, and antenna hardware are present.
4. Confirm page-2 audio hardware is populated: ES8311 codec, 8002A PA, speaker, `SPK_SHDN` control path.
5. Confirm page-2 W25Q64JWSIQ external NOR is populated for audio assets.
6. Confirm page-3 `BATT_VTG_SENS` divider is connected to EG800AK ADC0 and produces an ADC-safe voltage.
7. Confirm page-3 SW1/SW2/SW3 are active-low keys with pullups to `VDD_EXT`.
8. Confirm SIM hot-plug behavior and network attach with a production SIM/APN.

## Known assumptions

- KAE8_SQ1 net-to-EG800AK-pin mapping is captured in `sb_board_kae8_sq1.h`. The SPI NOR port macro still requires read-ID validation during Phase 1 bring-up because `ql_spi_nor.h` exposes platform port constants rather than schematic net names.
- The EC200U firmware uses several APIs that are not present in EG800AK `ql_application_eg800ak.zip`, such as `ql_mqttclient.h`, `ql_api_datacall.h`, `ql_api_spi6_ext_nor_flash.h`, and `ql_cust_nvm_fread/fwrite`. These must not be used in the EG800AK implementation.
- EG800AK MQTT implementation should use the Paho-style API available under `common/include/mqtt` and demonstrated by `interface/mqtt/example_mqtt.c`.
- The EC200U payload format is weakly structured and delimiter-based. The EG800AK production design must introduce bounded parsing, transaction ID idempotency, signed command validation, and schema versioning.
- SMS recovery is designed as configurable and disabled by default in production.
- OTA manifest verification requires a cryptographic verification method available in the final SDK/security library set. Until the exact crypto API is verified in the EG800AK source tree, the architecture defines the verification boundary but does not invent crypto function names.
