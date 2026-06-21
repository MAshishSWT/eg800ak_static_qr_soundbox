# Static QR UPI Soundbox Application

Production application module for EG800AK-CN QuecOpen SDK.

Current package phase: **Phase 9 - SMS recovery, serial/factory provisioning and diagnostics**.

Implemented layers:

```text
BSP/HAL                 GPIO, LED, keys, ADC battery, speaker PA
OS/Event bus            Queue-backed event dispatch
Platform services       Logging, error codes, U: filesystem, config, CRC, secure mode file, external NOR HW-SPI service, RTC/NTP
Connectivity services   SIM/network/data call, MQTT, HTTP, SSL/TLS profiles, OTA download, SMS recovery, USB CDC factory serial
Domain services         Config service, amount tokenizer, audio script builder, payment processor, command dispatcher, transaction ledger, daily summary, signed OTA manifest, factory diagnostics
Audio services          EG800AK audio HAL, ES8311 support, MP3 playback queue, asset validation
Application supervisor  Startup, heartbeat, event monitoring
```

Phase 9 adds controlled production/debug access:

```text
sb_mode_service       production/factory/debug mode persistence
sb_factory_diag       bounded JSON diagnostics and provisioning dispatcher
sb_serial_service     USB CDC serial factory command channel, disabled in production
sb_sms_service        SMS recovery command channel, disabled by default
```

Production security defaults:

```text
Serial factory service disabled in production mode
SMS recovery disabled by default in configuration
Factory provisioning commands rejected unless factory/debug mode or SMS recovery permits them
OTA HMAC key stored through ql_securedata index 1 only
Command payloads and secrets are not logged
```

For manufacturing-only images, define:

```text
-DSB_FACTORY_SERIAL_DEFAULT_ENABLED=1
```

Do not enable that macro in production firmware.

## Phase 10 - Audio asset business logic

Phase 10 adds logical audio asset paths, language aliasing for the supplied asset pack, transaction/summary/health prompt grammar, external-NOR-aware single-asset staging, and one-LED semantic status patterns for KAE8 USER_LED_1.

The app version is `1.0.0-phase10-audio-assets-business-logic` and `SB_PHASE_NUMBER=10`.
