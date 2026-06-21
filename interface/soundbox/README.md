# Static QR UPI Soundbox Application

Production application module for EG800AK-CN QuecOpen SDK.

Current package phase: **Phase 4 - audio service**.

Implemented layers:

```text
BSP/HAL                 GPIO, LED, keys, ADC battery, speaker PA
OS/Event bus            Queue-backed event dispatch
Platform services       Logging, error codes, U: filesystem, config, CRC, external NOR HW-SPI service
Domain services         Config service, amount tokenizer, audio script builder
Audio services          EG800AK audio HAL, ES8311 support, MP3 playback queue, asset validation
Application supervisor  Startup, heartbeat, event monitoring
```

Build from the EG800AK SDK root with the Quectel build flow. Runtime MP3 playback requires the MP3 feature to be enabled in OpenEntry.
