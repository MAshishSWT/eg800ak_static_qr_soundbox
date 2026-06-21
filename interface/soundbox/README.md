# Static QR UPI Soundbox Application

Production application module for EG800AK-CN QuecOpen SDK.

Current package phase: **Phase 6 - MQTT, HTTP and SSL cloud connectivity**.

Implemented layers:

```text
BSP/HAL                 GPIO, LED, keys, ADC battery, speaker PA
OS/Event bus            Queue-backed event dispatch
Platform services       Logging, error codes, U: filesystem, config, CRC, external NOR HW-SPI service, RTC/NTP
Connectivity services   SIM/network/data call, MQTT, HTTP, SSL/TLS profiles
Domain services         Config service, amount tokenizer, audio script builder
Audio services          EG800AK audio HAL, ES8311 support, MP3 playback queue, asset validation
Application supervisor  Startup, heartbeat, event monitoring
```
