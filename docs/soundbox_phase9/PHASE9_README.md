# Soundbox Phase 9 - SMS Recovery, Serial Factory Provisioning and Diagnostics

## Implemented

Phase 9 adds controlled production/debug access services on top of the Phase 8 OTA package:

- Production/factory/debug mode service persisted in `U:/sb_mode.bin` with magic, version and CRC.
- USB CDC serial factory diagnostic/provisioning command service.
- SMS recovery command service, disabled by default in production config.
- Shared bounded JSON factory diagnostic dispatcher.
- Diagnostic status command.
- Controlled provisioning command for network, MQTT, HTTP, language, volume, SMS recovery and log-level config.
- Secure OTA HMAC key provisioning into `ql_securedata` index 1.
- Factory mode switch command through serial only when factory access is already allowed.
- Supervisor events for serial, SMS and factory command execution.

## EG800AK SDK examples referenced

- `interface/network/sms/example_sms.c` for SMS event registration, stored-message index handling, `ql_search_sms_text_message()` and `ql_sms_send_text_msg()`.
- `common/include/ql_sms.h` for SMS event IDs, message structures and delete modes.
- `interface/driver/example_uart.c` for `ql_uart_open()`, `ql_uart_register_cb()`, `ql_uart_read()` and `ql_uart_write()`.
- `common/include/ql_uart.h` for UART port and callback API.
- `interface/securedata/example_securedata.c` for secure data storage API.
- `interface/os/example_rtos.c` for task, queue and mutex use.
- `interface/fs/example_fs.c` for persistent mode file storage through the existing storage wrapper.

## Quectel documents used

- RTOS Development Guide.
- RTOS API Mapping User Guide.
- SMS Development / `(U)SIM` related documentation.
- Serial Port Development Guide.
- Serial Port AT Command Processing User Guide.
- File System Development Guide.
- Device Management Guide.
- SSL Application Note and FOTA Upgrade Guide for secure key provisioning context.

## EC200U modules migrated

The EC200U recovery/provisioning concepts are migrated into isolated EG800AK modules:

- Recovery SMS commands -> `sb_sms_service` + `sb_factory_diag`.
- Factory serial provisioning -> `sb_serial_service` + `sb_factory_diag`.
- Diagnostic status reporting -> `sb_factory_diag`.
- Secure OTA key loading -> `ql_securedata_store()` in `sb_factory_diag`.

No EC200U platform API is copied.

## Security model

Default production behavior is locked:

- Serial factory service is disabled in production mode.
- SMS recovery is disabled by default in config.
- Provisioning commands are rejected unless factory/debug mode is active or SMS recovery is explicitly enabled.
- `set_mode` is serial-only and requires factory access.
- OTA key is stored in secure data index 1 and never logged.
- Command payloads are not logged.

For manufacturing builds, serial factory access can be enabled by adding:

```text
-DSB_FACTORY_SERIAL_DEFAULT_ENABLED=1
```

Do not enable this macro in production firmware.

## Factory/SMS command examples

Diagnostic status:

```json
{"cmd":"diag"}
```

Provision MQTT/HTTP config:

```json
{"cmd":"set_config","mqtt_host":"broker.example.com","mqtt_port":1883,"mqtt_client_id":"sb001","mqtt_sub_topic":"upi/sb001/pay","mqtt_pub_topic":"upi/sb001/event","http_base_url":"https://api.example.com/soundbox","language":"en","volume":70}
```

Enable SMS recovery temporarily:

```json
{"cmd":"set_config","sms_recovery_enabled":1}
```

Provision OTA HMAC key:

```json
{"cmd":"set_ota_key","key_hex":"00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"}
```

Switch mode over factory serial:

```json
{"cmd":"set_mode","mode":"production"}
```

Supported modes:

```text
production
factory
debug
```

## Integration inside ql_application_eg800ak.zip

The package keeps the existing clean application module layout:

```text
interface/soundbox/include/sb_mode_service.h
interface/soundbox/include/sb_factory_diag.h
interface/soundbox/include/sb_serial_service.h
interface/soundbox/include/sb_sms_service.h
interface/soundbox/src/sb_mode_service.c
interface/soundbox/src/sb_factory_diag.c
interface/soundbox/src/sb_serial_service.c
interface/soundbox/src/sb_sms_service.c
```

`interface/soundbox/Makefile` registers the new source files and sets:

```text
-DSB_PHASE_NUMBER=9
```

No Quectel common header or library is modified.

## Build

Copy the package over the EG800AK `ql-application/threadx` tree and build with the normal Quectel OpenEntry SDK build flow.

## KAE8_SQ1 hardware test

1. Flash the Phase 9 package.
2. Confirm boot logs show Phase 9 version.
3. In production mode, confirm serial service logs disabled.
4. Build a factory image with `SB_FACTORY_SERIAL_DEFAULT_ENABLED=1` for manufacturing/provisioning test only.
5. Send `{"cmd":"diag"}` over USB CDC serial and verify JSON reply.
6. Use serial `set_config` to provision MQTT/HTTP and enable SMS recovery only if needed.
7. Send SMS `{"cmd":"diag"}` after enabling SMS recovery and verify reply SMS.
8. Provision OTA key with `set_ota_key` and verify Phase 8 OTA test path remains functional.
9. Switch mode back to production before release.

## Known assumptions

- SMS recovery is intentionally disabled by default.
- Serial factory access is intentionally disabled in production unless a manufacturing build macro is set.
- SMS payloads are expected to be short JSON commands fitting the Quectel text SMS buffer.
- External NOR remains a separate hardware/SPI issue and is not used by Phase 9.

## Review security fixes

The review-fixed Phase 9 package adds two protections for SMS recovery:

1. SMS commands are accepted only from the authorized sender number stored in secure data index 3.
2. SMS provisioning commands remain blocked in production builds unless the explicit lab-only macro is enabled:

```text
-DSB_ENABLE_INSECURE_SMS_RECOVERY_PROVISIONING=1
```

Do not enable this macro in production firmware. Production recovery should use factory serial provisioning, or a future HMAC-signed SMS recovery command profile.

Provision the authorized SMS sender number from factory serial:

```json
{"cmd":"set_sms_auth","phone":"+919999999999"}
```
