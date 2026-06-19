# Phase 0 Test Procedure

Phase 0 validates package integration, EG800AK SDK compatibility, build-anchor compilation, and KAE8_SQ1 hardware mapping readiness.

## 1. Package integration test

1. Extract original `ql_application_eg800ak.zip`.
2. Overlay this Phase 0 package on top of the extracted directory, or build directly from the archive's `phase0_pkg` directory.
3. Confirm the new paths exist:
   - `docs/soundbox_phase0/PHASE0_README.md`
   - `docs/soundbox_phase0/MIGRATION_ARCHITECTURE.md`
   - `docs/soundbox_phase0/EG800AK_API_REPLACEMENT_MATRIX.md`
   - `docs/soundbox_phase0/EC200U_BUSINESS_LOGIC_INVENTORY.md`
   - `docs/soundbox_phase0/KAE8_SQ1_HARDWARE_MAPPING.md`
   - `docs/soundbox_phase0/PHASE0_TEST_PROCEDURE.md`
   - `docs/soundbox_phase0/PHASE0_ACCEPTANCE_CHECKLIST.md`
   - `docs/soundbox_phase0/PHASE0_REVIEW_FIXES.md`
   - `interface/soundbox/README.md`
   - `interface/soundbox/Makefile`
   - `interface/soundbox/include/sb_phase0_contract.h`
   - `interface/soundbox/include/sb_board_kae8_sq1.h`
   - `interface/soundbox/src/sb_phase0_build_anchor.c`
4. Confirm no Quectel `common/include` or `common/lib` file was modified.
5. Confirm root `Makefile` includes `interface/soundbox` in `COMMPILE_DIRS`.

## 2. Build compatibility test

Run from the SDK application root on the Quectel-supported Windows build environment:

```bat
build.bat app
```

Expected result:

- The soundbox Phase 0 anchor module compiles.
- No additional `application_init()` entry is introduced by `interface/soundbox`.
- No Quectel common header/library modification is required.
- Existing SDK example startup behavior is unchanged.

The uploaded `build.bat` handles `clean` specially and otherwise invokes the default Makefile build. A separate `firmware` target is not assumed by this Phase 0 package.

## 3. Static review test

Review `EG800AK_API_REPLACEMENT_MATRIX.md` and confirm:

- Every EC200U platform API has an EG800AK replacement or an explicit redesign decision.
- MQTT maps to Paho-style `MQTTClient` APIs, not EC200U `ql_mqttclient.h`.
- Data call maps to `ql_data_call.h` signatures in the uploaded EG800AK source tree.
- HTTP maps to `ql_http_client.h` rather than raw EC200U request strings.
- NVM maps to file system/A-B config and `ql_securedata`, not EC200U `ql_cust_nvm_*`.

## 4. Hardware readiness review

With KAE8_SQ1 board and schematic:

1. Confirm battery and charging section:
   - TP4056 charger populated.
   - DW01A/FS8205 protection populated.
   - `BATT_VTG_SENS` divider populated with R24 120K and R27 47K.
2. Confirm audio path:
   - ES8311 codec populated.
   - 8002A PA populated.
   - Speaker connected at SPK1 / LS_P / LS_N.
   - `SPK_SHDN` control path populated.
3. Confirm external flash:
   - W25Q64JWSIQ populated.
   - FLASH_* nets routed to EG800AK pins listed in `sb_board_kae8_sq1.h`.
4. Confirm SIM and antenna:
   - Nano SIM socket populated.
   - Main antenna and matching network populated.
5. Confirm keys:
   - SW1/SW2/SW3 active-low circuits populated.
   - PWRKEY and RESET_N switches populated.

## 5. Phase 1 bring-up preparation tests

These tests are executed during Phase 1 BSP/service implementation:

| Test | EG800AK API/example to use | Expected result |
|---|---|---|
| GPIO LED toggle | `ql_gpio_init()`, `ql_gpio_set_level()` from `example_gpio.c` | USER_LED_1 toggles |
| Key read/debounce | `ql_gpio_get_level()` or `ql_eint_register()` from GPIO/EINT examples | SW1/SW2/SW3 produce active-low events |
| Battery ADC | `ql_adc_read()` from `example_adc.c` | ADC value converts to realistic VBAT |
| External NOR ID | `ql_spi_nor_init()`, `ql_spi_nor_read_id()` from `example_spi_nor.c` | W25Q64 JEDEC ID is read |
| Audio prompt | `ql_set_audio_path_speaker()`, `ql_set_volume()`, `ql_mp3_file_play()` from audio examples | MP3 prompt plays clearly |
| SIM status | `ql_sim_get_card_status()` from `example_sim.c` | SIM ready/missing detected |
| Data call | `ql_network_register_wait()`, `ql_start_data_call()` from `example_datacall.c` | IP/DNS assigned |
| MQTT TLS | Paho `NetworkInit()`, `MQTTConnect()`, `MQTTSubscribe()` from `example_mqtt.c` | Test broker connected and subscribed |
| HTTP health | `ql_http_client_perform()` from `example_httpclient_perform.c` | HTTPS POST succeeds |
| NTP | `ql_ntp_set_server()`, `ql_ntp_sync_ex()` from `example_ntp.c` | RTC-valid time available |
| Watchdog | `ql_wtd_*` from `example_wtd.c` | Supervisor feed policy verified |

## 6. Production risk review before Phase 1

Phase 1 implementation begins only after these product inputs are available:

- Final backend MQTT/HTTP payload schemas.
- Command signature method and key provisioning method.
- Audio asset naming convention and mandatory file list per language/provider.
- SIM/APN policy for production operators.
- Firmware/audio OTA server manifest format.
