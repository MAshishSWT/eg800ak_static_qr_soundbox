# Migration Architecture - EG800AK-CN Static QR UPI Soundbox

## 1. Design goal

Build a production-ready, maintainable Static QR UPI Soundbox application for EG800AK-CN QuecOpen SDK by extracting reusable business logic from the existing EC200U implementation and replacing all EC200U platform calls with APIs confirmed in `ql_application_eg800ak.zip`.

The application must support:

- Static QR payment notification over MQTT.
- Amount announcement in English, Hindi, Marathi, Gujarati, Bengali, Kannada, Malayalam, Tamil, Telugu, and Punjabi.
- Provider-specific audio prefixes/suffixes: Paytm, PhonePe, Google Pay, BHIM, and other provider packs.
- Power-on, device ready, setup, SIM/network/internet/MQTT errors, battery low, and transaction error prompts.
- Volume and mode keys, long-press battery/network/daily-summary announcements, last transaction browsing.
- Daily total amount, daily transaction count, automatic day reset.
- Battery ADC, CSQ, SIM detection, data call, RTC/NTP, MQTT reconnect/resubscribe.
- HTTP/HTTPS health and command response packets.
- Device commands, firmware OTA, audio pack OTA, SMS recovery/config hooks disabled by default.
- Serial/debug/factory provisioning hooks, persistent NVM/configuration, external-flash audio assets, secure credentials, production logging, watchdog and fault recovery.

## 2. Phase 0 scope boundary

Phase 0 produces architecture and API replacement documents only. It does not add production C implementation, because adding source in this phase would conflict with the stated Phase 0 definition.

Later phases must generate source package incrementally from this architecture.

## 3. EG800AK SDK build model discovered

### 3.1 Application registration

`common/include/ql_application.h` defines the production entry style:

```c
#define application_init(app_entry, app_name, stack_size_kib, startup_prio) \
    appRegItem_t _regAppItem_##app_entry _appRegTable_attr_ = \
    {app_entry, app_name, stack_size_kib*1024, startup_prio}
```

Implication: the soundbox production image must expose exactly one application supervisor entry through `application_init()` and then create service tasks using `ql_rtos_task_create()`.

### 3.2 RTOS primitives

`common/include/ql_rtos.h` provides:

- Tasks: `ql_rtos_task_create()`, `ql_rtos_task_create_ex()`, `ql_rtos_task_delete()`, `ql_rtos_task_sleep_ms()`, `ql_rtos_task_sleep_s()`.
- Queues: `ql_rtos_queue_create()`, `ql_rtos_queue_wait()`, `ql_rtos_queue_release()`, `ql_rtos_queue_get_cnt()`, `ql_rtos_queue_delete()`.
- Semaphores: `ql_rtos_semaphore_create()`, `ql_rtos_semaphore_wait()`, `ql_rtos_semaphore_release()`, `ql_rtos_semaphore_delete()`.
- Mutexes: `ql_rtos_mutex_create()`, `ql_rtos_mutex_lock()`, `ql_rtos_mutex_unlock()`, `ql_rtos_mutex_delete()`.
- Timers: `ql_rtos_timer_create()`, `ql_rtos_timer_start()`, `ql_rtos_timer_stop()`, `ql_rtos_timer_delete()`.
- Flags: `ql_rtos_flag_create()`, `ql_rtos_flag_wait()`, `ql_rtos_flag_release()`, `ql_rtos_flag_delete()`.
- System ticks: `ql_rtos_get_systicks()`.

Use the callback and queue style demonstrated in `interface/os/example_rtos.c`.

### 3.3 Package build pattern

Each interface module owns a local `Makefile`, sets `SRC_FILES`, `INC_DIRS`, optional flags/libraries, then includes `config/common/makefile.mk`.

The Phase 0 soundbox module follows this pattern and avoids CMake because the uploaded `ql_application_eg800ak.zip` uses Makefiles.

### 3.4 Important existing build observation

The updated Phase 0 root `Makefile` compiles:

- `interface/wifi`
- `interface/usbnet`
- `interface/fpu`
- `interface/driver`
- `interface/soundbox`

`interface/soundbox` contributes only the Phase 0 build anchor and does not register `application_init()`. `interface/driver/example_gpio.c` still contains an active `application_init(quec_gpio_test, "quec_gpio_test", 2, 0);`, so Phase 1 production build selection must prevent example applications from starting in the production soundbox image.

## 4. Clean layered architecture

```text
+---------------------------------------------------------------+
| Application Supervisor                                        |
| sb_app_entry, boot sequencing, health, watchdog policy         |
+---------------------------------------------------------------+
| Domain / Business Services                                    |
| payment_service, ledger_service, audio_prompt_service,         |
| key_action_service, daily_summary_service, command_service,    |
| ota_policy_service                                            |
+---------------------------------------------------------------+
| Connectivity Services                                         |
| data_call_service, mqtt_service, http_service, ntp_service,    |
| sms_recovery_service                                          |
+---------------------------------------------------------------+
| Platform Services                                             |
| config_manager, credential_store, fs_asset_store,              |
| time_service, log_service, watchdog_service, factory_service   |
+---------------------------------------------------------------+
| OS Abstraction / Event Bus                                    |
| sb_event_queue, sb_timer, sb_mutex, typed events               |
+---------------------------------------------------------------+
| BSP / HAL                                                     |
| hal_gpio, hal_key, hal_adc, hal_audio, hal_i2c, hal_nor,       |
| hal_uart, hal_sim                                             |
+---------------------------------------------------------------+
| Quectel EG800AK QuecOpen SDK                                  |
| ql_rtos, ql_gpio, ql_adc, ql_audio, ql_data_call, Paho MQTT,   |
| ql_http_client, ql_fota, ql_fs, ql_spi_nor, ql_securedata      |
+---------------------------------------------------------------+
```

## 5. Phase 1 folder structure

```text
interface/soundbox/
  Makefile
  README.md
  app/
    sb_app_main.c
    sb_app_supervisor.c
    sb_app_supervisor.h
  bsp/
    sb_bsp_board_kae8_sq1.c
    sb_bsp_board_kae8_sq1.h
  hal/
    sb_hal_gpio.c/.h
    sb_hal_key.c/.h
    sb_hal_adc.c/.h
    sb_hal_audio.c/.h
    sb_hal_i2c.c/.h
    sb_hal_nor.c/.h
    sb_hal_uart.c/.h
    sb_hal_sim.c/.h
  osal/
    sb_event.h
    sb_event_bus.c/.h
    sb_os.c/.h
    sb_timer.c/.h
  platform/
    sb_config_manager.c/.h
    sb_credential_store.c/.h
    sb_fs_asset_store.c/.h
    sb_log.c/.h
    sb_watchdog.c/.h
    sb_time_service.c/.h
    sb_factory.c/.h
  connectivity/
    sb_data_call_service.c/.h
    sb_mqtt_service.c/.h
    sb_http_service.c/.h
    sb_ntp_service.c/.h
    sb_sms_recovery_service.c/.h
  domain/
    sb_payment_service.c/.h
    sb_command_service.c/.h
    sb_ledger_service.c/.h
    sb_audio_prompt_service.c/.h
    sb_audio_phrasebook.c/.h
    sb_key_action_service.c/.h
    sb_daily_summary_service.c/.h
    sb_ota_policy_service.c/.h
  util/
    sb_safe_string.c/.h
    sb_crc32.c/.h
    sb_ring.c/.h
    sb_amount_format.c/.h
    sb_json_reader.c/.h
```

## 6. Runtime task model

Use a small number of long-running tasks and typed events. Do not share mutable global flags across modules.

| Task | QuecOpen primitive | Responsibility | Key inputs | Key outputs |
|---|---|---|---|---|
| `sb_supervisor_task` | `application_init()` entry then `ql_rtos_task_create()` | Boot sequencing, state machine, watchdog feed, service lifecycle. | Internal events | Service start/stop commands, fault recovery. |
| `sb_event_task` | `ql_rtos_queue_wait()` | Central event dispatch. | Typed `sb_event_t` | Calls domain/connectivity services. |
| `sb_key_task` | GPIO EINT/queue or polling timer | Debounce and classify key presses. | GPIO levels / EINT | `SB_EVT_KEY_SHORT`, `SB_EVT_KEY_LONG`. |
| `sb_audio_task` | queue + audio callbacks | Serialize audio prompt playback. | `SB_EVT_AUDIO_PLAY_REQUEST` | `SB_EVT_AUDIO_DONE`, errors. |
| `sb_connectivity_task` | queue + timers | SIM/network/data call/MQTT lifecycle. | SIM/network/timer events | MQTT online/offline, IP, CSQ. |
| `sb_http_task` | queue + HTTP callbacks | Health packet and command response. | HTTP jobs | response status events. |
| `sb_maintenance_task` | timer | Day reset, periodic health, config flush, asset cleanup. | timer events | maintenance events. |

## 7. Event model

All cross-module communication should use a bounded queue with a fixed event structure. Event types planned for Phase 1:

```text
SB_EVT_BOOT
SB_EVT_SIM_READY
SB_EVT_SIM_MISSING
SB_EVT_NETWORK_REGISTERED
SB_EVT_NETWORK_LOST
SB_EVT_DATA_CALL_UP
SB_EVT_DATA_CALL_DOWN
SB_EVT_MQTT_CONNECTED
SB_EVT_MQTT_DISCONNECTED
SB_EVT_PAYMENT_RECEIVED
SB_EVT_PAYMENT_DUPLICATE
SB_EVT_COMMAND_RECEIVED
SB_EVT_KEY_SHORT
SB_EVT_KEY_LONG
SB_EVT_AUDIO_PLAY_REQUEST
SB_EVT_AUDIO_DONE
SB_EVT_BATTERY_LOW
SB_EVT_DAY_CHANGED
SB_EVT_OTA_REQUEST
SB_EVT_OTA_RESULT
SB_EVT_FAULT
```

## 8. Boot sequence

1. Configure production logging level.
2. Initialize watchdog but delay hard reset policy until minimum services are online.
3. Initialize board pins: keys, LED, speaker shutdown, ADC, UART, I2C/audio path if required.
4. Mount/check internal file system and external audio asset area.
5. Load configuration A/B slot:
   - Check magic.
   - Check version.
   - Check CRC.
   - Select highest valid generation.
   - Apply migration if old version.
   - Fall back to immutable factory defaults if both slots fail.
6. Load secure credentials from `ql_securedata_read()` indexed records and/or encrypted file store.
7. Start key/audio/event/connectivity tasks.
8. Play power-on tune.
9. Check SIM with `ql_sim_get_card_status()` and network with `ql_network_register_wait()`/`ql_nw_get_reg_status()`.
10. Start data call using `ql_start_data_call()`.
11. Sync time via NITZ/RTC and `ql_ntp_sync_ex()` if data call is up.
12. Connect MQTT and subscribe to payment/command topics.
13. Play device ready prompt.
14. Start periodic health and maintenance timers.

## 9. Configuration manager design

### 9.1 Persistent A/B slots

Use two config slots with magic, schema version, generation, length, CRC32, and payload.

```text
config_slot_a.bin
config_slot_b.bin
```

Suggested slot header:

```text
magic:          0x53425131   // "SBQ1"
schema_version: uint16
header_size:    uint16
payload_size:   uint32
generation:     uint32
payload_crc32:  uint32
header_crc32:   uint32
flags:          uint32
```

### 9.2 Payload fields

- Device identity override, merchant ID, client code.
- APN, PDP profile index, MQTT host/port, TLS profile.
- Topic templates.
- Health endpoint, command response endpoint.
- Default language, provider, volume, audio pack version.
- Firmware version and asset version.
- SMS recovery enable flag and allowlist.
- Logging level.
- Battery calibration points.
- OTA policy and retry limits.

### 9.3 Migration

Each schema version must have a deterministic migration function. If migration fails, keep the old slot unchanged and boot with safe defaults plus provisioning-required status.

## 10. Transaction ledger design

EC200U stores the latest amounts in string arrays and a circular double buffer. EG800AK production design should use an idempotent ledger:

```text
transaction_id:       fixed string, max 64 bytes
timestamp_utc:        ql_rtc_time_t or epoch when available
amount_paise:         signed int64, paise only
provider:             enum / bounded string
language:             enum
status:               received, announced, duplicate, error
crc32:                record integrity
```

### 10.1 Idempotency

- MQTT payment payload must include `transaction_id` or equivalent provider reference.
- Store a rolling idempotency index in RAM and flash-backed ledger.
- Duplicate transaction IDs must not increment daily count or total and must not play a new amount announcement.
- If the current backend cannot send transaction ID, introduce a temporary hash of `{amount, provider, backend timestamp, topic, payload}` but treat it as a compatibility fallback, not the final production mechanism.

### 10.2 Daily totals

- Daily total should be stored in paise, not floating point.
- Reset on date change using RTC date after NTP/NITZ sync.
- If RTC is invalid on boot, keep the previous day bucket until time becomes valid.

## 11. Payment message parsing

EC200U currently accepts delimiter payloads such as:

```text
<amount>#<language>#<provider>
conf#<command>#<session>#<value>
play#<language>#<audio>
```

EG800AK production should support versioned JSON commands and preserve delimiter compatibility only through a compatibility parser module.

Recommended Phase 1 payment JSON:

```json
{
  "schema": 1,
  "type": "payment",
  "transactionId": "...",
  "amountPaise": 12345,
  "currency": "INR",
  "provider": "paytm",
  "language": "hi",
  "timestamp": "2026-06-19T10:30:00+05:30",
  "signature": "base64-or-hex"
}
```

## 12. Signed command validation architecture

Commands must not be executed directly after MQTT receive. Use this pipeline:

1. Receive payload from MQTT callback.
2. Copy into bounded buffer and publish event to command service.
3. Parse schema and command ID.
4. Validate timestamp/nonce/replay window.
5. Validate device identity and merchant scope.
6. Verify signature using key material from secure storage.
7. Check command allowlist and production policy.
8. Execute through command handlers.
9. Persist state only after successful execution.
10. Send HTTP/HTTPS command response.

Do not invent a crypto API until the exact EG800AK security/crypto headers in the production SDK are verified. The architecture boundary is `sb_command_verify_signature()`.

## 13. Audio architecture

### 13.1 Audio playback service

Use EG800AK audio APIs from `ql_audio.h` and examples:

- `ql_set_audio_path_speaker()`
- `ql_set_volume()` / `ql_get_volume()`
- `ql_bind_speakerpa_cb()` for speaker PA enable if mapped to `SPK_SHDN`
- `ql_play_mp3()` or `ql_mp3_file_play()` for file playback
- `ql_stop_mp3_play()` / `ql_mp3_file_stop()`
- `ql_audio_play_init()` / `ql_play_file_start()` if the generic file player is selected

### 13.2 Asset path convention

Recommended external flash/file path convention:

```text
/audio/<pack_version>/<provider>/<language>/<asset>.mp3
/audio/<pack_version>/common/<language>/<asset>.mp3
/audio/<pack_version>/numbers/<language>/<token>.mp3
/audio/manifest.json
/audio/manifest.sig
```

### 13.3 Phrase engine

The EC200U `transaction_play.c`, `summary_play.c`, and `dailylog_play.c` should be refactored into a token-list phrase engine:

```text
amount_paise -> token sequence -> asset paths -> audio queue
```

Avoid constructing paths with unsafe `strcat()`. Use bounded formatting helpers.

## 14. Connectivity architecture

### 14.1 SIM and network

Use:

- `ql_sim_get_card_status()`
- `ql_sim_get_imsi()` / `ql_sim_get_iccid()` if required for provisioning.
- `ql_nw_get_csq()` for signal.
- `ql_nw_get_reg_status()` for registration details.
- `ql_network_register_wait(timeout_s)` for initial attach wait as demonstrated in data call, MQTT, HTTP, NTP examples.

### 14.2 Data call

Use:

- `ql_wan_start(nw_status_cb)`
- `ql_set_auto_connect(profile_idx, TRUE)`
- `ql_set_data_call_asyn_mode(enable, cb)` if async state events are needed.
- `ql_start_data_call(profile_idx, ip_version, apn, username, password, auth_type)`
- `ql_get_data_call_info(profile_idx, ip_version, &info)`
- `ql_stop_data_call(profile_idx, ip_version)`

### 14.3 MQTT

EG800AK `ql_application_eg800ak.zip` provides Paho-style MQTT under `common/include/mqtt`, not the EC200U `ql_mqttclient.h` interface.

Use:

- `NetworkInit(&network, &SSLConfig, profile_idx)`
- `NetworkConnect(&network, host, port)`
- `MQTTClientInit(&client, &network, timeout_ms, sendbuf, sendbuf_size, readbuf, readbuf_size)`
- `MQTTConnect(&client, &connectData)`
- `MQTTSubscribe(&client, topic, qos, messageArrived)`
- `MQTTPublish(&client, topic, &message)`
- `MQTTYield(&client, timeout_ms)` if MQTT task mode is disabled.
- `MQTTStartTask(&client)` if using `MQTT_TASK` mode from `MQTTUserConfig.h`.
- `MQTTDisconnect()` and `MQTTClientDeinit()` during recovery.

### 14.4 HTTP/HTTPS

Use `ql_http_client.h` APIs:

- `ql_http_client_init()`
- `ql_http_client_setopt()`
- `ql_http_client_perform()`
- `ql_http_client_release()`
- `ql_http_client_list_append()` / `ql_http_client_list_destroy()` for headers
- Response callback style from `interface/http/example_httpclient_perform.c`.

TLS context uses `SSLConfig` from `ql_ssl_hal.h` where demonstrated.

## 15. OTA architecture

### 15.1 Firmware OTA

Use EG800AK FOTA APIs from `ql_fota.h` and examples:

- `ql_fota_firmware_download()`
- `ql_fota_get_progress()`
- `ql_fota_image_write()`
- `ql_fota_image_flush()`
- `ql_fota_image_verify()`
- `ql_fota_set_update_flag()`
- `ql_fota_start()`

Production OTA must use a secure manifest:

```json
{
  "schema": 1,
  "type": "firmware",
  "version": "1.2.3",
  "url": "https://...",
  "size": 123456,
  "sha256": "...",
  "signature": "...",
  "minBatteryPercent": 30,
  "rebootPolicy": "maintenance_window"
}
```

### 15.2 Audio pack OTA

Audio pack OTA must not overwrite the active pack in-place. Use:

1. Download to staging folder.
2. Verify manifest size/hash/signature.
3. Verify all mandatory assets exist.
4. Atomically switch active pack version in config slot.
5. Keep previous pack until new pack successfully plays a validation prompt.

## 16. Secure credential handling

Use `ql_securedata_store(index, pdata, len)` and `ql_securedata_read(index, pBuffer, len)` for secrets that fit the secure data facility.

Recommended indexes:

| Index | Secret |
|---:|---|
| 1 | MQTT client private key or credential handle. |
| 2 | MQTT client certificate or credential handle. |
| 3 | Root CA bundle hash / CA handle. |
| 4 | Command signature public key or HMAC secret handle. |
| 5 | Factory provisioning token. |

Do not hard-code production certificates in C source.

## 17. Logging and diagnostics

Use `printf()` for early bring-up only. Production logging should wrap `ql_log_mask_set()`/`printf()` behind `sb_log_*()` macros with levels:

- ERROR
- WARN
- INFO
- DEBUG
- TRACE disabled in production

Sensitive values must never be logged:

- Private keys
- MQTT passwords
- Provisioning tokens
- Command signatures
- Full certificate material in production

## 18. Fault recovery and watchdog

Use `ql_wtd.h` APIs:

- `ql_wtd_timeoutperiod_set()`
- `ql_wtd_faultwake_enable()`
- `ql_wtd_enable()`
- `ql_wtd_feed()`

Watchdog policy:

- Feed only from supervisor after all critical services report healthy.
- Do not feed inside individual service loops.
- On repeated MQTT/data call failure, restart connectivity service first.
- On memory/config corruption, boot with defaults and play setup prompt.
- On repeated unrecoverable faults, call `ql_power_reset()` only through supervisor policy.

## 19. Phase implementation plan after Phase 0

### Phase 1 - Build skeleton and OS/event framework

- Add `interface/soundbox/Makefile`.
- Add one application entry point.
- Add safe string, CRC, event queue, logging, board config, config manager skeleton.
- Compile with no network/audio yet.

### Phase 2 - BSP/HAL bring-up

- GPIO, keys, ADC, LED, speaker PA, UART, external NOR ID, file system sanity.
- Hardware self-test menu over UART.

### Phase 3 - Audio engine and phrasebook

- MP3 prompt playback from file system/external flash.
- Multilingual number grammar and provider prompts.

### Phase 4 - Config/ledger persistence

- A/B config slots, CRC, migration, transaction ledger idempotency.

### Phase 5 - Connectivity

- SIM, CSQ, data call, RTC/NTP, MQTT connect/reconnect/resubscribe.

### Phase 6 - Payment and command processing

- Payment payload parser, signed command validator, HTTP health/command response.

### Phase 7 - OTA and recovery

- Firmware OTA, audio pack OTA, SMS recovery disabled by default, production watchdog.

### Phase 8 - Production hardening

- Memory audits, fault injection, power-loss testing, long-run testing, manufacturing provisioning.
