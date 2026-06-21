# Static QR UPI Soundbox Application

Production application module for EG800AK-CN QuecOpen SDK.

Current package phase: **Phase 5 - SIM, network, data call and time service**.

Implemented layers:

```text
BSP/HAL                 GPIO, LED, keys, ADC battery, speaker PA
OS/Event bus            Queue-backed event dispatch
Platform services       Logging, error codes, U: filesystem, config, CRC, external NOR HW-SPI service, RTC/NTP
Connectivity services   SIM check, network registration, PDP data call, CSQ, reconnect FSM
Domain services         Config service, amount tokenizer, audio script builder
Audio services          EG800AK audio HAL, ES8311 support, MP3 playback queue, asset validation
Application supervisor  Startup, heartbeat, event monitoring
```
